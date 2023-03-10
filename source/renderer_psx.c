#ifdef _PSX
#include "renderer.h"
#include <stdio.h>
#include <psxgte.h>	// GTE header, not really used but libgpu.h depends on it
#include <psxgpu.h>	// GPU library header
#include <psxetc.h>	// Includes some functions that controls the display
#include <inline_c.h>

// Define environment pairs and buffer counter
DISPENV disp[2];
DRAWENV draw[2];
int drawbuffer;
int delta_time_raw_curr = 0;
int delta_time_raw_prev = 0;
uint32_t n_total_triangles = 0;
int n_rendered_triangles = 0;

uint32_t ord_tbl[2][ORD_TBL_LENGTH];
uint32_t primitive_buffer[2][(32768 << 3) / sizeof(uint32_t)];
uint32_t* next_primitive;
MATRIX view_matrix;
RECT	screen_clip;
int vsync_enable = 0;
int frame_counter = 0;

pixel32_t textures_avg_colors[256];
RECT textures[256];
RECT palettes[256];

void renderer_init(void) {
    // Reset GPU and enable interrupts
    ResetGraph(0);

    // Configures the pair of DISPENVs
    SetDefDispEnv(&disp[0], 0, 0, RES_X, RES_Y);
    SetDefDispEnv(&disp[1], 0, RES_Y, RES_X, RES_Y);

    // Screen offset to center the picture vertically
#ifdef PAL
    disp[0].screen.y = 24;
    disp[1].screen.y = 24;
#endif

    // Forces PAL video standard
#ifdef PAL
    SetVideoMode(MODE_PAL);
#else
    SetVideoMode(MODE_NTSC);
#endif

    // Configures the pair of DRAWENVs for the DISPENVs
    SetDefDrawEnv(&draw[0], 0, RES_Y, RES_X, RES_Y);
    SetDefDrawEnv(&draw[1], 0, 0, RES_X, RES_Y);
    
    // Specifies the clear color of the DRAWENV
    setRGB0(&draw[0], 63, 0, 127);
    setRGB0(&draw[1], 63, 0, 127);

    // Enable background clear
    draw[0].isbg = 1;
    draw[1].isbg = 1;

    //Set to drawbuffer 0
    drawbuffer = 0;
    next_primitive = primitive_buffer[0];

    // Clear ordering tables
    ClearOTagR(ord_tbl[0], ORD_TBL_LENGTH);
    ClearOTagR(ord_tbl[1], ORD_TBL_LENGTH);

    // Initialize GTE
    InitGeom();

    // Set up where we want the center of the screen to be
    gte_SetGeomOffset(CENTER_X, CENTER_Y);

    // Set screen depth (which according to example code is kinda like FOV apparently)
    gte_SetGeomScreen(120);

    // Set screen side clip
	setRECT( &screen_clip, 0, 0, RES_X, RES_Y);
    next_primitive = primitive_buffer[0];
}

#define CLIP_LEFT	1
#define CLIP_RIGHT	2
#define CLIP_TOP	4
#define CLIP_BOTTOM	8

short test_clip(const RECT *clip, const short x, const short y) {
	
	// Tests which corners of the screen a point lies outside of
	
	short result = 0;

	if ( x < clip->x ) {
		result |= CLIP_LEFT;
	}
	
	if ( x >= (clip->x+(clip->w-1)) ) {
		result |= CLIP_RIGHT;
	}
	
	if ( y < clip->y ) {
		result |= CLIP_TOP;
	}
	
	if ( y >= (clip->y+(clip->h-1)) ) {
		result |= CLIP_BOTTOM;
	}

	return result;
	
}

int tri_clip(const RECT *clip, const DVECTOR *v0, const DVECTOR *v1, const DVECTOR *v2) {
	
	// Returns non-zero if a triangle is outside the screen boundaries
	
	short c[3];

	c[0] = test_clip(clip, v0->vx, v0->vy);
	c[1] = test_clip(clip, v1->vx, v1->vy);
	c[2] = test_clip(clip, v2->vx, v2->vy);

	if ( ( c[0] & c[1] ) == 0 )
		return 0;
	if ( ( c[1] & c[2] ) == 0 )
		return 0;
	if ( ( c[2] & c[0] ) == 0 )
		return 0;

	return 1;
}

