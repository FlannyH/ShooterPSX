#include "renderer.h"

#include "particles.h"
#include "lut.h"

#include <string.h>
#include <psxgte.h>
#include <psxgpu.h>

#define TRI_THRESHOLD_MUL_SUB2_30 3
#define TRI_THRESHOLD_MUL_SUB1_30 7
#define TRI_THRESHOLD_MUL_SUB2_60 2
#define TRI_THRESHOLD_MUL_SUB1_60 4
#define TRI_THRESHOLD_NORMAL 500
#define TRI_THRESHOLD_FADE_START 600
#define TRI_THRESHOLD_FADE_END 900
#define MESH_RENDER_DISTANCE 9600
#define N_CLUT_FADES 16
#define N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE 4

// Render context
DISPENV disp[2];
DRAWENV draw[2];
int drawbuffer;
int curr_res_y = RES_Y_NTSC;

// Primitives
uint32_t ord_tbl[2][ORD_TBL_LENGTH];
uint32_t* next_primitive;

// Rendering parameters
extern uint8_t tex_id_start;
int curr_ot_bias = 0;

// Camera info
MATRIX view_matrix;
MATRIX aspect_matrix;
vec3_t camera_pos;
vec3_t camera_dir;

// Settings
int horizontal_resolutions[] = {256, 320, 368, 512};
int res_x = 512;
int vsync_enable = 2;
int is_pal = 0;

// Misc
int drawn_first_frame = 0;
int frame_counter = 0;
int n_meshes_drawn = 0;
int delta_time_raw_curr = 0;
int delta_time_raw_prev = 0;
int tex_level_start = 0;
int tex_entity_start = 0;
int tex_weapon_start = 0;
int tex_alloc_cursor = 0;
int store_to_precomp_prims = 0;
uint32_t sxy_storage = 0;

// Textures
pixel32_t textures_avg_colors[256];
RECT textures[256];
RECT palettes[256];

// Import inline helper functions
#include "renderer_inline.c"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

int renderer_width(void) {
    return res_x;
}

int renderer_height(void) {
    return curr_res_y;
}

void renderer_init(void) {
    SetVideoMode(MODE_NTSC);

    // Configures the pair of DISPENVs
    SetDefDispEnv(&disp[0], 0, 0, res_x, RES_Y_NTSC);
    SetDefDispEnv(&disp[1], res_x, 0, res_x, RES_Y_NTSC);

    // Configures the pair of DRAWENVs for the DISPENVs
    SetDefDrawEnv(&draw[0], res_x, 0, res_x, RES_Y_NTSC);
    SetDefDrawEnv(&draw[1], 0, 0, res_x, RES_Y_NTSC);
    
    // Specifies the clear color of the DRAWENV
    setRGB0(&draw[0], 16, 16, 20);
    setRGB0(&draw[1], 16, 16, 20);

    // Enable background clear
    draw[0].isbg = 1;
    draw[1].isbg = 1;

    //Set to drawbuffer 0
    drawbuffer = 0;

    // Clear ordering tables
    ClearOTagR(ord_tbl[0], ORD_TBL_LENGTH);
    ClearOTagR(ord_tbl[1], ORD_TBL_LENGTH);

    // Initialize GTE
    InitGeom();

    // Set up where we want the center of the screen to be
    gte_SetGeomOffset(res_x / 2, curr_res_y / 2);
    gte_SetGeomScreen(120);

    drawn_first_frame = 0;
}

void renderer_begin_frame(const transform_t* camera_transform) {
    mem_stack_release(STACK_TEMP);

    // Set the next primitive to draw to be the first primitive in the buffer
    uint32_t* primitive_buffer_on_temp_stack = (uint32_t*)mem_stack_alloc(256 * KiB, STACK_TEMP);
    next_primitive = &primitive_buffer_on_temp_stack[drawbuffer * ((128 * KiB) / sizeof(uint32_t))];

	renderer_set_depth_bias(0);

    // Get camera position
    VECTOR position;
    memcpy(&position, &camera_transform->position, sizeof(position));
    memcpy(&camera_pos, &camera_transform->position, sizeof(camera_pos));
    position.vx = -position.vx >> 12;
    position.vy = -position.vy >> 12;
    position.vz = -position.vz >> 12;
    HiRotMatrix((VECTOR*)&camera_transform->rotation, &view_matrix); // VECTOR and vec3_t are identical bit-wise
    ApplyMatrixLV(&view_matrix, &position, &position);
    TransMatrix(&view_matrix, &position);

    // Scale by aspect ratio
    aspect_matrix = (MATRIX){
        .m = {
            {(ONE * res_x) / (widescreen ? 427 : 320), 0, 0},
            {0, ONE, 0},
            {0, 0, ONE},
        },
        .t = {0, 0, 0},
    };
    CompMatrixLV(&aspect_matrix, &view_matrix, &view_matrix);

    // Handle metadata
	camera_dir.x = view_matrix.m[2][0];
	camera_dir.y = view_matrix.m[2][1];
	camera_dir.z = view_matrix.m[2][2];
    n_meshes_drawn = 0;
}

