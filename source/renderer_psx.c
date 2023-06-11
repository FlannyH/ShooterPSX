#ifdef _PSX
#include "renderer.h"
#include <stdio.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <inline_c.h>
#include <string.h>
#include <psxpad.h>
#include "input.h"
#include "vec2.h"

#define TRI_THRESHOLD_SUB2 125
#define TRI_THRESHOLD_SUB1 300
#define TRI_THRESHOLD_NORMAL 500
#define TRI_THRESHOLD_FADE_START 800
#define TRI_THRESHOLD_FADE_END 1200
#define N_CLUT_FADES 16

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
vec3_t camera_pos;
vec3_t camera_dir;
int vsync_enable = 0;
int frame_counter = 0;

pixel32_t textures_avg_colors[256];
RECT textures[256];
RECT palettes[256];

vertex_3d_t get_halfway_point(const vertex_3d_t v0, const vertex_3d_t v1) {
    return (vertex_3d_t) {
        .x = v0.x + ((v1.x - v0.x) >> 1),
        .y = v0.y + ((v1.y - v0.y) >> 1),
        .z = v0.z + ((v1.z - v0.z) >> 1),
        .r = v0.r + ((v1.r - v0.r) >> 1),
        .g = v0.g + ((v1.g - v0.g) >> 1),
        .b = v0.b + ((v1.b - v0.b) >> 1),
        .u = v0.u + ((v1.u - v0.u) >> 1),
        .v = v0.v + ((v1.v - v0.v) >> 1),
    };
}

void renderer_init(void) {
    // Reset GPU and enable interrupts
    ResetGraph(0);

#ifdef PAL
    SetVideoMode(MODE_PAL);
#else
    SetVideoMode(MODE_NTSC);
#endif

    // Configures the pair of DISPENVs
    SetDefDispEnv(&disp[0], 0, 0, RES_X, RES_Y);
    SetDefDispEnv(&disp[1], 512, 0, RES_X, RES_Y);


    // Configures the pair of DRAWENVs for the DISPENVs
    SetDefDrawEnv(&draw[0], 512, 0, RES_X, RES_Y);
    SetDefDrawEnv(&draw[1], 0, 0, RES_X, RES_Y);
    
    // Specifies the clear color of the DRAWENV
    setRGB0(&draw[0], 16, 16, 20);
    setRGB0(&draw[1], 16, 16, 20);

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

    next_primitive = primitive_buffer[0];
}

static int mul = 0;
void renderer_begin_frame(transform_t* camera_transform) {
    mul ^= 1;
    // Apply camera transform
    static int scalex = 512;

    if (input_held(PAD_LEFT, 0)) {
        scalex -= 4;
    }
    if (input_held(PAD_RIGHT, 0)) {
        scalex += 4;
    }
    HiRotMatrix(&camera_transform->rotation, &view_matrix);
    //ScaleMatrix(&view_matrix, &scale);
    VECTOR position = camera_transform->position;
    memcpy(&camera_pos, &camera_transform->position, sizeof(camera_pos));
    //memcpy(&camera_dir, &camera_transform->rotation, sizeof(camera_dir));
    position.vx = -position.vx >> 12;
    position.vy = -position.vy >> 12;
    position.vz = -position.vz >> 12;
    ApplyMatrixLV(&view_matrix, &position, &position);
    TransMatrix(&view_matrix, &position);
    // Finishing touch: scale by aspect ratio
    MATRIX aspect_matrix = {
        .m = {
            {(ONE * RES_X) / (widescreen ? 427 : 320), 0, 0},
            {0, ONE, 0},
            {0, 0, ONE},
        },
        .t = {0, 0, 0},
    };
    CompMatrixLV(&aspect_matrix, &view_matrix, &view_matrix);
    gte_SetRotMatrix(&view_matrix);
    gte_SetTransMatrix(&view_matrix);
	//printf("RIGHT: %i, %i, %i\n", view_matrix.m[0][0], view_matrix.m[1][0], view_matrix.m[2][0]);
	//printf("UP:    %i, %i, %i\n", view_matrix.m[0][1], view_matrix.m[1][1], view_matrix.m[2][1]);
	//printf("FRONT: %i, %i, %i\n", view_matrix.m[0][2], view_matrix.m[1][2], view_matrix.m[2][2]);
	camera_dir.x = view_matrix.m[2][0];
	camera_dir.y = view_matrix.m[2][1];
	camera_dir.z = view_matrix.m[2][2];
    
    
    n_rendered_triangles = 0;
    n_total_triangles = 0;
}