void renderer_begin_frame(transform_t* camera_transform) {
    // Apply camera transform
    HiRotMatrix(&camera_transform->rotation, &view_matrix);
    VECTOR position = camera_transform->position;
    position.vx = -position.vx >> 12;
    position.vy = -position.vy >> 12;
    position.vz = -position.vz >> 12;
    ApplyMatrixLV(&view_matrix, &position, &position);
    TransMatrix(&view_matrix, &position);
    gte_SetRotMatrix(&view_matrix);
    gte_SetTransMatrix(&view_matrix);
    
    n_rendered_triangles = 0;
    n_total_triangles = 0;
}

void draw_triangle_shaded(vertex_3d_t v0, vertex_3d_t v1, vertex_3d_t v2, uint8_t tex_id, uint16_t tex_offset_x, int p) {
    // Create primitive
    POLY_GT3* new_triangle = (POLY_GT3*)next_primitive;
    next_primitive += sizeof(POLY_GT3);

    // Set the vertex positions of the triangle
    setXY3(
        new_triangle,
        v0.x, v0.y,
        v1.x, v1.y,
        v2.x, v2.y
    );
    if (tri_clip(&screen_clip,
        (DVECTOR*)&new_triangle->x0, (DVECTOR*)&new_triangle->x1, (DVECTOR*)&new_triangle->x2))
        return;

    // Set the vertex colors of the triangle
    setRGB0(new_triangle,
        v0.r,
        v0.g,
        v0.b
    );
    setRGB1(new_triangle,
        v1.r,
        v1.g,
        v1.b
    );
    setRGB2(new_triangle,
        v2.r,
        v2.g,
        v2.b
    );

    // Bind texture
    new_triangle->clut = getClut(palettes[tex_id].x, palettes[tex_id].y);
    new_triangle->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y);

    // Transform UVs to texture space
    setUV3(new_triangle,
        (((uint16_t)(v0.u) * textures[tex_id].w) >> 6) + tex_offset_x, // 6 because UVs do happen to be 4-bit pixel coordinates instead of 16-bit
        (((uint16_t)(v0.v) * textures[tex_id].h) >> 8),
        (((uint16_t)(v1.u) * textures[tex_id].w) >> 6) + tex_offset_x, // also + offset because apparently texture pages are 256x256 in 4-bit
        (((uint16_t)(v1.v) * textures[tex_id].h) >> 8),
        (((uint16_t)(v2.u) * textures[tex_id].w) >> 6) + tex_offset_x,
        (((uint16_t)(v2.v) * textures[tex_id].h) >> 8)
    );

    // Initialize the entry in the render queue
    setPolyGT3(new_triangle);

    // Add the triangle to the draw queue
    addPrim(ord_tbl[drawbuffer] + (p >> 2), new_triangle);
    ++n_rendered_triangles;
}

vertex_3d_t get_halfway_point(const vertex_3d_t v0, const vertex_3d_t v1) {
    const vertex_3d_t return_value = {
        .x = v0.x + ((v1.x - v0.x) >> 1),
        .y = v0.y + ((v1.y - v0.y) >> 1),
        .z = v0.z + ((v1.z - v0.z) >> 1),
        .r = v0.r + ((v1.r - v0.r) >> 1),
        .g = v0.g + ((v1.g - v0.g) >> 1),
        .b = v0.b + ((v1.b - v0.b) >> 1),
        .u = v0.u + ((v1.u - v0.u) >> 1),
        .v = v0.v + ((v1.v - v0.v) >> 1),
    };
    return return_value;
}