void renderer_end_frame(void) {
    renderer_tick_fade();
    
    // Wait for GPU to finish drawing and V-blank
    DrawSync(0);

    if (vsync_enable)
    {
        while (((VSync(-1) - frame_counter) & 0x3FFF) < vsync_enable)
            VSync(0);
        frame_counter = VSync(-1);
    }

    // Flip buffer counter
    drawbuffer = !drawbuffer;

    // Clear render queue
    ClearOTagR(ord_tbl[drawbuffer], ORD_TBL_LENGTH);
    
    // Apply Envs
    PutDispEnv(&disp[drawbuffer]);
    PutDrawEnv(&draw[drawbuffer]);
    
    // Enable display
    if (drawn_first_frame)
        SetDispMask(1);
    
    // Draw Ordering Table
    DrawOTag(ord_tbl[1-drawbuffer] + ORD_TBL_LENGTH - 1);

    drawn_first_frame = 1;
}

void renderer_draw_mesh_shaded(const mesh_t* mesh, const transform_t* model_transform, int local, int facing_camera, int tex_id_offset) {
    if (!mesh) {
        printf("renderer_draw_mesh_shaded: mesh was null!\n");
        return;
    }
    tex_id_start = tex_id_offset;

    // Set rotation and translation matrix
    MATRIX model_matrix;
    if (facing_camera)  {
        const vec3_t up = vec3_from_scalars(0, ONE, 0);
        const vec3_t forward = vec3_normalize(vec3_sub(vec3_muls(camera_pos, 192), vec3_muls(model_transform->position, 192*ONE))); // 192 to add some more precision when very close to the player
        const vec3_t right = vec3_normalize(vec3_cross(up, forward));
        model_matrix.m[0][0] = right.x;     model_matrix.m[1][0] = right.y;     model_matrix.m[2][0] = right.z;
        model_matrix.m[0][1] = up.x;        model_matrix.m[1][1] = up.y;        model_matrix.m[2][1] = up.z;
        model_matrix.m[0][2] = forward.x;   model_matrix.m[1][2] = forward.y;   model_matrix.m[2][2] = forward.z;
    }
    else HiRotMatrix((VECTOR*)&model_transform->rotation, &model_matrix); // VECTOR and vec3_t are identical bitwise
    TransMatrix(&model_matrix, (VECTOR*)&model_transform->position); 

    if (local)  CompMatrixLV(&aspect_matrix, &model_matrix, &model_matrix);
    else        CompMatrixLV(&view_matrix, &model_matrix, &model_matrix);

    // Send it to the GTE
	PushMatrix();
    gte_SetRotMatrix(&model_matrix);
    gte_SetTransMatrix(&model_matrix);

	// If the mesh's bounding box is not inside the viewing frustum, cull it
    if (frustrum_cull_aabb(mesh->bounds.min, mesh->bounds.max)) return;

    ++n_meshes_drawn;

    // Loop over each triangle
    size_t vert_idx = 0;
    if (mesh->optimized_for_single_render_per_frame) {
        store_to_precomp_prims = 1;
        for (size_t i = 0; i < mesh->n_triangles; ++i) {
            draw_tex_triangle3d_fancy(mesh, vert_idx, i);
            vert_idx += 3;
        }
        for (size_t i = 0; i < mesh->n_quads; ++i) {
            draw_tex_quad3d_fancy(mesh, vert_idx, i);
            vert_idx += 4;
        }
    }
    else {
        store_to_precomp_prims = 0;
        for (size_t i = 0; i < mesh->n_triangles; ++i) {
            draw_tex_triangle3d_fancy_no_precomp(mesh, vert_idx, i);
            vert_idx += 3;
        }
        for (size_t i = 0; i < mesh->n_quads; ++i) {
            draw_tex_quad3d_fancy_no_precomp(mesh, vert_idx, i);
            vert_idx += 4;
        }
    }

	PopMatrix();
}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
    const int y_offset = is_pal ? 0 : -16;
    POLY_FT4* new_triangle = (POLY_FT4*)next_primitive;
    next_primitive += sizeof(POLY_FT4) / sizeof(*next_primitive);
    setPolyFT4(new_triangle); 
    setXY4(new_triangle, 
        (tl.x * res_x) / (512 * ONE), (tl.y / ONE) + y_offset,
        (tr.x * res_x) / (512 * ONE), (tr.y / ONE) + y_offset,
        (bl.x * res_x) / (512 * ONE), (bl.y / ONE) + y_offset,
        (br.x * res_x) / (512 * ONE), (br.y / ONE) + y_offset
    );
    setRGB0(new_triangle, color.r, color.g, color.b);\
    if (is_page) {
        new_triangle->clut = getClut(768, 256 + (32*(texture_id)));
        new_triangle->tpage = getTPage(1, 0, 128 * texture_id, 256) & ~(1 << 9);
        setUV4(new_triangle,
            uv_tl.x / ONE, uv_tl.y / ONE, // top left
            uv_br.x / ONE, uv_tl.y / ONE, // top right
            uv_tl.x / ONE, uv_br.y / ONE, // bottom left
            uv_br.x / ONE, uv_br.y / ONE // bottom right
        );
    }
    else {
        const uint8_t tex_offset_x = ((texture_id) & 0b00001100) << 4;
        const uint8_t tex_offset_y = ((texture_id) & 0b00000011) << 6;
        new_triangle->clut = getClut(palettes[texture_id].x, palettes[texture_id].y);
        new_triangle->tpage = getTPage(0, 0, textures[texture_id].x, textures[texture_id].y) & ~(1 << 9);
        setUV4(new_triangle,
            uv_tl.x + tex_offset_x, uv_tl.y + tex_offset_y,
            uv_br.x + tex_offset_x, uv_tl.y + tex_offset_y,
            uv_tl.x + tex_offset_x, uv_br.y + tex_offset_y,
            uv_br.x + tex_offset_x, uv_br.y + tex_offset_y
        );
    }
    addPrim(ord_tbl[drawbuffer] + depth + curr_ot_bias, new_triangle);
}

