#ifdef _PSX
#include "renderer.h"
#include <stdio.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <inline_c.h>
#include <string.h>
#include <psxpad.h>
#include "player.h"
#include "input.h"
#include "vec2.h"
#include "lut.h"

#define TRI_THRESHOLD_MUL_SUB2 2
#define TRI_THRESHOLD_MUL_SUB1 5
#define TRI_THRESHOLD_NORMAL 500
#define TRI_THRESHOLD_FADE_START 700
#define TRI_THRESHOLD_FADE_END 1000
#define MESH_RENDER_DISTANCE 12000
#define N_CLUT_FADES 16
#define N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE 4

// Define environment pairs and buffer counter
DISPENV disp[2];
DRAWENV draw[2];
int drawbuffer;
int delta_time_raw_curr = 0;
int delta_time_raw_prev = 0;

uint32_t ord_tbl[2][ORD_TBL_LENGTH];
uint32_t primitive_buffer[2][(32768 << 3) / sizeof(uint32_t)];
uint32_t* next_primitive;
MATRIX view_matrix;
vec3_t camera_pos;
vec3_t camera_dir;
int vsync_enable = 1;
int frame_counter = 0;
int n_meshes_drawn = 0;
int n_meshes_total = 0;
uint8_t tex_id_start = 0;
int drawn_first_frame = 0;

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

#ifdef PAL
    // Correction for PAL
    disp[0].screen.y = 20;
    disp[0].screen.h = 256;
    disp[1].screen.y = 20;
    disp[1].screen.h = 256;
#endif


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
    gte_SetGeomScreen(120);

    next_primitive = primitive_buffer[0];
    drawn_first_frame = 0;
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
    VECTOR position = camera_transform->position;
    memcpy(&camera_pos, &camera_transform->position, sizeof(camera_pos));
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
	camera_dir.x = view_matrix.m[2][0];
	camera_dir.y = view_matrix.m[2][1];
	camera_dir.z = view_matrix.m[2][2];
    n_meshes_drawn = 0;
    n_meshes_total = 0;
}

// For some reason, making this a macro improved performance by rough 8%. Can't tell you why though.
#define ADD_TEX_TRI_TO_QUEUE(p0, p1, p2, v0, v1, v2, avg_z, clut_fade, tex_id, is_page) {       \
    POLY_GT3* new_triangle = (POLY_GT3*)next_primitive;\
    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);\
    setPolyGT3(new_triangle); \
    setXY3(new_triangle, \
        p0.x, p0.y,\
        p1.x, p1.y,\
        p2.x, p2.y\
    );\
    setRGB0(new_triangle, v0.r >> 1, v0.g >> 1, v0.b >> 1);\
    setRGB1(new_triangle, v1.r >> 1, v1.g >> 1, v1.b >> 1);\
    setRGB2(new_triangle, v2.r >> 1, v2.g >> 1, v2.b >> 1);\
    if (is_page) {\
        new_triangle->clut = getClut(768, 384 + (32*(tex_id)));\
        new_triangle->tpage = getTPage(1, 0, 128 * tex_id, 256);\
        setUV3(new_triangle,\
            v0.u,\
            v0.v,\
            v1.u,\
            v1.v,\
            v2.u,\
            v2.v\
        );\
    }\
    else {\
        const uint8_t tex_offset_x = ((tex_id) & 0b00001100) << 4;\
        const uint8_t tex_offset_y = ((tex_id) & 0b00000011) << 6;\
        new_triangle->clut = getClut(palettes[tex_id].x, palettes[tex_id].y + clut_fade);\
        new_triangle->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y);\
        setUV3(new_triangle,\
            (v0.u >> 2) + tex_offset_x,\
            (v0.v >> 2) + tex_offset_y,\
            (v1.u >> 2) + tex_offset_x,\
            (v1.v >> 2) + tex_offset_y,\
            (v2.u >> 2) + tex_offset_x,\
            (v2.v >> 2) + tex_offset_y\
        );\
    }\
    addPrim(ord_tbl[drawbuffer] + (avg_z >> 0), new_triangle);\
}\