void renderer_draw_mesh_shaded(const mesh_t* mesh, transform_t* model_transform) {
    if (!mesh) {
        printf("renderer_draw_mesh_shaded: mesh was null!\n");
        return;
    }

    // Set rotation and translation matrix
    MATRIX model_matrix;
    HiRotMatrix(&model_transform->rotation, &model_matrix);
    TransMatrix(&model_matrix, &model_transform->position);
    CompMatrixLV(&view_matrix, &model_matrix, &model_matrix);

    // Send it to the GTE
	PushMatrix();
    gte_SetRotMatrix(&model_matrix);
    gte_SetTransMatrix(&model_matrix);

    n_total_triangles += mesh->n_vertices / 3;
    const uint8_t tex_id = mesh->vertices[0].tex_id;
    const uint16_t tex_offset_x = (tex_id % 4) * 64;

    // Loop over each triangle
    for (size_t i = 0; i < mesh->n_vertices; i += 3) {
        // Load a triangle's vertices into the GTE
        gte_ldv3(
            &mesh->vertices[i + 0].x,
            &mesh->vertices[i + 1].x,
            &mesh->vertices[i + 2].x
        );

        // Apply transformations
        gte_rtpt();

        // Calculate normal clip for backface culling
        int p;
        gte_nclip();
        gte_stopz(&p);

        // If this is the back of the triangle, cull it
        if (p <= 0) 
            continue;

        // Calculate average depth of the triangle
        gte_avsz3();
        gte_stotz(&p);

        // Depth clipping
        if ((p>>2) > ORD_TBL_LENGTH || ((p >> 2) <= 0))
            continue;

        // So when triangles are very close to the camera, they tend to distort a lot because lmao PS1 texture mapping
        // Let's subdivide them
        if (p < 0) {
            // Subdivide edges
            vertex_3d_t v0 = mesh->vertices[i + 0];
            vertex_3d_t v1 = mesh->vertices[i + 1];
            vertex_3d_t v2 = mesh->vertices[i + 2];
            vertex_3d_t v01 = get_halfway_point(v0, v1);
            vertex_3d_t v12 = get_halfway_point(v1, v2);
            vertex_3d_t v20 = get_halfway_point(v2, v0);

            // So the GTE still has the 3 original vertices, let's store those into the original triangles
            gte_stsxy0(&v0.x);
            gte_stsxy1(&v1.x);
            gte_stsxy2(&v2.x);

            // Now transform the edge middles
            gte_ldv3(
                &v01.x,
                &v12.x,
                &v20.x
            );
            gte_rtpt();
            gte_stsxy0(&v01.x);
            gte_stsxy1(&v12.x);
            gte_stsxy2(&v20.x);
            
            draw_triangle_shaded(v0, v01, v20, tex_id, tex_offset_x, p);
            draw_triangle_shaded(v01, v1, v12, tex_id, tex_offset_x, p);
            draw_triangle_shaded(v20, v12, v2, tex_id, tex_offset_x, p);
            draw_triangle_shaded(v20, v01, v12, tex_id, tex_offset_x, p);


            //draw_triangle_shaded(&mesh->vertices[i], tex_id, tex_offset_x, p);
        }
        else if (p < 320) {
            // So the GTE still has the 3 original vertices, let's store those into the original triangles
            vertex_3d_t v0 = mesh->vertices[i + 0];
            vertex_3d_t v1 = mesh->vertices[i + 1];
            vertex_3d_t v2 = mesh->vertices[i + 2];
            gte_stsxy0(&v0.x);
            gte_stsxy1(&v1.x);
            gte_stsxy2(&v2.x);
            draw_triangle_shaded(v0, v1, v2, tex_id, tex_offset_x, p);
        }
        // Otherwise, draw as untextured triangle
        else {
            // Create primitive
            POLY_G3* new_triangle = (POLY_G3*)next_primitive;
            next_primitive += sizeof(POLY_G3);

            // Set the vertex positions of the triangle
            gte_stsxy0(&new_triangle->x0);
            gte_stsxy1(&new_triangle->x1);
            gte_stsxy2(&new_triangle->x2);
            if (tri_clip(&screen_clip,
                (DVECTOR*)&new_triangle->x0, (DVECTOR*)&new_triangle->x1, (DVECTOR*)&new_triangle->x2))
                continue;

            // Calculate vertex colors of the triangle - multiply the vertex colors by the average color of the texture
            // so that from a distance it looks close enough to the texture
            pixel32_t vert_colors[3];
            for (size_t ci = 0; ci < 3; ++ci) {
                const uint16_t r = ((((uint16_t)mesh->vertices[i + ci].r) * ((uint16_t)textures_avg_colors[tex_id].r)) >> 7);
                const uint16_t g = ((((uint16_t)mesh->vertices[i + ci].g) * ((uint16_t)textures_avg_colors[tex_id].g)) >> 7);
                const uint16_t b = ((((uint16_t)mesh->vertices[i + ci].b) * ((uint16_t)textures_avg_colors[tex_id].b)) >> 7);
                vert_colors[ci].r = (r > 255) ? 255 : r;
                vert_colors[ci].g = (g > 255) ? 255 : g;
                vert_colors[ci].b = (b > 255) ? 255 : b;
            }

            // Set the vertex colors of the triangle
            setRGB0(new_triangle,
                vert_colors[0].r,
                vert_colors[0].g,
                vert_colors[0].b
            );
            setRGB1(new_triangle,
                vert_colors[1].r,
                vert_colors[1].g,
                vert_colors[1].b
            );
            setRGB2(new_triangle,
                vert_colors[2].r,
                vert_colors[2].g,
                vert_colors[2].b
            );

            // Initialize the entry in the render queue
            setPolyG3(new_triangle);

            // Add the triangle to the draw queue
            addPrim(ord_tbl[drawbuffer] + (p >> 2), new_triangle);
            ++n_rendered_triangles;
        }
    }

	PopMatrix();

    // Set texture page
}