void renderer_apply_fade(int fade_level) {
    if (fade_level <= 0) return;
    if (fade_level > 255) fade_level = 255;
    fade_level *= fade_level;
    fade_level /= 255;

    // Add rectangle
    TILE* new_tile = (TILE*)next_primitive;
    next_primitive += sizeof(TILE) / sizeof(*next_primitive);
    setTile(new_tile);
    setSemiTrans(new_tile, 1);
    setRGB0(new_tile, fade_level, fade_level, fade_level);
    setXY0(new_tile, 0, 0);
    setWH(new_tile, res_x, curr_res_y);
    addPrim(ord_tbl[drawbuffer] + 0, new_tile);
    
    // Set color blend mode to subtract
    DR_TPAGE* new_tpage = (DR_TPAGE*)next_primitive;
    next_primitive += sizeof(DR_TPAGE) / sizeof(*next_primitive);
    setDrawTPage(new_tpage, 1, 0, 2 << 5);
    addPrim(ord_tbl[drawbuffer] + 0, new_tpage);
}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {
    // Set rotation and translation matrix
    MATRIX model_matrix;
    HiRotMatrix((VECTOR*)&model_transform->rotation, &model_matrix); // VECTOR and vec3_t are identical bitwise
    TransMatrix(&model_matrix, (VECTOR*)&model_transform->position);
    CompMatrixLV(&view_matrix, &model_matrix, &model_matrix);

    // Send it to the GTE
	PushMatrix();
    gte_SetRotMatrix(&model_matrix);
    gte_SetTransMatrix(&model_matrix);

    // Transform line to screen space
    int16_t v0_tr[2];
    int16_t v1_tr[2];
    svec3_t sv0 = {v0.x / -COL_SCALE, v0.y / -COL_SCALE, v0.z / -COL_SCALE};
    svec3_t sv1 = {v1.x / -COL_SCALE, v1.y / -COL_SCALE, v1.z / -COL_SCALE};
    int depth = 0;
    gte_ldv3(&sv0.x, &sv1.x, &sv1.x);
    gte_rtpt();
    gte_stsxy3(v0_tr, v1_tr, v1_tr);
    gte_avsz3();
    gte_stotz(&depth);
    if (depth >= ORD_TBL_LENGTH || (depth <= 0)) return;

    // Add to queue
    LINE_F2* new_line = (LINE_F2*)next_primitive;
    next_primitive += sizeof(LINE_F2) / sizeof(*next_primitive);
    setLineF2(new_line);
    setXY2(new_line,
        v0_tr[0], v0_tr[1],
        v1_tr[0], v1_tr[1]
    );
    setColor0(new_line, *(uint32_t*)&color); // ugly but eh it's debug anyway
    addPrim(ord_tbl[drawbuffer] + depth + curr_ot_bias, new_line);

    PopMatrix();
}