// Same as above but for quads
#define ADD_TEX_QUAD_TO_QUEUE(p0, p1, p2, p3, v0, v1, v2, v3, avg_z, clut_fade, tex_id, is_page) {       \
    POLY_GT4* new_triangle = (POLY_GT4*)next_primitive;\
    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);\
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
    if (is_page) {\
        new_triangle->clut = getClut(768, 384 + (32*(tex_id)));\
        new_triangle->tpage = getTPage(1, 0, 128 * tex_id, 256);\
        setUV4(new_triangle,\
            v0.u,\
            v0.v,\
            v1.u,\
            v1.v,\
            v2.u,\
            v2.v,\
            v3.u,\
            v3.v\
        );\
    }\
    else {\
        const uint8_t tex_offset_x = ((tex_id) & 0b00001100) << 4;\
        const uint8_t tex_offset_y = ((tex_id) & 0b00000011) << 6;\
        new_triangle->clut = getClut(palettes[tex_id].x, palettes[tex_id].y + clut_fade);\
        new_triangle->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y) & ~(1 << 9);\
        setUV4(new_triangle,\
            (v0.u >> 2) + tex_offset_x,\
            (v0.v >> 2) + tex_offset_y,\
            (v1.u >> 2) + tex_offset_x,\
            (v1.v >> 2) + tex_offset_y,\
            (v2.u >> 2) + tex_offset_x,\
            (v2.v >> 2) + tex_offset_y,\
            (v3.u >> 2) + tex_offset_x,\
            (v3.v >> 2) + tex_offset_y\
        );\
    }\
    addPrim(ord_tbl[drawbuffer] + (avg_z), new_triangle);\
}\

__attribute__((always_inline)) inline void draw_triangle_shaded_paged(vertex_3d_t* verts, int texpage) {    // Transform the triangle vertices - there's 6 entries instead of 3, so we have enough room for subdivided triangles
    svec2_t trans_vec_xy[3];
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
    gte_avsz3();
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z >> 0) >= ORD_TBL_LENGTH || ((avg_z >> 0) <= 0)) return;

    ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], verts[0], verts[1], verts[2], avg_z, 0, texpage, 1);
}

__attribute__((always_inline)) inline void draw_quad_shaded_paged(vertex_3d_t* verts, int texpage) {    // Transform the triangle vertices - there's 6 entries instead of 3, so we have enough room for subdivided triangles
    // Transform the quad vertices, with enough entries in the array for subdivided quads
    svec2_t trans_vec_xy[4];
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
    gte_ldv0(&verts[3]);
    gte_rtps();
    gte_stsxy(&trans_vec_xy[3]);
    gte_avsz4();
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z >> 0) >= ORD_TBL_LENGTH || ((avg_z >> 0) <= 0)) return;

    // Depth culling
    if ((avg_z >> 0) >= ORD_TBL_LENGTH || ((avg_z >> 0) <= 0)) return;

    ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[3], verts[0], verts[1], verts[2], verts[3], avg_z, 0, texpage, 1);
}