void renderer_draw_model_shaded(const model_t* model, transform_t* model_transform) {
    for (size_t i = 0; i < model->n_meshes; ++i) {
        renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
    }
}

void renderer_draw_triangles_shaded_2d(const vertex_2d_t* vertex_buffer, const uint16_t n_verts, const int16_t x, const int16_t y) {
    // Loop over each triangle
    for (size_t i = 0; i < n_verts; i += 3) {
        // Allocate a triangle in the render queue
        POLY_G3* new_triangle = (POLY_G3*)next_primitive;
        next_primitive += sizeof(POLY_G3);

        // Initialize the entry in the render queue
        setPolyG3(new_triangle);

        // Set the vertex positions of the triangle
        setXY3(new_triangle, 
            vertex_buffer[i + 0].x + x, vertex_buffer[i + 0].y + y,
            vertex_buffer[i + 1].x + x, vertex_buffer[i + 1].y + y,
            vertex_buffer[i + 2].x + x, vertex_buffer[i + 2].y + y
        );

        // Set the vertex colors of the triangle
        setRGB0(new_triangle,
            vertex_buffer[i + 0].r,
            vertex_buffer[i + 0].g,
            vertex_buffer[i + 0].b
        );
        setRGB1(new_triangle,
            vertex_buffer[i + 1].r,
            vertex_buffer[i + 1].g,
            vertex_buffer[i + 1].b
        );
        setRGB2(new_triangle,
            vertex_buffer[i + 2].r,
            vertex_buffer[i + 2].g,
            vertex_buffer[i + 2].b
        );

        // Add the triangle to the draw queue
        addPrim(ord_tbl[drawbuffer], new_triangle);
    }
}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, const pixel32_t color, transform_t* model_transform) {
    // Set rotation and translation matrix
    MATRIX model_matrix;
    HiRotMatrix(&model_transform->rotation, &model_matrix);
    TransMatrix(&model_matrix, &model_transform->position);
    CompMatrixLV(&view_matrix, &model_matrix, &model_matrix);

    // Send it to the GTE
    PushMatrix();
    gte_SetRotMatrix(&model_matrix);
    gte_SetTransMatrix(&model_matrix);

    // Load a line's vertices into the GTE
    SVECTOR sv0 = { v0.x.integer, v0.y.integer, v0.z.integer };
    SVECTOR sv1 = { v1.x.integer, v1.y.integer, v1.z.integer };
    gte_ldv3(
        &sv0.vx,
        &sv1.vx,
        &sv1.vx
        );

    // Apply transformations
    gte_rtpt();

    // Calculate average depth of the line
    int p = 4;

    // Depth clipping
    if ((p >> 2) > ORD_TBL_LENGTH || ((p >> 2) <= 0))
        return;

    LINE_F2* new_line = (LINE_F2*)next_primitive;
    next_primitive += sizeof(LINE_F2);

    // Set the vertex positions of the line
    gte_stsxy0(&new_line->x0);
    gte_stsxy1(&new_line->x1);

    // Set the vertex colors of the line
    setRGB0(new_line,
        color.r,
        color.g,
        color.b
    );

    // Initialize the entry in the render queue
    setLineF2(new_line);

    // Add the line to the draw queue
    addPrim(ord_tbl[drawbuffer] + (p >> 2), new_line);
}