// For some reason, making this a macro improved performance by rough 8%. Can't tell you why though.
#define ADD_TEX_TRI_TO_QUEUE(p0, p1, p2, v0, v1, v2, avg_z, clut_fade, tex_id) {       \
    POLY_GT3* new_triangle = (POLY_GT3*)next_primitive;\
    next_primitive += sizeof(POLY_GT3);\
    setPolyGT3(new_triangle); \
    setXY3(new_triangle, \
        p0.x, p0.y,\
        p1.x, p1.y,\
        p2.x, p2.y\
    );\
    setRGB0(new_triangle, v0.r >> 1, v0.g >> 1, v0.b >> 1);\
    setRGB1(new_triangle, v1.r >> 1, v1.g >> 1, v1.b >> 1);\
    setRGB2(new_triangle, v2.r >> 1, v2.g >> 1, v2.b >> 1);\
    const uint16_t tex_offset_x = (tex_id % 4) * 64;\
    new_triangle->clut = getClut(palettes[tex_id].x, palettes[tex_id].y + clut_fade);\
    new_triangle->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y);\
    setUV3(new_triangle,\
        (((uint16_t)(v0.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v0.v)) >> 2),\
        (((uint16_t)(v1.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v1.v)) >> 2),\
        (((uint16_t)(v2.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v2.v)) >> 2)\
    );\
    addPrim(ord_tbl[drawbuffer] + (avg_z>>2), new_triangle);\
    ++n_rendered_triangles;\
}\

// Same as above but for quads
#define ADD_TEX_QUAD_TO_QUEUE(p0, p1, p2, p3, v0, v1, v2, v3, avg_z, clut_fade, tex_id) {       \
    POLY_GT4* new_triangle = (POLY_GT4*)next_primitive;\
    next_primitive += sizeof(POLY_GT4);\
    setPolyGT4(new_triangle); \
    setXY4(new_triangle, \
        p0.x, p0.y,\
        p1.x, p1.y,\
        p2.x, p2.y,\
        p3.x, p3.y\
    );\
    setRGB0(new_triangle, v0.r >> 1, v0.g >> 1, v0.b >> 1);\
    setRGB1(new_triangle, v1.r >> 1, v1.g >> 1, v1.b >> 1);\
    setRGB2(new_triangle, v2.r >> 1, v2.g >> 1, v2.b >> 1);\
    setRGB3(new_triangle, v3.r >> 1, v3.g >> 1, v3.b >> 1);\
    const uint16_t tex_offset_x = (tex_id % 4) * 64;\
    new_triangle->clut = getClut(palettes[tex_id].x, palettes[tex_id].y + clut_fade);\
    new_triangle->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y);\
    setUV4(new_triangle,\
        (((uint16_t)(v0.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v0.v)) >> 2),\
        (((uint16_t)(v1.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v1.v)) >> 2),\
        (((uint16_t)(v2.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v2.v)) >> 2),\
        (((uint16_t)(v3.u)) >> 2) + tex_offset_x,\
        (((uint16_t)(v3.v)) >> 2)\
    );\
    addPrim(ord_tbl[drawbuffer] + (avg_z>>2), new_triangle);\
    ++n_rendered_triangles;\
}\