__attribute__((always_inline)) inline void draw_triangle_shaded(vertex_3d_t* verts) {
    // Transform the triangle vertices - there's 6 entries instead of 3, so we have enough room for subdivided triangles
    svec2_t trans_vec_xy[15];
    scalar_t trans_vec_z[15];
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
    if ((avg_z >> 0) >= ORD_TBL_LENGTH || ((avg_z >> 0) <= 0)) return;

#if 1
    // Is this triangle even on screen?
    for (size_t i = 0; i < 3; ++i) {
        if (
            trans_vec_xy[i].x >= 0 && 
            trans_vec_xy[i].x <= RES_X &&
            trans_vec_xy[i].y >= 0 && 
            trans_vec_xy[i].y <= RES_Y 
        ) {
            goto dont_return;
        }
    }
    return;

    dont_return:
#endif
	int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
	if (clut_fade > 15) clut_fade = 15;
	else if (clut_fade < 0) clut_fade = 0;
#if 1

#endif
    const scalar_t sub2_threshold = TRI_THRESHOLD_MUL_SUB2 * (int32_t)verts[1].tex_id;
    if (avg_z < sub2_threshold) {
        // Generate all 15 vertices
        #define vtx0 verts[0]
        #define vtx1 verts[1]
        #define vtx2 verts[2]
        const vertex_3d_t vtx3 = get_halfway_point(vtx0, vtx1);
        const vertex_3d_t vtx4 = get_halfway_point(vtx1, vtx2);
        const vertex_3d_t vtx5 = get_halfway_point(vtx0, vtx2);
        const vertex_3d_t vtx6 = get_halfway_point(vtx0, vtx3);
        const vertex_3d_t vtx7 = get_halfway_point(vtx1, vtx3);
        const vertex_3d_t vtx8 = get_halfway_point(vtx1, vtx4);
        const vertex_3d_t vtx9 = get_halfway_point(vtx4, vtx2);
        const vertex_3d_t vtx10 = get_halfway_point(vtx5, vtx2);
        const vertex_3d_t vtx11 = get_halfway_point(vtx5, vtx0);
        const vertex_3d_t vtx12 = get_halfway_point(vtx5, vtx3);
        const vertex_3d_t vtx13 = get_halfway_point(vtx12, vtx8);
        const vertex_3d_t vtx14 = get_halfway_point(vtx5, vtx4);

        // Transform them all
        gte_ldv3(&vtx3.x, &vtx4.x, &vtx5.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[3]);
        gte_stsz3c(&trans_vec_z[3]);
        gte_ldv3(&vtx6.x, &vtx7.x, &vtx8.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[6]);
        gte_stsz3c(&trans_vec_z[6]);
        gte_ldv3(&vtx9.x, &vtx10.x, &vtx11.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[9]);
        gte_stsz3c(&trans_vec_z[9]);
        gte_ldv3(&vtx12.x, &vtx13.x, &vtx14.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[12]);
        gte_stsz3c(&trans_vec_z[12]);

        // Add to queue
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[6], trans_vec_xy[11], vtx0, vtx6, vtx11, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[11], trans_vec_xy[12], trans_vec_xy[5], vtx11, vtx12, vtx5, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[5], trans_vec_xy[14], trans_vec_xy[10], vtx5, vtx14, vtx10, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[10], trans_vec_xy[9], trans_vec_xy[2], vtx10, vtx9, vtx2, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[6], trans_vec_xy[3], trans_vec_xy[11], trans_vec_xy[12], vtx6, vtx3, vtx11, vtx12, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[7], trans_vec_xy[12], trans_vec_xy[13], vtx3, vtx7, vtx12, vtx13, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[7], trans_vec_xy[1], trans_vec_xy[13], trans_vec_xy[8], vtx7, vtx1, vtx13, vtx8, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[12], trans_vec_xy[13], trans_vec_xy[5], trans_vec_xy[14], vtx12, vtx13, vtx5, vtx14, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[13], trans_vec_xy[8], trans_vec_xy[14], trans_vec_xy[4], vtx13, vtx8, vtx14, vtx4, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[14], trans_vec_xy[4], trans_vec_xy[10], trans_vec_xy[9], vtx14, vtx4, vtx10, vtx9, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);

        // Filler triangles
        scalar_t max_z = trans_vec_z[0];
        if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
        if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2]; 
        if (max_z >= sub2_threshold) {
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[3], vtx0, vtx1, vtx3, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[3], trans_vec_xy[6], vtx0, vtx3, vtx6, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[1], trans_vec_xy[7], vtx3, vtx1, vtx7, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[4], vtx1, vtx2, vtx4, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[1], trans_vec_xy[4], trans_vec_xy[8], vtx1, vtx4, vtx8, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[4], trans_vec_xy[2], trans_vec_xy[9], vtx4, vtx2, vtx9, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[2], trans_vec_xy[5], vtx0, vtx2, vtx5, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[5], trans_vec_xy[11], vtx0, vtx5, vtx11, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[5], trans_vec_xy[2], trans_vec_xy[10], vtx5, vtx2, vtx10, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
        }

        #undef vtx0
        #undef vtx1
        #undef vtx2
        return;
    }
#if 1
    const scalar_t sub1_threshold = TRI_THRESHOLD_MUL_SUB1 * (int32_t)verts[1].tex_id;
    if (avg_z < sub1_threshold) {
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
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[3], trans_vec_xy[5], verts[0], ab, ca, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[1], trans_vec_xy[4], ab, verts[1], bc, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[4], trans_vec_xy[5], ab, bc, ca, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[5], trans_vec_xy[4], trans_vec_xy[2], ca, bc, verts[2], avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);

        // Filler triangles
        scalar_t max_z = trans_vec_z[0];
        if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
        if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2]; 
        if (max_z >= sub1_threshold) {
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[3], verts[0], verts[1], ab, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[4], verts[1], verts[2], bc, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[2], trans_vec_xy[0], trans_vec_xy[5], verts[2], verts[0], ca, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
        }
        return;
    }
