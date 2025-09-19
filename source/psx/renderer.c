#include "renderer.h"
#include "../renderer.h"
#include "../common.h"

#include "texture_pool.h"
#include "collision.h"
#include "particles.h"
#include "lut.h"

#include <string.h>
#include <assert.h>
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
int tex_level_alloc_cursor = 0;
int tex_entity_alloc_cursor = 0;
int tex_misc_alloc_cursor = 0;
int store_to_precomp_prims = 0;
uint32_t sxy_storage = 0;

// Textures
texture_entry_t textures_level[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_entity[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_misc[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_weapon[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_persistent[MAX_TEXTURE_COUNT] = {0};
texture_entry_t* renderer_get_texture_entry(texture_category_t category, int texture_id) {
    if (texture_id >= 0) {
        switch (category) {
            case TEX_CAT_NONE: return NULL;
            case TEX_CAT_LEVEL: return &textures_level[texture_id];
            case TEX_CAT_ENTITY: return &textures_entity[texture_id];
            case TEX_CAT_WEAPON: return &textures_weapon[texture_id];
            case TEX_CAT_MISC: return &textures_misc[texture_id];
            case TEX_CAT_PERSISTENT: return &textures_persistent[texture_id];
            default: printf("[ERROR] Invalid texture category %i\n", (int)category); return NULL;
        }
    }
    return NULL;
}

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

void renderer_psx_clear_vram(svec2_t top_left, svec2_t size, pixel32_t color) {
    FILL fill;
    setFill(&fill);
    setRGB0(&fill, color.r, color.g, color.b);
    setXY0(&fill, top_left.x, top_left.y);
    setWH(&fill, size.x, size.y);
    DrawPrim((uint32_t*)&fill);
    DrawSync(0);
}

void renderer_init(void) {
    texture_pool_init(0, 0, 256, 256);
    texture_pool_init(1, 256, 256, 256);
    texture_pool_init(2, 512, 256, 256);
    texture_pool_init(3, 768, 256, 256);

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

#ifdef _DEBUG
    int fnt_alloc = texture_pool_alloc(3, 32, 32);
    rect_t fnt_rect = texture_pool_rect(3, fnt_alloc);
    FntLoad(fnt_rect.x, fnt_rect.y);
    FntOpen(32, 32, 256, 192, 0, 512);
#endif

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

void renderer_draw_mesh_shaded(mesh_t* mesh, const transform_t* model_transform, int local, int facing_camera) {
    if (!mesh) {
        printf("renderer_draw_mesh_shaded: mesh was null!\n");
        return;
    }
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

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, texture_category_t category) {
    // Fetch the right texture entry
    texture_entry_t* entry = renderer_get_texture_entry(category, texture_id);
    if (entry == NULL) return;

    // Allocate quad primitive
    POLY_FT4* new_quad = (POLY_FT4*)next_primitive;
    next_primitive += sizeof(POLY_FT4) / sizeof(*next_primitive);
    setPolyFT4(new_quad); 

    // Position
    const int y_offset = is_pal ? 0 : -16;
    setXY4(new_quad,
        (tl.x / ONE), (tl.y / ONE) + y_offset,
        (tr.x / ONE), (tr.y / ONE) + y_offset,
        (bl.x / ONE), (bl.y / ONE) + y_offset,
        (br.x / ONE), (br.y / ONE) + y_offset
    );

    // Color
    setRGB0(new_quad, color.r, color.g, color.b);
    
    // Texture info
    new_quad->clut = entry->clut;
    new_quad->tpage = entry->tpage;
    setUV4(new_quad,
        (uv_tl.x / ONE) + entry->offset_u, (uv_tl.y / ONE) + entry->offset_v,
        (uv_br.x / ONE) + entry->offset_u, (uv_tl.y / ONE) + entry->offset_v,
        (uv_tl.x / ONE) + entry->offset_u, (uv_br.y / ONE) + entry->offset_v,
        (uv_br.x / ONE) + entry->offset_u, (uv_br.y / ONE) + entry->offset_v
    );
    addPrim(ord_tbl[drawbuffer] + depth + curr_ot_bias, new_quad);
}

void renderer_apply_fade(scalar_t fade_level) {
    fade_level /= ONE;

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

void renderer_upload_texture(const texture_cpu_t* texture, int index, texture_category_t category) {
    // Calculate VRAM width based on bpp
    assert(texture != NULL);
    uint32_t width = (uint32_t)texture->width;
    uint32_t height = (uint32_t)texture->height;
    if (width == 0) width = 256;
    if (height == 0) height = 256;
    if (texture->bits_per_pixel == 8) width /= 2;
    else if (texture->bits_per_pixel == 4) width /= 4;

    // Allocate texture and palette in VRAM
    int pool_id = -1;
    int texture_id = -1;
    while (texture_id < 0 && ++pool_id < 4) {
        texture_id = texture_pool_alloc((uint32_t)pool_id, width, height);
    }
#ifdef _DEBUG
    if (texture_id < 0) {
        printf("[ERROR] Failed to allocate %i bit texture with index %i and resolution (%ix%i)\n", texture->bits_per_pixel, index, texture->width ? texture->width : 256, height);
        return;
    }
#endif
    int palette_id = texture_pool_alloc(3, (1 << texture->bits_per_pixel), texture->palette_count);
#ifdef _DEBUG
    if (palette_id < 0) {
        printf("[ERROR] Failed to allocate palette\n");
        return;
    }
#endif
    
    // Upload texture pixels to VRAM
    const rect_t texture_rect = texture_pool_rect(pool_id, texture_id);
    const rect_t palette_rect = texture_pool_rect(3, palette_id);
    LoadImage((RECT*)&texture_rect, (uint32_t*)texture->data);
    LoadImage((RECT*)&palette_rect, (uint32_t*)texture->palette);
    DrawSync(0);
    
    // Calculate texture metadata
    /*if (texture->bits_per_pixel == 16)*/ uint16_t texture_mode = 2;
    if (texture->bits_per_pixel == 8) texture_mode = 1;
    else if (texture->bits_per_pixel == 4) texture_mode = 0;
    #ifdef _DEBUG   
    else printf("[WARN] texture->bits_per_pixel: got %i, expected 4, 8, or 16\n", texture->bits_per_pixel);
    #endif
    
    // Store texture metadata
    uint32_t offset_u = texture_rect.x;
    if (texture->bits_per_pixel == 8) offset_u *= 2;
    else if (texture->bits_per_pixel == 4) offset_u *= 4;
    
    const texture_entry_t tex_entry = {
        .tpage = getTPage(texture_mode, 0, texture_rect.x, texture_rect.y),
        .clut = getClut(palette_rect.x, palette_rect.y),
		.width = texture->width,
		.height = texture->height,
        .offset_u = (uint8_t)(offset_u & 0xFF),
        .offset_v = (uint8_t)(texture_rect.y & 0xFF),
        .texture_pool_id = (uint8_t)pool_id,
        .texture_entry_id = texture_id,
        .palette_pool_id = 3,
        .palette_entry_id = palette_id,
        .allocated = 1,
        .average_color = texture->avg_color,
    };

    switch (category) {
        case TEX_CAT_LEVEL: textures_level[index] = tex_entry; break;
        case TEX_CAT_ENTITY: textures_entity[index] = tex_entry; break;
        case TEX_CAT_WEAPON: textures_weapon[index] = tex_entry; break;
        case TEX_CAT_MISC: textures_misc[index] = tex_entry; break;
        case TEX_CAT_PERSISTENT: textures_persistent[index] = tex_entry; break;
        default: break;
    }
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

int curr_dt_ms = 33;

int renderer_delta_time_ms(dt_flags_t flags) {
    if (flags == DT_TICK) {
        int dt_raw = renderer_get_delta_time_raw();
        curr_dt_ms = renderer_convert_dt_raw_to_ms(dt_raw);
    }
    return curr_dt_ms;
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
        renderer_draw_2d_quad_axis_aligned(center, size, (vec2_t){0, 0}, (vec2_t){63, 63}, color, depth, system->params->texture_id + p->curr_frame / ONE, TEX_CAT_MISC);
    }
}

#pragma GCC diagnostic pop