__attribute__((always_inline)) inline void draw_triangle_shaded(vertex_3d_t* verts) {
    // Transform the triangle vertices - there's 6 entries instead of 3, so we have enough room for subdivided triangles
    svec2_t trans_vec_xy[6];
    scalar_t trans_vec_z[6];
    scalar_t avg_z;
    gte_ldv3(
        &verts[0],
        &verts[1],
        &verts[2]
    );
    gte_rtpt();

    // Backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);
    if (p <= 0) return;
    (void)p; // we don't need it anymore after this

    // Store transformed position, and center depth
    gte_stsxy3c(&trans_vec_xy[0]);
    gte_stsz3c(&trans_vec_z[0]);
    gte_avsz3();
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z>>2) > ORD_TBL_LENGTH || ((avg_z >> 2) <= 0)) return;

    if (avg_z < TRI_THRESHOLD_SUB1) {
        // Let's calculate the center of each edge
        const vertex_3d_t ab = get_halfway_point(verts[0], verts[1]);
        const vertex_3d_t bc = get_halfway_point(verts[1], verts[2]);
        const vertex_3d_t ca = get_halfway_point(verts[2], verts[0]);

        // Transform them
        gte_ldv3(&ab.x, &bc.x, &ca.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[3]);
        gte_stsz3c(&trans_vec_z[3]);

        // Draw them
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[3], trans_vec_xy[5], verts[0], ab, ca, avg_z, 0, verts[0].tex_id);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[1], trans_vec_xy[4], ab, verts[1], bc, avg_z, 0, verts[0].tex_id);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[4], trans_vec_xy[5], ab, bc, ca, avg_z, 0, verts[0].tex_id);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[5], trans_vec_xy[4], trans_vec_xy[2], ca, bc, verts[2], avg_z, 0, verts[0].tex_id);
        return;
    }
    if (avg_z < TRI_THRESHOLD_FADE_END) {
        int16_t clut_fade = 0;
        if (avg_z >= TRI_THRESHOLD_FADE_START) {
            clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
        }
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], verts[0], verts[1], verts[2], avg_z, clut_fade, verts[0].tex_id);
        return;
    }

    else {
        // Create primitive
        POLY_G3* new_triangle = (POLY_G3*)next_primitive;
        next_primitive += sizeof(POLY_G3);

        // Initialize the entry in the render queue
        setPolyG3(new_triangle);

        // Set the vertex positions of the triangle
        setXY3(new_triangle, 
            trans_vec_xy[0].x, trans_vec_xy[0].y,
            trans_vec_xy[1].x, trans_vec_xy[1].y,
            trans_vec_xy[2].x, trans_vec_xy[2].y
        );

        pixel32_t vert_colors[3];
        for (size_t ci = 0; ci < 3; ++ci) {
            const uint16_t r = ((((uint16_t)verts[ci].r) * ((uint16_t)textures_avg_colors[verts->tex_id].r)) >> 8);
            const uint16_t g = ((((uint16_t)verts[ci].g) * ((uint16_t)textures_avg_colors[verts->tex_id].g)) >> 8);
            const uint16_t b = ((((uint16_t)verts[ci].b) * ((uint16_t)textures_avg_colors[verts->tex_id].b)) >> 8);
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
        
        // Add the triangle to the draw queue
        addPrim(ord_tbl[drawbuffer] + (avg_z>>2), new_triangle);
        ++n_rendered_triangles;
        return;
    }
}
__attribute__((always_inline)) inline void draw_quad_shaded(vertex_3d_t* verts) {
    // Transform the quad vertices, with enough entries in the array for subdivided quads
    svec2_t trans_vec_xy[10];
    scalar_t trans_vec_z[10];
    scalar_t avg_z;
    gte_ldv3(
        &verts[0],
        &verts[1],
        &verts[2]
    );
    gte_rtpt();

    // Backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);
    if (p <= 0) return;

    // Store transformed position, and center depth
    gte_stsxy3c(&trans_vec_xy[0]);
    gte_stsz3c(&trans_vec_z[0]);
    gte_ldv0(&verts[3]);
    gte_rtps();
    gte_stsxy(&trans_vec_xy[3]);
    gte_stsz(&trans_vec_z[3]);
    gte_avsz4();
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z>>2) > ORD_TBL_LENGTH || ((avg_z >> 2) <= 0)) return;

    if (avg_z < TRI_THRESHOLD_SUB1) {
        // Let's calculate the new points we need
        const vertex_3d_t ab = get_halfway_point(verts[0], verts[1]);
        const vertex_3d_t bc = get_halfway_point(verts[1], verts[3]);
        const vertex_3d_t cd = get_halfway_point(verts[3], verts[2]);
        const vertex_3d_t da = get_halfway_point(verts[2], verts[0]);
        const vertex_3d_t center = get_halfway_point(ab, cd);
        // Transform them
        gte_ldv3(&ab.x, &bc.x, &cd.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[4]);
        gte_stsz3c(&trans_vec_z[4]);
        gte_ldv01(&da.x, &center.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[7]);
        gte_stsz3c(&trans_vec_z[7]);

        // Draw them
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[4], trans_vec_xy[7], trans_vec_xy[8], verts[0], ab, da, center, avg_z, 0, verts[0].tex_id);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[4], trans_vec_xy[1], trans_vec_xy[8], trans_vec_xy[5], ab, verts[1], center, bc, avg_z, 0, verts[0].tex_id);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[7], trans_vec_xy[8], trans_vec_xy[2], trans_vec_xy[6], da, center, verts[2], cd, avg_z, 0, verts[0].tex_id);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[8], trans_vec_xy[5], trans_vec_xy[6], trans_vec_xy[3], center, bc, cd, verts[3], avg_z, 0, verts[0].tex_id);
        return;
    }
    if (avg_z < TRI_THRESHOLD_FADE_END) {
        int16_t clut_fade = 0;
        if (avg_z >= TRI_THRESHOLD_FADE_START) {
            clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
        }
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[3], verts[0], verts[1], verts[2], verts[3], avg_z, clut_fade, verts[0].tex_id);
        return;
    }

    else {
        // Create primitive
        POLY_G4* new_quad = (POLY_G4*)next_primitive;
        next_primitive += sizeof(POLY_G4);

        // Initialize the entry in the render queue
        setPolyG4(new_quad);

        // Set the vertex positions of the triangle
        setXY4(new_quad, 
            trans_vec_xy[0].x, trans_vec_xy[0].y,
            trans_vec_xy[1].x, trans_vec_xy[1].y,
            trans_vec_xy[2].x, trans_vec_xy[2].y,
            trans_vec_xy[3].x, trans_vec_xy[3].y
        );

        pixel32_t vert_colors[4];
        for (size_t ci = 0; ci < 4; ++ci) {
            const uint16_t r = ((((uint16_t)verts[ci].r) * ((uint16_t)textures_avg_colors[verts->tex_id].r)) >> 8);
            const uint16_t g = ((((uint16_t)verts[ci].g) * ((uint16_t)textures_avg_colors[verts->tex_id].g)) >> 8);
            const uint16_t b = ((((uint16_t)verts[ci].b) * ((uint16_t)textures_avg_colors[verts->tex_id].b)) >> 8);
            vert_colors[ci].r = (r > 255) ? 255 : r;
            vert_colors[ci].g = (g > 255) ? 255 : g;
            vert_colors[ci].b = (b > 255) ? 255 : b;
        }
        // Set the vertex colors of the triangle
        setRGB0(new_quad,
            vert_colors[0].r,
            vert_colors[0].g,
            vert_colors[0].b
        );
        setRGB1(new_quad,
            vert_colors[1].r,
            vert_colors[1].g,
            vert_colors[1].b
        );
        setRGB2(new_quad,
            vert_colors[2].r,
            vert_colors[2].g,
            vert_colors[2].b
        );
        setRGB3(new_quad,
            vert_colors[3].r,
            vert_colors[3].g,
            vert_colors[3].b
        );
        
        // Add the triangle to the draw queue
        addPrim(ord_tbl[drawbuffer] + (avg_z>>2), new_quad);
        ++n_rendered_triangles;
        return;
    }
}