void renderer_debug_draw_aabb(const aabb_t* box, const pixel32_t color, transform_t* model_transform) {
    // Create 8 vertices
    vec3_t vertex000 = { box->min.x, box->min.y, box->min.z };
    vec3_t vertex001 = { box->min.x, box->min.y, box->max.z };
    vec3_t vertex010 = { box->min.x, box->max.y, box->min.z };
    vec3_t vertex011 = { box->min.x, box->max.y, box->max.z };
    vec3_t vertex100 = { box->max.x, box->min.y, box->min.z };
    vec3_t vertex101 = { box->max.x, box->min.y, box->max.z };
    vec3_t vertex110 = { box->max.x, box->max.y, box->min.z };
    vec3_t vertex111 = { box->max.x, box->max.y, box->max.z };

    // Draw the lines
    renderer_debug_draw_line(vertex000, vertex100, color, model_transform);
    renderer_debug_draw_line(vertex100, vertex101, color, model_transform);
    renderer_debug_draw_line(vertex101, vertex001, color, model_transform);
    renderer_debug_draw_line(vertex001, vertex000, color, model_transform);
    renderer_debug_draw_line(vertex010, vertex110, color, model_transform);
    renderer_debug_draw_line(vertex110, vertex111, color, model_transform);
    renderer_debug_draw_line(vertex111, vertex011, color, model_transform);
    renderer_debug_draw_line(vertex011, vertex010, color, model_transform);
    renderer_debug_draw_line(vertex000, vertex010, color, model_transform);
    renderer_debug_draw_line(vertex100, vertex110, color, model_transform);
    renderer_debug_draw_line(vertex101, vertex111, color, model_transform);
    renderer_debug_draw_line(vertex001, vertex011, color, model_transform);
}

void renderer_upload_texture(const texture_cpu_t* texture, const uint8_t index) {
    // Load texture pixels to VRAM - starting from 320,0, spanning 512x512 VRAM pixels, stored in 16x64 blocks (for 64x64 texture)
    const RECT rect_tex = {
        320 + ((int16_t)index) * 16,
        0 + (((int16_t)index) / 128),
        (int16_t)texture->width / 4,
        (int16_t)texture->height
    };
    LoadImage(&rect_tex, (uint32_t*)texture->data);
    DrawSync(0);
    textures[index] = rect_tex;
    textures_avg_colors[index] = texture->avg_color;

    // Load palette to VRAM - starting from 832,0, spanning 128x32 VRAM pixels, stored in 16x1 blocks (for 16-bit 16-color palettes)
    RECT rect_palette = {
        832 + ((int16_t)index) * 16,
        0 + (((int16_t)index) / 8),
        16,
        1
    };
    LoadImage(&rect_palette, (uint32_t*)texture->palette);
    DrawSync(0);
    palettes[index] = rect_palette;
}

void renderer_end_frame(void) {
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

    // Set the next primitive to draw to be the first primitive in the buffer
    next_primitive = primitive_buffer[drawbuffer];

    // Clear render queue
    ClearOTagR(ord_tbl[drawbuffer], ORD_TBL_LENGTH);
    
    // Apply Envs
    PutDispEnv(&disp[drawbuffer]);
    PutDrawEnv(&draw[drawbuffer]);
    
    // Enable display
    SetDispMask(1);
    
    // Draw Ordering Table
    DrawOTag(ord_tbl[1-drawbuffer] + ORD_TBL_LENGTH - 1);
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
    int dt_ms;
    if (vsync_enable) {
#ifdef PAL
        dt_ms = 20 * dt_raw;
#else
        dt_ms = (16666 * dt_raw) / 1000;
#endif
    }
    else {
        dt_ms = (1000 * dt_raw) / 15625; // Somehow this works for both PAL and NTSC
    }
    if (dt_ms == 0) {
        dt_ms = 1;
    }
    return dt_ms;
}

uint32_t renderer_get_n_rendered_triangles(void) {
    return n_rendered_triangles;
}

uint32_t renderer_get_n_total_triangles(void) {
    return n_total_triangles;
}

int renderer_should_close() {
    return 0;
}

vec3_t renderer_get_forward_vector() {
    vec3_t result;
    result.x.raw = (int32_t)view_matrix.m[2][0] >> 4;
    result.y.raw = (int32_t)view_matrix.m[2][1] >> 4;
    result.z.raw = (int32_t)view_matrix.m[2][2] >> 4;
    return result;
}

#endif