#endif
    if (avg_z < TRI_THRESHOLD_FADE_END) {
        ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], verts[0], verts[1], verts[2], avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        return;
    }

    else {
        // Create primitive
        POLY_G3* new_triangle = (POLY_G3*)next_primitive;
        next_primitive += sizeof(POLY_G3) / sizeof(*next_primitive);

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
            const uint16_t r = ((((uint16_t)verts[ci].r) * ((uint16_t)textures_avg_colors[verts->tex_id + tex_id_start].r)) >> 8);
            const uint16_t g = ((((uint16_t)verts[ci].g) * ((uint16_t)textures_avg_colors[verts->tex_id + tex_id_start].g)) >> 8);
            const uint16_t b = ((((uint16_t)verts[ci].b) * ((uint16_t)textures_avg_colors[verts->tex_id + tex_id_start].b)) >> 8);
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
        addPrim(ord_tbl[drawbuffer] + (avg_z >> 0), new_triangle);
        return;
    }
}
__attribute__((always_inline)) inline void draw_quad_shaded(vertex_3d_t* verts) {
    // Transform the quad vertices, with enough entries in the array for subdivided quads
    svec2_t trans_vec_xy[25];
    scalar_t trans_vec_z[25];
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
    if (p <= -16) return;

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
    if ((avg_z >> 0) >= ORD_TBL_LENGTH || ((avg_z >> 0) <= 0)) return;
    
#if 1
    // Is this quad even on screen?
    for (size_t i = 0; i < 4; ++i) {
        if (
            trans_vec_xy[i].x >= 0 && 
            trans_vec_xy[i].x <= RES_X &&
            trans_vec_xy[i].y >= 0 && 
            trans_vec_xy[i].y <= RES_Y 
        ) {
            goto dont_return;
        }
    }
    return;

    dont_return:
#endif
	int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
	if (clut_fade > 15) clut_fade = 15;
	else if (clut_fade < 0) clut_fade = 0;
#if 1
    const scalar_t sub2_threshold = TRI_THRESHOLD_MUL_SUB2 * (int32_t)verts[1].tex_id;
    if (avg_z < TRI_THRESHOLD_MUL_SUB2 * (int32_t)verts[1].tex_id) {
        // Generate all 25 vertices
        #define vtx0 verts[0]
        #define vtx1 verts[1]
        #define vtx2 verts[2]
        #define vtx3 verts[3]
        const vertex_3d_t vtx4 = get_halfway_point(vtx0, vtx1);
        const vertex_3d_t vtx5 = get_halfway_point(vtx2, vtx3);
        const vertex_3d_t vtx6 = get_halfway_point(vtx4, vtx5);
        const vertex_3d_t vtx7 = get_halfway_point(vtx0, vtx2);
        const vertex_3d_t vtx8 = get_halfway_point(vtx1, vtx3);
        const vertex_3d_t vtx9 = get_halfway_point(vtx0, vtx4);
        const vertex_3d_t vtx10 = get_halfway_point(vtx4, vtx1);
        const vertex_3d_t vtx11 = get_halfway_point(vtx7, vtx6);
        const vertex_3d_t vtx12 = get_halfway_point(vtx6, vtx8);
        const vertex_3d_t vtx13 = get_halfway_point(vtx2, vtx5);
        const vertex_3d_t vtx14 = get_halfway_point(vtx5, vtx3);
        const vertex_3d_t vtx15 = get_halfway_point(vtx0, vtx7);
        const vertex_3d_t vtx16 = get_halfway_point(vtx9, vtx11);
        const vertex_3d_t vtx17 = get_halfway_point(vtx4, vtx6);
        const vertex_3d_t vtx18 = get_halfway_point(vtx10, vtx12);
        const vertex_3d_t vtx19 = get_halfway_point(vtx1, vtx8);
        const vertex_3d_t vtx20 = get_halfway_point(vtx7, vtx2);
        const vertex_3d_t vtx21 = get_halfway_point(vtx11, vtx13);
        const vertex_3d_t vtx22 = get_halfway_point(vtx6, vtx5);
        const vertex_3d_t vtx23 = get_halfway_point(vtx12, vtx14);
        const vertex_3d_t vtx24 = get_halfway_point(vtx8, vtx3);

        // Transform them all
        gte_ldv3(&vtx4.x, &vtx5.x, &vtx6.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[4]);
        gte_ldv3(&vtx7.x, &vtx8.x, &vtx9.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[7]);
        gte_ldv3(&vtx10.x, &vtx11.x, &vtx12.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[10]);
        gte_ldv3(&vtx13.x, &vtx14.x, &vtx15.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[13]);
        gte_ldv3(&vtx16.x, &vtx17.x, &vtx18.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[16]);
        gte_ldv3(&vtx19.x, &vtx20.x, &vtx21.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[19]);
        gte_ldv3(&vtx22.x, &vtx23.x, &vtx24.x);
        gte_rtpt();
        gte_stsxy3c(&trans_vec_xy[22]);

        // Add to queue
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[9], trans_vec_xy[15], trans_vec_xy[16], vtx0, vtx9, vtx15, vtx16, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[9], trans_vec_xy[4], trans_vec_xy[16], trans_vec_xy[17], vtx9, vtx4, vtx16, vtx17, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[4], trans_vec_xy[10], trans_vec_xy[17], trans_vec_xy[18], vtx4, vtx10, vtx17, vtx18, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[10], trans_vec_xy[1], trans_vec_xy[18], trans_vec_xy[19], vtx10, vtx1, vtx18, vtx19, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[15], trans_vec_xy[16], trans_vec_xy[7], trans_vec_xy[11], vtx15, vtx16, vtx7, vtx11, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[16], trans_vec_xy[17], trans_vec_xy[11], trans_vec_xy[6], vtx16, vtx17, vtx11, vtx6, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[17], trans_vec_xy[18], trans_vec_xy[6], trans_vec_xy[12], vtx17, vtx18, vtx6, vtx12, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[18], trans_vec_xy[19], trans_vec_xy[12], trans_vec_xy[8], vtx18, vtx19, vtx12, vtx8, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[7], trans_vec_xy[11], trans_vec_xy[20], trans_vec_xy[21], vtx7, vtx11, vtx20, vtx21, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[11], trans_vec_xy[6], trans_vec_xy[21], trans_vec_xy[22], vtx11, vtx6, vtx21, vtx22, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[6], trans_vec_xy[12], trans_vec_xy[22], trans_vec_xy[23], vtx6, vtx12, vtx22, vtx23, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[12], trans_vec_xy[8], trans_vec_xy[23], trans_vec_xy[24], vtx12, vtx8, vtx23, vtx24, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[20], trans_vec_xy[21], trans_vec_xy[2], trans_vec_xy[13], vtx20, vtx21, vtx2, vtx13, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[21], trans_vec_xy[22], trans_vec_xy[13], trans_vec_xy[5], vtx21, vtx22, vtx13, vtx5, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[22], trans_vec_xy[23], trans_vec_xy[5], trans_vec_xy[14], vtx22, vtx23, vtx5, vtx14, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[23], trans_vec_xy[24], trans_vec_xy[14], trans_vec_xy[3], vtx23, vtx24, vtx14, vtx3, avg_z, clut_fade, verts[0].tex_id + tex_id_start, 0);

        // Filler triangles
        scalar_t max_z = trans_vec_z[0];
        if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
        if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2]; 
        if (trans_vec_z[3] > max_z) max_z = trans_vec_z[3];
        if (max_z >= sub2_threshold) {
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[4], vtx0, vtx1, vtx4, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[4], trans_vec_xy[9], vtx0, vtx4, vtx9, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[4], trans_vec_xy[1], trans_vec_xy[10], vtx4, vtx1, vtx10, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[1], trans_vec_xy[3], trans_vec_xy[8], vtx1, vtx3, vtx8, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[1], trans_vec_xy[8], trans_vec_xy[19], vtx1, vtx8, vtx19, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[8], trans_vec_xy[3], trans_vec_xy[24], vtx8, vtx3, vtx24, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[2], trans_vec_xy[5], vtx3, vtx2, vtx5, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[5], trans_vec_xy[14], vtx3, vtx5, vtx14, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[5], trans_vec_xy[2], trans_vec_xy[13], vtx5, vtx2, vtx13, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[2], trans_vec_xy[0], trans_vec_xy[7], vtx2, vtx0, vtx7, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[2], trans_vec_xy[7], trans_vec_xy[20], vtx2, vtx7, vtx20, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[7], trans_vec_xy[0], trans_vec_xy[15], vtx7, vtx0, vtx15, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
        }

        #undef vtx0
        #undef vtx1
        #undef vtx2
        #undef vtx3
        return;
    }