__attribute__((always_inline)) inline void draw_triangle_shaded_untextured(vertex_3d_t* verts, uint8_t tex_id) {
    // Load triangle into GTE
    gte_ldv3(
        &verts[0],
        &verts[1],
        &verts[2]
    );

    // Apply transformations
    gte_rtpt();

    // Calculate normal clip for backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);

    // If this is the back of the triangle, cull it
    if (p >= 0)
        return;

    // Calculate average depth of the triangle
    gte_avsz3();
    gte_stotz(&p);

    // Depth clipping
    if ((p >> 2) > ORD_TBL_LENGTH || ((p >> 2) <= 0))
        return;

    // Store them
    int16_t vertex_positions[6];
    gte_stsxy0(&vertex_positions[0]);
    gte_stsxy1(&vertex_positions[2]);
    gte_stsxy2(&vertex_positions[4]);

    // Create primitive
    POLY_G3* new_triangle = (POLY_G3*)next_primitive;
    next_primitive += sizeof(POLY_G3);

    // Set the vertex positions of the triangle
    setXY3(
        new_triangle,
        vertex_positions[0], vertex_positions[1],
        vertex_positions[2], vertex_positions[3],
        vertex_positions[4], vertex_positions[5]
    );

    // Calculate vertex colors of the triangle - multiply the vertex colors by the average color of the texture
    // so that from a distance it looks close enough to the texture
    pixel32_t vert_colors[3];
    for (size_t ci = 0; ci < 3; ++ci) {
        const uint16_t r = ((((uint16_t)verts[ci].r) * ((uint16_t)textures_avg_colors[tex_id].r)) >> 7);
        const uint16_t g = ((((uint16_t)verts[ci].g) * ((uint16_t)textures_avg_colors[tex_id].g)) >> 7);
        const uint16_t b = ((((uint16_t)verts[ci].b) * ((uint16_t)textures_avg_colors[tex_id].b)) >> 7);
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

	// If the mesh's bounding box is not inside the viewing frustum, cull it
    if (1)
    {
        // Create shorthand for better readability
        const vec3_t *min = &mesh->bounds.min; // shorthand for readability
        const vec3_t *max = &mesh->bounds.max; // shorthand for readability

        // We will transform these to screen space
        SVECTOR vertices[8] = {
            ((SVECTOR) {min->x, min->y, min->z}),
            ((SVECTOR) {min->x, min->y, max->z}),
            ((SVECTOR) {min->x, max->y, min->z}),
            ((SVECTOR) {min->x, max->y, max->z}),
            ((SVECTOR) {max->x, min->y, min->z}),
            ((SVECTOR) {max->x, min->y, max->z}),
            ((SVECTOR) {max->x, max->y, min->z}),
            ((SVECTOR) {max->x, max->y, max->z}),
        };

        // Transform vertices and store them back
        int16_t padding_for_last_store;
        // 0, 1, 2
        gte_ldv3(&vertices[0].vx, &vertices[1].vx, &vertices[2].vx);
        gte_rtpt();
        gte_stsxy3(&vertices[0].vx, &vertices[1].vx, &vertices[2].vx);
        gte_stsz3(&vertices[0].vz, &vertices[1].vz, &vertices[2].vz);
        // 3, 4, 5
        gte_ldv3(&vertices[3].vx, &vertices[4].vx, &vertices[5].vx); // 3 4 5
        gte_rtpt();
        gte_stsxy3(&vertices[3].vx, &vertices[4].vx, &vertices[5].vx);
        gte_stsz3(&vertices[3].vz, &vertices[4].vz, &vertices[5].vz);
        // 6, 7
        gte_ldv01(&vertices[6].vx, &vertices[7].vx);
        gte_rtpt();
        gte_stsxy01(&vertices[6].vx, &vertices[7].vx);
        gte_stsz3(&vertices[6].vz, &vertices[7].vz, &padding_for_last_store);

        // Find screen aligned bounding box
        SVECTOR after_min = (SVECTOR){INT16_MAX, INT16_MAX, INT16_MAX};
        SVECTOR after_max = (SVECTOR){INT16_MIN, INT16_MIN, INT16_MIN};
        for (size_t i = 0; i < 8; ++i) {
            if      (vertices[i].vx < after_min.vx) after_min.vx = vertices[i].vx;
            else if (vertices[i].vx > after_max.vx) after_max.vx = vertices[i].vx;
            if      (vertices[i].vy > after_max.vy) after_max.vy = vertices[i].vy;
            else if (vertices[i].vy < after_min.vy) after_min.vy = vertices[i].vy;
            if      (vertices[i].vz > after_max.vz) after_max.vz = vertices[i].vz;
        }

        // If this screen aligned bounding box is off screen, do not draw this mesh
        #define FRUSCUL_PAD_X 0 // these are in case I ever want to add a safe zone
        #define FRUSCUL_PAD_Y 0 // or for debugging.
        if (after_max.vx < 0+FRUSCUL_PAD_X) return; // mesh is to the left of the screen
        if (after_max.vy < 0+FRUSCUL_PAD_Y) return; // mesh is above the screen
        if (after_max.vz < 0) return; // mesh is behind the screen
        if (after_min.vx > RES_X-FRUSCUL_PAD_X) return; // mesh is to the right of the screen
        if (after_min.vy > RES_Y-FRUSCUL_PAD_Y) return; // mesh is below the screen
        #undef FRUSCUL_PAD_X
        #undef FRUSCUL_PAD_Y
    };

    //n_total_triangles += mesh->n_vertices / 3;

    // Loop over each triangle
    size_t vert_idx = 0;
    for (size_t i = 0; i < mesh->n_triangles; ++i) {
        draw_triangle_shaded(&mesh->vertices[vert_idx]);
        vert_idx += 3;
    }
    for (size_t i = 0; i < mesh->n_quads; ++i) {
        //draw_triangle_shaded(&mesh->vertices[vert_idx]);
        //vertex_3d_t hack[3] = {mesh->vertices[vert_idx], mesh->vertices[vert_idx + 2], mesh->vertices[vert_idx + 3]};
        //draw_triangle_shaded(hack);
        draw_quad_shaded(&mesh->vertices[vert_idx]);
        vert_idx += 4;
    }

	PopMatrix();
}

void renderer_draw_model_shaded(const model_t* model, transform_t* model_transform) {
    for (size_t i = 0; i < model->n_meshes; ++i) {
        renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
    }
}

extern inline void renderer_draw_triangles_shaded_2d(const vertex_2d_t* vertex_buffer, const uint16_t n_verts, const int16_t x, const int16_t y) {
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
    SVECTOR sv0 = { v0.x, v0.y, v0.z };
    SVECTOR sv1 = { v1.x, v1.y, v1.z };
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
    const vec3_t vertex000 = { box->min.x, box->min.y, box->min.z };
    const vec3_t vertex001 = { box->min.x, box->min.y, box->max.z };
    const vec3_t vertex010 = { box->min.x, box->max.y, box->min.z };
    const vec3_t vertex011 = { box->min.x, box->max.y, box->max.z };
    const vec3_t vertex100 = { box->max.x, box->min.y, box->min.z };
    const vec3_t vertex101 = { box->max.x, box->min.y, box->max.z };
    const vec3_t vertex110 = { box->max.x, box->max.y, box->min.z };
    const vec3_t vertex111 = { box->max.x, box->max.y, box->max.z };

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

pixel16_t pixel_lerp(const uint16_t t_0_16, const pixel16_t a, const pixel16_t b) {
    return (pixel16_t) {
        .r = (((uint16_t)a.r * (16-t_0_16)) + ((uint16_t)b.r * t_0_16)) >> 4,
        .g = (((uint16_t)a.g * (16-t_0_16)) + ((uint16_t)b.g * t_0_16)) >> 4,
        .b = (((uint16_t)a.b * (16-t_0_16)) + ((uint16_t)b.b * t_0_16)) >> 4,
        .a = (((uint16_t)a.a * (16-t_0_16)) + ((uint16_t)b.a * t_0_16)) >> 4,
    };
}

// Blends a 16-color palette to a 16x16 color palette that fades all the colors to the texture average
void blend_palette(const pixel16_t* in_palette, pixel16_t* out_palette, const pixel16_t target_color) {
    // Loop over each color in the palette
    for (size_t x = 0; x < 16; ++x) {
        // Get the current color in the input palette
        const pixel16_t in_color = in_palette[x];

        // Iteratively blend
        const size_t w = 16;
        out_palette[x + w * 0x00] = in_color;
        out_palette[x + w * 0x01] = pixel_lerp(0x01, in_color, target_color);
        out_palette[x + w * 0x02] = pixel_lerp(0x02, in_color, target_color);
        out_palette[x + w * 0x03] = pixel_lerp(0x03, in_color, target_color);
        out_palette[x + w * 0x04] = pixel_lerp(0x04, in_color, target_color);
        out_palette[x + w * 0x05] = pixel_lerp(0x05, in_color, target_color);
        out_palette[x + w * 0x06] = pixel_lerp(0x06, in_color, target_color);
        out_palette[x + w * 0x07] = pixel_lerp(0x07, in_color, target_color);
        out_palette[x + w * 0x08] = pixel_lerp(0x08, in_color, target_color);
        out_palette[x + w * 0x09] = pixel_lerp(0x09, in_color, target_color);
        out_palette[x + w * 0x0A] = pixel_lerp(0x0A, in_color, target_color);
        out_palette[x + w * 0x0B] = pixel_lerp(0x0B, in_color, target_color);
        out_palette[x + w * 0x0C] = pixel_lerp(0x0C, in_color, target_color);
        out_palette[x + w * 0x0D] = pixel_lerp(0x0D, in_color, target_color);
        out_palette[x + w * 0x0E] = pixel_lerp(0x0E, in_color, target_color);
        out_palette[x + w * 0x0F] = pixel_lerp(0x0F, in_color, target_color);

    }
}

uint16_t find_distance_between_colors(const pixel16_t a, const pixel16_t b) {
    return(
        ((uint16_t)a.r * (uint16_t)b.r) +
        ((uint16_t)a.g * (uint16_t)b.g) +
        ((uint16_t)a.b * (uint16_t)b.b)
    );
}

void texture_64_to_32(const uint8_t* in_data, uint8_t* out_data, const pixel16_t* palette) {
    memset(out_data, 0x00, 32 * 64);
    for (size_t y = 0; y < 32; ++y)
    {
        for (size_t x = 0; x < 32; ++x)
        {
            // Sample 2x2 pixels, and blend the color
            const pixel16_t texsample1 = palette[in_data[((x * 2)) + ((y * 2) + 0) * 64] & 0x0F];
            const pixel16_t texsample2 = palette[in_data[((x * 2)) + ((y * 2) + 0) * 64] >> 4];
            const pixel16_t texsample3 = palette[in_data[((x * 2)) + ((y * 2) + 1) * 64] & 0x0F];
            const pixel16_t texsample4 = palette[in_data[((x * 2)) + ((y * 2) + 1) * 64] >> 4];
            const pixel16_t blended = {
                .r = ((uint8_t)texsample1.r + (uint8_t)texsample2.r + (uint8_t)texsample3.r + (uint8_t)texsample4.r) / 4,
                .g = ((uint8_t)texsample1.g + (uint8_t)texsample2.g + (uint8_t)texsample3.g + (uint8_t)texsample4.g) / 4,
                .b = ((uint8_t)texsample1.b + (uint8_t)texsample2.b + (uint8_t)texsample3.b + (uint8_t)texsample4.b) / 4,
                .a = ((uint8_t)texsample1.a + (uint8_t)texsample2.a + (uint8_t)texsample3.a + (uint8_t)texsample4.a) / 4
            };

            // Find the closest color in the palette to that
            uint16_t lowest_distance = 0xFFFF;
            uint8_t closest_value = 0;
            for (size_t i = 0; i < 16; ++i) {
                const uint16_t curr_distance = find_distance_between_colors(blended, palette[i]);
                if (curr_distance < lowest_distance) {
                    lowest_distance = curr_distance;
                    closest_value = i;
                }
            }

            // Write it to the texture
            if (x % 2 == 0) {
                out_data[(x >> 1) + y * 16] |= closest_value << 4;
            } else {
                out_data[(x >> 1) + y * 16] |= closest_value;
            }
        }
    }
}

void renderer_upload_texture(const texture_cpu_t* texture, const uint8_t index) {
    // Load texture pixels to VRAM - starting from 0,256, spanning 672x256 VRAM pixels, stored in 16x64 blocks (for 64x64 texture)
    // This means that the grid consists of 42x4 textures
    PANIC_IF("texture index >= 168", index >= 168);
    const RECT rect_tex = {
        0 + ((int16_t)index % 42) * 16,
        256 + (((int16_t)index) / 42) * 64,
        (int16_t)texture->width / 4,
        (int16_t)texture->height
    };
    LoadImage(&rect_tex, (uint32_t*)texture->data);
    DrawSync(0);
    textures[index] = rect_tex;
    textures_avg_colors[index] = texture->avg_color;

    // Load mipmap to VRAM - starting from 688, 256, spanning 336x128 VRAM pixels, stored in 8x32 blocks (for 32x32 textures)
    //uint8_t mip_data[8 * 32];
    //texture_64_to_32(texture->data, mip_data, texture->palette);
    //const RECT rect_mip = {
    //    688 + ((int16_t)index % 42) * 8,
    //    256 + (((int16_t)index) / 42) * 32,
    //    (int16_t)texture->width / 8,
    //    (int16_t)texture->height
    //};
    //LoadImage(&rect_mip, (uint32_t*)mip_data);
    //DrawSync(0);
    //textures[index] = rect_tex;

    // Load palette to VRAM - starting from 688,384, spanning 336x128 VRAM pixels, stored in 16x16 blocks (for 16-bit 16-color palettes, with fades to the average texture color for distance blur)
    const RECT rect_palette = {
        688 + ((int16_t)index % 21) * 16,
        384 + (((int16_t)index) / 21) * 16,
        16,
        16
    };
    //pixel16_t palette_buffer[336 * 128];
    //const pixel16_t target_color = {
    //    .r = texture->avg_color.r >> 3,
    //    .g = texture->avg_color.r >> 3,
    //    .b = texture->avg_color.r >> 3,
    //    .a = texture->avg_color.a >> 7,
    //};
    //blend_palette(texture->palette, palette_buffer, target_color);
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
    result.x = (int32_t)view_matrix.m[2][0] >> 4;
    result.y = (int32_t)view_matrix.m[2][1] >> 4;
    result.z = (int32_t)view_matrix.m[2][2] >> 4;
    return result;
}

#endif