void renderer_upload_texture(const texture_cpu_t* texture, uint8_t index) {
    // Load texture pixels to VRAM - starting from 0,256, spanning 512x256 VRAM pixels, stored in 16x64 blocks (for 64x64 texture)
    // This means that the grid consists of 32x4 textures
    WARN_IF("texture is not 64x64!", texture->width != 64 || texture->height != 64);
    const RECT rect_tex = {
        0 + ((int16_t)index / 4) * 16,
        256 + (((int16_t)index) % 4) * 64,
        (int16_t)texture->width / 4,
        (int16_t)texture->height
    };
    LoadImage(&rect_tex, (uint32_t*)texture->data);
    DrawSync(0);
    textures[index] = rect_tex;
    textures_avg_colors[index] = texture->avg_color;

    // Load palette to VRAM - starting from 512,384, spanning 256x128 VRAM pixels, stored in 16x16 blocks (for 16 16-color palettes, with fades to the average texture color for distance blur)
    const RECT rect_palette = {
        768 + ((int16_t)index % 16) * 16,
        256 + (((int16_t)index) / 16) * 16,
        16,
        16
    };

    LoadImage(&rect_palette, (uint32_t*)texture->palette);
    DrawSync(0);
    palettes[index] = rect_palette;
}

void renderer_upload_8bit_texture_page(const texture_cpu_t* texture, const uint8_t index) {
    WARN_IF("texture is not a full page!", texture->width != 0 || texture->height != 0); // 0 is interpreted as 256
    const RECT rect_page = {
        index * 256 / (16 / 8), // X: 256 pixels, 8bpp
        256, // Y: fixed at 256, which is where textures are stored,
        128,
        256
    };
    LoadImage(&rect_page, (uint32_t*)texture->data);
    DrawSync(0);

    // Load palette to VRAM - starting from 512,384, spanning 256x128 VRAM pixels, stored in 256x16 blocks (for 16 256-color palettes, with fades to the average texture color for distance blur)
    const RECT rect_palette = {
        768,
        256 + ((int16_t)index * 32),
        256,
        1
    };
    LoadImage(&rect_palette, (uint32_t*)texture->palette);
    DrawSync(0);
}

void renderer_set_video_mode(int is_pal) {
    ResetGraph(0);
    if (is_pal) {
        // Configures the pair of DISPENVs
        SetDefDispEnv(&disp[0], 0, 0, res_x, RES_Y_PAL);
        SetDefDispEnv(&disp[1], res_x, 0, res_x, RES_Y_PAL);

        // Configures the pair of DRAWENVs for the DISPENVs
        SetDefDrawEnv(&draw[0], res_x, 0, res_x, RES_Y_PAL);
        SetDefDrawEnv(&draw[1], 0, 0, res_x, RES_Y_PAL);

        // Seems like I have to do this in order to actually set the display
        // resolution to the right value?
        disp[0].screen.h = RES_Y_PAL;
        disp[1].screen.h = RES_Y_PAL;

        SetVideoMode(MODE_PAL);
        curr_res_y = RES_Y_PAL;
    }
    else {
        // Configures the pair of DISPENVs
        SetDefDispEnv(&disp[0], 0, 0, res_x, RES_Y_NTSC);
        SetDefDispEnv(&disp[1], res_x, 0, res_x, RES_Y_NTSC);

        // Configures the pair of DRAWENVs for the DISPENVs
        SetDefDrawEnv(&draw[0], res_x, 0, res_x, RES_Y_NTSC);
        SetDefDrawEnv(&draw[1], 0, 0, res_x, RES_Y_NTSC);

        SetVideoMode(MODE_NTSC);
        curr_res_y = RES_Y_NTSC;

    }
    gte_SetGeomOffset(res_x / 2, curr_res_y / 2);
    gte_SetGeomScreen(120);

    // Specifies the clear color of the DRAWENV
    setRGB0(&draw[0], 16, 16, 20);
    setRGB0(&draw[1], 16, 16, 20);
    
    // Enable background clear
    draw[0].isbg = 1;
    draw[1].isbg = 1;
    drawn_first_frame = 0;
}