#endif 
#if 1
    const scalar_t sub1_threshold = TRI_THRESHOLD_MUL_SUB1 * (int32_t)verts[1].tex_id;
    if (avg_z < sub1_threshold) {
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

        // Calculate average Z
        int avg_z_0478;
        int avg_z_4185;
        int avg_z_7826;
        int avg_z_8563;
        gte_ldsz4(trans_vec_z[0], trans_vec_z[4], trans_vec_z[7], trans_vec_z[8]);
        gte_avsz4();
        gte_stotz(&avg_z_0478);
        gte_ldsz4(trans_vec_z[4], trans_vec_z[1], trans_vec_z[8], trans_vec_z[5]);
        gte_avsz4();
        gte_stotz(&avg_z_4185);
        gte_ldsz4(trans_vec_z[7], trans_vec_z[8], trans_vec_z[2], trans_vec_z[6]);
        gte_avsz4();
        gte_stotz(&avg_z_7826);
        gte_ldsz4(trans_vec_z[8], trans_vec_z[5], trans_vec_z[6], trans_vec_z[3]);
        gte_avsz4();
        gte_stotz(&avg_z_8563);

        // Draw them
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[4], trans_vec_xy[7], trans_vec_xy[8], verts[0], ab, da, center, avg_z_0478, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[4], trans_vec_xy[1], trans_vec_xy[8], trans_vec_xy[5], ab, verts[1], center, bc, avg_z_4185, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[7], trans_vec_xy[8], trans_vec_xy[2], trans_vec_xy[6], da, center, verts[2], cd, avg_z_7826, clut_fade, verts[0].tex_id + tex_id_start, 0);
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[8], trans_vec_xy[5], trans_vec_xy[6], trans_vec_xy[3], center, bc, cd, verts[3], avg_z_8563, clut_fade, verts[0].tex_id + tex_id_start, 0);

        // Filler triangles
        scalar_t max_z = trans_vec_z[0];
        if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
        if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2];
        if (trans_vec_z[3] > max_z) max_z = trans_vec_z[3];

        if (max_z >= sub2_threshold) {
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[4], verts[0], verts[1], ab, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[1], trans_vec_xy[3], trans_vec_xy[5], verts[1], verts[3], bc, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[3], trans_vec_xy[2], trans_vec_xy[6], verts[3], verts[2], cd, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
            ADD_TEX_TRI_TO_QUEUE(trans_vec_xy[2], trans_vec_xy[0], trans_vec_xy[7], verts[2], verts[0], da, (avg_z + 8), clut_fade, verts[0].tex_id + tex_id_start, 0);
        }

        return;
    }
#endif
    if (avg_z < TRI_THRESHOLD_FADE_END) {
        ADD_TEX_QUAD_TO_QUEUE(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[3], verts[0], verts[1], verts[2], verts[3], avg_z, clut_fade, verts[0].tex_id, 0);
        return;
    }

    else {
        // Create primitive
        POLY_G4* new_quad = (POLY_G4*)next_primitive;
        next_primitive += sizeof(POLY_G4) / sizeof(*next_primitive);

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
        addPrim(ord_tbl[drawbuffer] + (avg_z >> 0), new_quad);
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
    if ((p >> 2) >= ORD_TBL_LENGTH || ((p >> 2) <= 0))
        return;

    // Store them
    int16_t vertex_positions[6];
    gte_stsxy0(&vertex_positions[0]);
    gte_stsxy1(&vertex_positions[2]);
    gte_stsxy2(&vertex_positions[4]);

    // Create primitive
    POLY_G3* new_triangle = (POLY_G3*)next_primitive;
    next_primitive += sizeof(POLY_G3) / sizeof(*next_primitive);

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
    ++n_meshes_total;
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
        const vec3_t *min = &mesh->bounds.min;
        const vec3_t *max = &mesh->bounds.max;

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
            if (vertices[i].vx < after_min.vx) after_min.vx = vertices[i].vx;
            if (vertices[i].vx > after_max.vx) after_max.vx = vertices[i].vx;
            if (vertices[i].vy < after_min.vy) after_min.vy = vertices[i].vy;
            if (vertices[i].vy > after_max.vy) after_max.vy = vertices[i].vy;
            if (vertices[i].vz < after_min.vz) after_min.vz = vertices[i].vz;
            if (vertices[i].vz > after_max.vz) after_max.vz = vertices[i].vz;
        }

        // If this screen aligned bounding box is off screen, do not draw this mesh
        #define FRUSCUL_PAD_X 0 // these are in case I ever want to add a safe zone
        #define FRUSCUL_PAD_Y 0 // or for debugging.
        if (after_max.vx < 0+FRUSCUL_PAD_X) return; // mesh is to the left of the screen
        if (after_max.vy < 0+FRUSCUL_PAD_Y) return; // mesh is above the screen
        if (after_max.vz == 0) return; // mesh is behind the screen
        if (after_min.vz > MESH_RENDER_DISTANCE) return; // mesh is too far away
        if (after_min.vx > RES_X-FRUSCUL_PAD_X) return; // mesh is to the right of the screen
        if (after_min.vy > RES_Y-FRUSCUL_PAD_Y) return; // mesh is below the screen
        #undef FRUSCUL_PAD_X
        #undef FRUSCUL_PAD_Y
    };

    ++n_meshes_drawn;
    //n_total_triangles += mesh->n_vertices / 3;

    // Loop over each triangle
    size_t vert_idx = 0;
    for (size_t i = 0; i < mesh->n_triangles; ++i) {
        draw_triangle_shaded(&mesh->vertices[vert_idx]);
        vert_idx += 3;
    }
    for (size_t i = 0; i < mesh->n_quads; ++i) {
        draw_quad_shaded(&mesh->vertices[vert_idx]);
        vert_idx += 4;
    }

	PopMatrix();
}