void renderer_set_depth_bias(int bias) {
    curr_ot_bias = bias;
}

int renderer_get_delta_time_raw(void) {
    if (vsync_enable) {
        delta_time_raw_prev = delta_time_raw_curr;
        delta_time_raw_curr = VSync(-1);
        return (delta_time_raw_curr - delta_time_raw_prev) & 0x7FFF;
    }
    else {
        delta_time_raw_prev = delta_time_raw_curr;
        delta_time_raw_curr = VSync(1);
        return (delta_time_raw_curr - delta_time_raw_prev) & 0x7FFF;
    }
}

int renderer_get_delta_time_ms(void) {
    int dt_raw = renderer_get_delta_time_raw();
    return renderer_convert_dt_raw_to_ms(dt_raw);
}

int renderer_n_meshes_drawn(void) { return n_meshes_drawn; }

int renderer_convert_dt_raw_to_ms(int dt_raw) {
    int dt_ms;
    if (vsync_enable) {
        if (is_pal)
            dt_ms = 20 * dt_raw;
        else
            dt_ms = (16666 * dt_raw) / 1000;
    }
    else {
        dt_ms = (1000 * dt_raw) / 15625; // Somehow this works for both PAL and NTSC
    }
    if (dt_ms == 0) {
        dt_ms = 1;
    }
    return dt_ms;
}

int renderer_should_close(void) {
    return 0;
}

void renderer_draw_particle_system(particle_system_t* system, scalar_t dt) {
    // Loop over the particles in chunks of 3
    for (int i = 0; i < system->params->n_particles_max; ++i) {
        particle_t* p = &system->particle_buffer[i];

        // If none of these are active, don't render them
        if (p->time_alive > p->total_lifetime) continue;

        // Transform the point
        gte_ldv0(&p->position);
        gte_rtps_b();

        // While it's transforming, we update the particle's data
        p->velocity = vec3_mul(p->velocity, system->params->velocity_multiplier_over_time);
        p->velocity = vec3_add(p->velocity, vec3_muls(system->params->constant_acceleration, dt));
        p->position = vec3_add(p->position, vec3_muls(p->velocity, dt));
        p->scale = vec2_mul(p->scale, system->params->scale_multiplier_over_time);
        p->curr_frame += dt * system->params->animation_frame_rate;
        p->curr_frame -= ((p->curr_frame / ONE) > system->params->n_animation_frames) ? system->params->loop_start * ONE : 0;

        // Let's get the transformed point, and base the size on the Z component. The number here is a bit hacky but eh it works right
        svec2_t scenter;
        scalar_t depth;
        gte_stsxy(&scenter);
        gte_stsz(&depth);
        vec2_t size = vec2_divs(p->scale, depth);
        vec2_t center = (vec2_t){scenter.x * ONE, scenter.y * ONE};
        pixel32_t color = {
            .r = scalar_lerp(p->start_colour.r, p->end_colour.r, scalar_div(p->time_alive, p->total_lifetime)),
            .g = scalar_lerp(p->start_colour.g, p->end_colour.g, scalar_div(p->time_alive, p->total_lifetime)),
            .b = scalar_lerp(p->start_colour.b, p->end_colour.b, scalar_div(p->time_alive, p->total_lifetime)),
            .a = 255,
        };

        // Render quad
        renderer_draw_2d_quad_axis_aligned(center, size, (vec2_t){0, 0}, (vec2_t){63, 63}, color, depth, system->params->texture_id + p->curr_frame / ONE, 0);
    }
}

#pragma GCC diagnostic pop