void renderer_draw_model_shaded(const model_t* model, transform_t* model_transform, vislist_t* vislist, int tex_id_offset) {
    if (vislist == NULL || n_sections == 0) {
        for (size_t i = 0; i < model->n_meshes; ++i) {
            //renderer_debug_draw_aabb(&model->meshes[i].bounds, red, &id_transform);
            renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
        }
    }
    else  {
        // Determine which meshes to render
        vislist_t combined = {0, 0, 0, 0};

        // Get all the vislist bitfields and combine them together
        for (size_t i = 0; i < n_sections; ++i) {
            combined.sections_0_31 |= vislist[sections[i]].sections_0_31;
            combined.sections_32_63 |= vislist[sections[i]].sections_32_63;
            combined.sections_64_95 |= vislist[sections[i]].sections_64_95;
            combined.sections_96_127 |= vislist[sections[i]].sections_96_127;
        }

        // Render only the meshes that are visible
        for (size_t i = 0; i < model->n_meshes; ++i) {
            if ((i < 32) && (combined.sections_0_31 & (1 << i))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i < 64) && (combined.sections_32_63 & (1 << (i - 32)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i < 96) && (combined.sections_64_95 & (1 << (i - 64)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i < 128) && (combined.sections_96_127 & (1 << (i - 96)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
        }
    }
}

void renderer_draw_entity_shaded(const mesh_t* mesh, transform_t* model_transform, int texpage) {
    if (!mesh) return;
    
    // Set rotation and translation matrix
    MATRIX model_matrix;
    HiRotMatrix(&model_transform->rotation, &model_matrix);
    TransMatrix(&model_matrix, &model_transform->position);
    CompMatrixLV(&view_matrix, &model_matrix, &model_matrix);

    // Send it to the GTE
	PushMatrix();
    gte_SetRotMatrix(&model_matrix);
    gte_SetTransMatrix(&model_matrix);

    size_t vert_idx = 0;
    for (size_t i = 0; i < mesh->n_triangles; ++i) {
        draw_triangle_shaded_paged(&mesh->vertices[vert_idx], texpage);
        vert_idx += 3;
    }
    for (size_t i = 0; i < mesh->n_quads; ++i) {
        draw_quad_shaded_paged(&mesh->vertices[vert_idx], texpage);
        vert_idx += 4;
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
    if ((p >> 2) >= ORD_TBL_LENGTH || ((p >> 2) <= 0))
        return;

    LINE_F2* new_line = (LINE_F2*)next_primitive;
    next_primitive += sizeof(LINE_F2) / sizeof(*next_primitive);

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

void renderer_upload_texture(const texture_cpu_t* texture, const uint8_t index) {
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

void render_upload_8bit_texture_page(const texture_cpu_t* texture, const uint8_t index) {
    WARN_IF("texture is not a full page!", texture->width != 256 || texture->height != 256);
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
    if (drawn_first_frame)
        SetDispMask(1);
    
    // Draw Ordering Table
    DrawOTag(ord_tbl[1-drawbuffer] + ORD_TBL_LENGTH - 1);

    drawn_first_frame = 1;
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

int renderer_convert_dt_raw_to_ms(int dt_raw) {
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

int renderer_get_delta_time_ms(void) {
    int dt_raw = renderer_get_delta_time_raw();
    return renderer_convert_dt_raw_to_ms(dt_raw);
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

void renderer_draw_2d_quad_axis_aligned(vec2_t center, vec2_t size, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
    const vec2_t tl = {center.x - size.x/2, center.y - size.y/2};
    const vec2_t tr = {center.x + size.x/2, center.y - size.y/2};
    const vec2_t bl = {center.x - size.x/2, center.y + size.y/2};
    const vec2_t br = {center.x + size.x/2, center.y + size.y/2};
    renderer_draw_2d_quad(tl, tr, bl, br, uv_tl, uv_br, color, depth, texture_id, is_page);
}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
#ifdef PAL
    const int y_offset = 0;
#else
    const int y_offset = -16;
#endif
    POLY_FT4* new_triangle = (POLY_FT4*)next_primitive;
    next_primitive += sizeof(POLY_FT4) / sizeof(*next_primitive);
    setPolyFT4(new_triangle); 
    setXY4(new_triangle, 
        tl.x / ONE, (tl.y / ONE) + y_offset,
        tr.x / ONE, (tr.y / ONE) + y_offset,
        bl.x / ONE, (bl.y / ONE) + y_offset,
        br.x / ONE, (br.y / ONE) + y_offset
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
    addPrim(ord_tbl[drawbuffer] + (depth), new_triangle);
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
    setWH(new_tile, RES_X, RES_Y);
    addPrim(ord_tbl[drawbuffer] + 0, new_tile);
    
    // Set color blend mode to subtract
    DR_TPAGE* new_tpage = (DR_TPAGE*)next_primitive;
    next_primitive += sizeof(DR_TPAGE) / sizeof(*next_primitive);
    setDrawTPage(new_tpage, 1, 0, 2 << 5);
    addPrim(ord_tbl[drawbuffer] + 0, new_tpage);
}


void renderer_draw_text(vec2_t pos, char* text, int is_big, int centered) {
    // if (is_small)
    int font_x = 0;
    int font_y = 60;
    int font_width = 7;
    int font_height = 9;
    int chars_per_row = 36;

    if (is_big) {
        font_x = 0;
        font_y = 80;
        font_width = 16;
        font_height = 16;
        chars_per_row = 16;
    }

    if (centered) {
        pos.x -= ((strlen(text)) * font_width - (font_width / 2)) * ONE / 2;
    }

    vec2_t start_pos = pos;

    // Draw each character
    while (*text) {
        // Handle special cases
        if (*text == '\n') {
            pos.x = start_pos.x;
            pos.y += font_height * ONE;
            goto end;
        }
        
        if (*text == '\r') {
            pos.x = start_pos.x;
            goto end;
        }

        if (*text == '\t') {
            // Get X coordinate relative to start, and round the position up to the nearest multiple of 4
            scalar_t rel_x = pos.x - start_pos.x;
            int n_spaces = 4 - ((rel_x / font_width) % 4);
            pos.x += n_spaces * font_width * ONE;
            goto end;
        }

        // Get index in bitmap
        int index_to_draw = (int)lut_font_letters[(size_t)*text];
        
        // Get UV coordinates
        vec2_t top_left;
        top_left.x = (font_x + (font_width * (index_to_draw % chars_per_row))) * ONE;
        top_left.y = (font_y + ((index_to_draw / chars_per_row) * font_height)) * ONE;
        vec2_t bottom_right = vec2_add(top_left, (vec2_t){(font_width-1)*ONE, (font_height-1)*ONE});

        renderer_draw_2d_quad_axis_aligned(pos, (vec2_t){(font_width-1)*ONE, (font_height-1)*ONE}, top_left, bottom_right, (pixel32_t){128, 128, 128}, 0, 5, 1);
        pos.x += font_width * ONE;
        end:
        ++text;
    }
}

#endif
