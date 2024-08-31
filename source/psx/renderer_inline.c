#include <psxgte.h>
#include <inline_c.h>

static inline int frustrum_cull_aabb(const vec3_t min, const vec3_t max) {
    // We will transform these to screen space
    SVECTOR vertices[8] = {
        ((SVECTOR) {min.x, min.y, min.z, 0}),
        ((SVECTOR) {min.x, min.y, max.z, 0}),
        ((SVECTOR) {min.x, max.y, min.z, 0}),
        ((SVECTOR) {min.x, max.y, max.z, 0}),
        ((SVECTOR) {max.x, min.y, min.z, 0}),
        ((SVECTOR) {max.x, min.y, max.z, 0}),
        ((SVECTOR) {max.x, max.y, min.z, 0}),
        ((SVECTOR) {max.x, max.y, max.z, 0}),
    };

    // Transform vertices and store them back
    int16_t padding_for_last_store;
    // 0, 1, 2
    gte_ldv3(&vertices[0].vx, &vertices[1].vx, &vertices[2].vx);
    gte_rtpt();
    gte_stsxy3(&vertices[0].vx, &vertices[1].vx, &vertices[2].vx);
    gte_stsz3(&vertices[0].vz, &vertices[1].vz, &vertices[2].vz);
    // 3, 4, 5
    gte_ldv3(&vertices[3].vx, &vertices[4].vx, &vertices[5].vx);
    gte_rtpt();
    gte_stsxy3(&vertices[3].vx, &vertices[4].vx, &vertices[5].vx);
    gte_stsz3(&vertices[3].vz, &vertices[4].vz, &vertices[5].vz);
    // 6, 7
    gte_ldv01(&vertices[6].vx, &vertices[7].vx);
    gte_rtpt();
    gte_stsxy01(&vertices[6].vx, &vertices[7].vx);
    gte_stsz3(&vertices[6].vz, &vertices[7].vz, &padding_for_last_store);

    // Find screen aligned bounding box
    SVECTOR after_min = (SVECTOR){INT16_MAX, INT16_MAX, INT16_MAX, 0};
    SVECTOR after_max = (SVECTOR){INT16_MIN, INT16_MIN, INT16_MIN, 0};
    
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
    if (after_max.vx < 0+FRUSCUL_PAD_X) return 1; // mesh is to the left of the screen
    if (after_max.vy < 0+FRUSCUL_PAD_Y) return 1; // mesh is above the screen
    if (after_max.vz == 0) return 1; // mesh is behind the screen
    if (after_min.vz > MESH_RENDER_DISTANCE) return 1; // mesh is too far away
    if (after_min.vx > res_x-FRUSCUL_PAD_X) return 1; // mesh is to the right of the screen
    if (after_min.vy > curr_res_y-FRUSCUL_PAD_Y) return 1; // mesh is below the screen
    #undef FRUSCUL_PAD_X
    #undef FRUSCUL_PAD_Y
    return 0;
}

static inline uint8_t mul_8x8(const uint8_t a, const uint8_t b) {
    uint16_t c = ((uint16_t)a * (uint16_t)b) >> 8;
    if (c > 255) c = 255;
    return (uint8_t)c;
}

// Queues textured triangle primitive
static inline void add_tex_triangle(
    const svec2_t p0, 
    const svec2_t p1, 
    const svec2_t p2, 
    const vertex_3d_t v0, 
    const vertex_3d_t v1, 
    const vertex_3d_t v2, 
    const scalar_t avg_z, 
    const int tex_id, 
    const int16_t clut_fade
) {	
    // Allocate new triangle
    ++n_polygons_drawn;
    POLY_GT3* new_triangle = (POLY_GT3*)next_primitive;
    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Populate triangle data
    setPolyGT3(new_triangle); 
    setXY3(new_triangle, 
        p0.x, p0.y,
        p1.x, p1.y,
        p2.x, p2.y
    );
    setRGB0(new_triangle, v0.r >> 1, v0.g >> 1, v0.b >> 1);
    setRGB1(new_triangle, v1.r >> 1, v1.g >> 1, v1.b >> 1);
    setRGB2(new_triangle, v2.r >> 1, v2.g >> 1, v2.b >> 1);
    const uint8_t tex_offset_x = ((tex_id) & 0b00001100) << 4;
    const uint8_t tex_offset_y = ((tex_id) & 0b00000011) << 6;
    new_triangle->clut = getClut(palettes[tex_id].x, palettes[tex_id].y + clut_fade);
    new_triangle->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y);
    setUV3(new_triangle,
        (v0.u >> 2) + tex_offset_x,
        (v0.v >> 2) + tex_offset_y,
        (v1.u >> 2) + tex_offset_x,
        (v1.v >> 2) + tex_offset_y,
        (v2.u >> 2) + tex_offset_x,
        (v2.v >> 2) + tex_offset_y
    );
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, new_triangle);
}

// Queues textured quad primitive
static inline void add_tex_quad(
    const svec2_t p0, 
    const svec2_t p1, 
    const svec2_t p2, 
    const svec2_t p3, 
    const vertex_3d_t v0, 
    const vertex_3d_t v1, 
    const vertex_3d_t v2, 
    const vertex_3d_t v3, 
    const scalar_t avg_z, 
    const int tex_id, 
    const int16_t clut_fade
) {	
    // Allocate new triangle
    ++n_polygons_drawn;
    POLY_GT4* new_quad = (POLY_GT4*)next_primitive;
    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);

    // Populate triangle data
    setPolyGT4(new_quad); 
    setXY4(new_quad, 
        p0.x, p0.y,
        p1.x, p1.y,
        p2.x, p2.y,
        p3.x, p3.y
    );
    setRGB0(new_quad, v0.r >> 1, v0.g >> 1, v0.b >> 1);
    setRGB1(new_quad, v1.r >> 1, v1.g >> 1, v1.b >> 1);
    setRGB2(new_quad, v2.r >> 1, v2.g >> 1, v2.b >> 1);
    setRGB3(new_quad, v3.r >> 1, v3.g >> 1, v3.b >> 1);
    const uint8_t tex_offset_x = ((tex_id) & 0b00001100) << 4;
    const uint8_t tex_offset_y = ((tex_id) & 0b00000011) << 6;
    new_quad->clut = getClut(palettes[tex_id].x, palettes[tex_id].y + clut_fade);
    new_quad->tpage = getTPage(0, 0, textures[tex_id].x, textures[tex_id].y);
    setUV4(new_quad,
        (v0.u >> 2) + tex_offset_x, (v0.v >> 2) + tex_offset_y,
        (v1.u >> 2) + tex_offset_x, (v1.v >> 2) + tex_offset_y,
        (v2.u >> 2) + tex_offset_x, (v2.v >> 2) + tex_offset_y,
        (v3.u >> 2) + tex_offset_x, (v3.v >> 2) + tex_offset_y
    );
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, new_quad);
}

// Queues untextured triangle primitive
static inline void add_untex_triangle(
    const svec2_t p0, 
    const svec2_t p1, 
    const svec2_t p2, 
    const vertex_3d_t v0, 
    const vertex_3d_t v1, 
    const vertex_3d_t v2, 
    const scalar_t avg_z
) {
    // Create primitive
    ++n_polygons_drawn;
    POLY_G3* new_triangle = (POLY_G3*)next_primitive;
    next_primitive += sizeof(POLY_G3) / sizeof(*next_primitive);

    // Initialize the entry in the render queue
    setPolyG3(new_triangle);

    // Set the vertex positions of the triangle
    setXY3(new_triangle, 
        p0.x, p0.y,
        p1.x, p1.y,
        p2.x, p2.y
    );

    // Set the vertex colors of the triangle
    setRGB0(new_triangle,
        mul_8x8(v0.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v0.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v0.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB1(new_triangle,
        mul_8x8(v1.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v1.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v1.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB2(new_triangle,
        mul_8x8(v2.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v2.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v2.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    
    // Add the triangle to the draw queue
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, new_triangle);
}

// Queues untextured quad primitive
static inline void add_untex_quad(
    const svec2_t p0, 
    const svec2_t p1,
    const svec2_t p2, 
    const svec2_t p3, 
    const vertex_3d_t v0, 
    const vertex_3d_t v1, 
    const vertex_3d_t v2, 
    const vertex_3d_t v3, 
    const scalar_t avg_z
) {
    // Create primitive
    ++n_polygons_drawn;
    POLY_G4* new_quad = (POLY_G4*)next_primitive;
    next_primitive += sizeof(POLY_G4) / sizeof(*next_primitive);

    // Initialize the entry in the render queue
    setPolyG4(new_quad);

    // Set the vertex positions of the quad
    setXY4(new_quad, 
        p0.x, p0.y,
        p1.x, p1.y,
        p2.x, p2.y,
        p3.x, p3.y
    );

    // Set the vertex colors of the quad
    setRGB0(new_quad,
        mul_8x8(v0.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v0.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v0.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB1(new_quad,
        mul_8x8(v1.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v1.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v1.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB2(new_quad,
        mul_8x8(v2.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v2.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v2.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB3(new_quad,
        mul_8x8(v3.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v3.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v3.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    
    // Add the triangle to the draw queue
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, new_quad);
}

static inline vertex_3d_t get_halfway_point(const vertex_3d_t v0, const vertex_3d_t v1) {
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

static inline void subdivide_twice_then_add_tex_triangle(const vertex_3d_t* verts, svec2_t* trans_vec_xy, scalar_t* trans_vec_z, int sub2_threshold) {
    // Generate all 15 vertices
    #define vtx0 verts[0]
    #define vtx1 verts[1]
    #define vtx2 verts[2]
    const vertex_3d_t vtx3 = get_halfway_point(vtx0, vtx1);
    const vertex_3d_t vtx4 = get_halfway_point(vtx1, vtx2);
    const vertex_3d_t vtx5 = get_halfway_point(vtx0, vtx2);
    gte_ldv3(&vtx3.x, &vtx4.x, &vtx5.x);
    gte_rtpt();
    const vertex_3d_t vtx6 = get_halfway_point(vtx0, vtx3);
    const vertex_3d_t vtx7 = get_halfway_point(vtx1, vtx3);
    const vertex_3d_t vtx8 = get_halfway_point(vtx1, vtx4);
    gte_stsxy3c(&trans_vec_xy[3]);
    gte_stsz3c(&trans_vec_z[3]);
    gte_ldv3(&vtx6.x, &vtx7.x, &vtx8.x);
    gte_rtpt();
    const vertex_3d_t vtx9 = get_halfway_point(vtx4, vtx2);
    const vertex_3d_t vtx10 = get_halfway_point(vtx5, vtx2);
    const vertex_3d_t vtx11 = get_halfway_point(vtx5, vtx0);
    gte_stsxy3c(&trans_vec_xy[6]);
    gte_stsz3c(&trans_vec_z[6]);
    gte_ldv3(&vtx9.x, &vtx10.x, &vtx11.x);
    gte_rtpt();
    const vertex_3d_t vtx12 = get_halfway_point(vtx5, vtx3);
    const vertex_3d_t vtx13 = get_halfway_point(vtx12, vtx8);
    const vertex_3d_t vtx14 = get_halfway_point(vtx5, vtx4);
    gte_stsxy3c(&trans_vec_xy[9]);
    gte_stsz3c(&trans_vec_z[9]);
    gte_ldv3(&vtx12.x, &vtx13.x, &vtx14.x);
    gte_rtpt();
    gte_stsxy3c(&trans_vec_xy[12]);
    gte_stsz3c(&trans_vec_z[12]);

    // Calculate average Z values
    int avg_z_06b;
    int avg_z_bc5;
    int avg_z_5ea;
    int avg_z_a92;
    int avg_z_63bc;
    int avg_z_37cd;
    int avg_z_71d8;
    int avg_z_cd5e;
    int avg_z_d8e4;
    int avg_z_e4a9;
    gte_ldsz3(trans_vec_z[0], trans_vec_z[6], trans_vec_z[11]);
    gte_avsz3();
    gte_stotz(&avg_z_06b);
    gte_ldsz3(trans_vec_z[11], trans_vec_z[12], trans_vec_z[5]);
    gte_avsz3();
    gte_stotz(&avg_z_bc5);
    gte_ldsz3(trans_vec_z[5], trans_vec_z[14], trans_vec_z[10]);
    gte_avsz3();
    gte_stotz(&avg_z_5ea);
    gte_ldsz3(trans_vec_z[10], trans_vec_z[9], trans_vec_z[2]);
    gte_avsz3();
    gte_stotz(&avg_z_a92);
    gte_ldsz4(trans_vec_z[6], trans_vec_z[3], trans_vec_z[11], trans_vec_z[12]);
    gte_avsz4();
    gte_stotz(&avg_z_63bc);
    gte_ldsz4(trans_vec_z[3], trans_vec_z[7], trans_vec_z[12], trans_vec_z[13]);
    gte_avsz4();
    gte_stotz(&avg_z_37cd);
    gte_ldsz4(trans_vec_z[7], trans_vec_z[1], trans_vec_z[13], trans_vec_z[8]);
    gte_avsz4();
    gte_stotz(&avg_z_71d8);
    gte_ldsz4(trans_vec_z[12], trans_vec_z[13], trans_vec_z[5], trans_vec_z[14]);
    gte_avsz4();
    gte_stotz(&avg_z_cd5e);
    gte_ldsz4(trans_vec_z[13], trans_vec_z[8], trans_vec_z[14], trans_vec_z[4]);
    gte_avsz4();
    gte_stotz(&avg_z_d8e4);
    gte_ldsz4(trans_vec_z[14], trans_vec_z[4], trans_vec_z[10], trans_vec_z[9]);
    gte_avsz4();
    gte_stotz(&avg_z_e4a9);

    // Add to queue
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[6], trans_vec_xy[11], vtx0, vtx6, vtx11, avg_z_06b, verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[11], trans_vec_xy[12], trans_vec_xy[5], vtx11, vtx12, vtx5, avg_z_bc5, verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[5], trans_vec_xy[14], trans_vec_xy[10], vtx5, vtx14, vtx10, avg_z_5ea, verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[10], trans_vec_xy[9], trans_vec_xy[2], vtx10, vtx9, vtx2, avg_z_a92, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[6], trans_vec_xy[3], trans_vec_xy[11], trans_vec_xy[12], vtx6, vtx3, vtx11, vtx12, avg_z_63bc, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[3], trans_vec_xy[7], trans_vec_xy[12], trans_vec_xy[13], vtx3, vtx7, vtx12, vtx13, avg_z_37cd, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[7], trans_vec_xy[1], trans_vec_xy[13], trans_vec_xy[8], vtx7, vtx1, vtx13, vtx8, avg_z_71d8, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[12], trans_vec_xy[13], trans_vec_xy[5], trans_vec_xy[14], vtx12, vtx13, vtx5, vtx14, avg_z_cd5e, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[13], trans_vec_xy[8], trans_vec_xy[14], trans_vec_xy[4], vtx13, vtx8, vtx14, vtx4, avg_z_d8e4, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[14], trans_vec_xy[4], trans_vec_xy[10], trans_vec_xy[9], vtx14, vtx4, vtx10, vtx9, avg_z_e4a9, verts[0].tex_id + tex_id_start, 0);

    // Filler triangles
    scalar_t max_z = trans_vec_z[0];
    if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
    if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2]; 
    max_z >>= 2;
    
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[3], vtx0, vtx1, vtx3, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[3], trans_vec_xy[6], vtx0, vtx3, vtx6, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[3], trans_vec_xy[1], trans_vec_xy[7], vtx3, vtx1, vtx7, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[4], vtx1, vtx2, vtx4, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[1], trans_vec_xy[4], trans_vec_xy[8], vtx1, vtx4, vtx8, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[4], trans_vec_xy[2], trans_vec_xy[9], vtx4, vtx2, vtx9, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[2], trans_vec_xy[5], vtx0, vtx2, vtx5, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[5], trans_vec_xy[11], vtx0, vtx5, vtx11, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[5], trans_vec_xy[2], trans_vec_xy[10], vtx5, vtx2, vtx10, (max_z + 1), verts[0].tex_id + tex_id_start, 0);

    #undef vtx0
    #undef vtx1
    #undef vtx2
    return;
}

static inline void subdivide_once_then_add_tex_triangle(const vertex_3d_t* verts, svec2_t* trans_vec_xy, scalar_t* trans_vec_z, const int sub1_threshold) {
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
    int avg_z_035;
    int avg_z_314;
    int avg_z_5324;
    gte_ldsz3(trans_vec_z[0], trans_vec_z[3], trans_vec_z[5]);
    gte_avsz3();
    gte_stotz(&avg_z_035);
    gte_ldsz3(trans_vec_z[3], trans_vec_z[1], trans_vec_z[4]);
    gte_avsz3();
    gte_stotz(&avg_z_314);
    gte_ldsz4(trans_vec_z[5], trans_vec_z[3], trans_vec_z[2], trans_vec_z[4]);
    gte_avsz4();
    gte_stotz(&avg_z_5324);
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[3], trans_vec_xy[5], verts[0], ab, ca, avg_z_035, verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[3], trans_vec_xy[1], trans_vec_xy[4], ab, verts[1], bc, avg_z_314, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[5], trans_vec_xy[3], trans_vec_xy[2], trans_vec_xy[4], ca, ab, bc, verts[2], avg_z_5324, verts[0].tex_id + tex_id_start, 0);

    // Filler triangles
    scalar_t max_z = trans_vec_z[0];
    if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
    if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2]; 
    max_z >>= 2;

    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[3], verts[0], verts[1], ab, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[4], verts[1], verts[2], bc, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[2], trans_vec_xy[0], trans_vec_xy[5], verts[2], verts[0], ca, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
}

// Transform and add to queue a textured, shaded triangle, with automatic subdivision based on size, fading to solid color in the distance.
static inline void draw_tex_triangle3d_fancy(const vertex_3d_t* verts) {
    // Transform the first 3 vertices
    struct scratchpad {
        svec2_t trans_vec_xy[15];
        scalar_t trans_vec_z[15];
    };

    struct scratchpad* sp = (struct scratchpad*)SCRATCHPAD;

    // Transform the 3 vertices
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
    scalar_t avg_z;
    gte_stsxy3c(&sp->trans_vec_xy[0]);
    gte_stsz3c(&sp->trans_vec_z[0]);
    gte_avsz3();
    gte_stotz(&avg_z);
    
    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || (avg_z <= 0)) return;

    // Cull if off screen
    int n_left = 1;
    int n_right = 1;
    int n_up = 1;
    int n_down = 1;
    for (size_t i = 0; i < 3; ++i) {
        if (sp->trans_vec_xy[i].x < 0) n_left <<= 1;
        if (sp->trans_vec_xy[i].x > res_x) n_right <<= 1;
        if (sp->trans_vec_xy[i].y < 0) n_up <<= 1;
        if (sp->trans_vec_xy[i].y > curr_res_y ) n_down <<= 1;
    }
    // If all vertices are off to one side of the screen, one of the values will be shifted left 3x.
    // When OR-ing the bits together, the result will be %00001???.
    // Shifting that value to the right 3x drops off the irrelevant ??? bits, returning 1 if all vertices are to one side, and 0 if not
    const int off_to_one_side = (n_left | n_right | n_up | n_down) >> 3;
    if (off_to_one_side) return;

    // If very close, subdivide twice
    const scalar_t sub2_threshold = TRI_THRESHOLD_MUL_SUB2 * (int32_t)verts[1].tex_id;
    if (avg_z < sub2_threshold) {
        subdivide_twice_then_add_tex_triangle(verts, sp->trans_vec_xy, sp->trans_vec_z, sub2_threshold);
        return;
    }
    
    // If close, subdivice once
    const scalar_t sub1_threshold = TRI_THRESHOLD_MUL_SUB1 * (int32_t)verts[1].tex_id;
    if (avg_z < sub1_threshold) {
        subdivide_once_then_add_tex_triangle(verts, sp->trans_vec_xy, sp->trans_vec_z, sub1_threshold);
        return;
    }

    // If normal distance, add triangle normally
    if (avg_z < TRI_THRESHOLD_FADE_END) {
        // Calculate CLUT fade
        int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
        if (clut_fade > (N_CLUT_FADES-1)) clut_fade = (N_CLUT_FADES-1);
        else if (clut_fade < 0) clut_fade = 0;
        add_tex_triangle(sp->trans_vec_xy[0], sp->trans_vec_xy[1], sp->trans_vec_xy[2], verts[0], verts[1], verts[2], avg_z, verts[0].tex_id + tex_id_start, clut_fade);
        return;
    }

    // If far away, calculate colors and add untextured triangle
    else {
        add_untex_triangle(sp->trans_vec_xy[0], sp->trans_vec_xy[1], sp->trans_vec_xy[2], verts[0], verts[1], verts[2], avg_z);
    }
}

// Transform and add to queue a textured, shaded triangle, ignoring the fading and subdivision
static inline void draw_tex_triangle3d_fast(const vertex_3d_t* verts) {
    // Transform the 3 vertices
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
    svec2_t trans_vec_xy[3];
    scalar_t avg_z;
    gte_stsxy3c(&trans_vec_xy[0]);
    gte_avsz3();
    gte_stotz(&avg_z);
    
    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || (avg_z <= 0)) return;

    // Cull if off screen
    int n_left = 1;
    int n_right = 1;
    int n_up = 1;
    int n_down = 1;
    for (size_t i = 0; i < 3; ++i) {
        if (trans_vec_xy[i].x < 0) n_left <<= 1;
        if (trans_vec_xy[i].x > res_x) n_right <<= 1;
        if (trans_vec_xy[i].y < 0) n_up <<= 1;
        if (trans_vec_xy[i].y > curr_res_y ) n_down <<= 1;
    }
    // If all vertices are off to one side of the screen, one of the values will be shifted left 3x.
    // When OR-ing the bits together, the result will be %00001???.
    // Shifting that value to the right 3x drops off the irrelevant ??? bits, returning 1 if all vertices are to one side, and 0 if not
    const int off_to_one_side = (n_left | n_right | n_up | n_down) >> 3;
    if (off_to_one_side) return;

    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], verts[0], verts[1], verts[2], avg_z, verts[0].tex_id, 0);
}

static inline void subdivide_twice_then_add_tex_quad(const vertex_3d_t* verts, svec2_t* trans_vec_xy, scalar_t* trans_vec_z, int sub2_threshold) {
    // Generate all 25 vertices
    #define vtx0 verts[0]
    #define vtx1 verts[1]
    #define vtx2 verts[2]
    #define vtx3 verts[3]
    const vertex_3d_t vtx4 = get_halfway_point(vtx0, vtx1);
    const vertex_3d_t vtx5 = get_halfway_point(vtx2, vtx3);
    const vertex_3d_t vtx6 = get_halfway_point(vtx4, vtx5);
    gte_ldv3(&vtx4.x, &vtx5.x, &vtx6.x);
    gte_rtpt();
    const vertex_3d_t vtx7 = get_halfway_point(vtx0, vtx2);
    const vertex_3d_t vtx8 = get_halfway_point(vtx1, vtx3);
    const vertex_3d_t vtx9 = get_halfway_point(vtx0, vtx4);
    gte_stsxy3c(&trans_vec_xy[4]);
    gte_stsz3c(&trans_vec_z[4]);
    gte_ldv3(&vtx7.x, &vtx8.x, &vtx9.x);
    gte_rtpt();
    const vertex_3d_t vtx10 = get_halfway_point(vtx4, vtx1);
    const vertex_3d_t vtx11 = get_halfway_point(vtx7, vtx6);
    const vertex_3d_t vtx12 = get_halfway_point(vtx6, vtx8);
    gte_stsxy3c(&trans_vec_xy[7]);
    gte_stsz3c(&trans_vec_z[7]);
    gte_ldv3(&vtx10.x, &vtx11.x, &vtx12.x);
    gte_rtpt();
    const vertex_3d_t vtx13 = get_halfway_point(vtx2, vtx5);
    const vertex_3d_t vtx14 = get_halfway_point(vtx5, vtx3);
    const vertex_3d_t vtx15 = get_halfway_point(vtx0, vtx7);
    gte_stsxy3c(&trans_vec_xy[10]);
    gte_stsz3c(&trans_vec_z[10]);
    gte_ldv3(&vtx13.x, &vtx14.x, &vtx15.x);
    gte_rtpt();
    const vertex_3d_t vtx16 = get_halfway_point(vtx9, vtx11);
    const vertex_3d_t vtx17 = get_halfway_point(vtx4, vtx6);
    const vertex_3d_t vtx18 = get_halfway_point(vtx10, vtx12);
    gte_stsxy3c(&trans_vec_xy[13]);
    gte_stsz3c(&trans_vec_z[13]);
    gte_ldv3(&vtx16.x, &vtx17.x, &vtx18.x);
    gte_rtpt();
    const vertex_3d_t vtx19 = get_halfway_point(vtx1, vtx8);
    const vertex_3d_t vtx20 = get_halfway_point(vtx7, vtx2);
    const vertex_3d_t vtx21 = get_halfway_point(vtx11, vtx13);
    gte_stsxy3c(&trans_vec_xy[16]);
    gte_stsz3c(&trans_vec_z[16]);
    gte_ldv3(&vtx19.x, &vtx20.x, &vtx21.x);
    gte_rtpt();
    const vertex_3d_t vtx22 = get_halfway_point(vtx6, vtx5);
    const vertex_3d_t vtx23 = get_halfway_point(vtx12, vtx14);
    const vertex_3d_t vtx24 = get_halfway_point(vtx8, vtx3);
    gte_stsxy3c(&trans_vec_xy[19]);
    gte_stsz3c(&trans_vec_z[19]);
    gte_ldv3(&vtx22.x, &vtx23.x, &vtx24.x);
    gte_rtpt();
    gte_stsxy3c(&trans_vec_xy[22]);
    gte_stsz3c(&trans_vec_z[22]);

    // Calculate average Z values    
    int avg_z_0_9_15_16;
    int avg_z_9_4_16_17;
    int avg_z_4_10_17_18;
    int avg_z_10_1_18_19;
    int avg_z_15_16_7_11;
    int avg_z_16_17_11_6;
    int avg_z_17_18_6_12;
    int avg_z_18_19_12_8;
    int avg_z_7_11_20_21;
    int avg_z_11_6_21_22;
    int avg_z_6_12_22_23;
    int avg_z_12_8_23_24;
    int avg_z_20_21_2_13;
    int avg_z_21_22_13_5;
    int avg_z_22_23_5_14;
    int avg_z_23_24_14_3;
    gte_ldsz4(trans_vec_z[0], trans_vec_z[9], trans_vec_z[15], trans_vec_z[16]);
    gte_avsz4();
    gte_stotz(&avg_z_0_9_15_16);
    gte_ldsz4(trans_vec_z[9], trans_vec_z[4], trans_vec_z[16], trans_vec_z[17]);
    gte_avsz4();
    gte_stotz(&avg_z_9_4_16_17);
    gte_ldsz4(trans_vec_z[4], trans_vec_z[10], trans_vec_z[17], trans_vec_z[18]);
    gte_avsz4();
    gte_stotz(&avg_z_4_10_17_18);
    gte_ldsz4(trans_vec_z[10], trans_vec_z[1], trans_vec_z[18], trans_vec_z[19]);
    gte_avsz4();
    gte_stotz(&avg_z_10_1_18_19);
    gte_ldsz4(trans_vec_z[15], trans_vec_z[16], trans_vec_z[7], trans_vec_z[11]);
    gte_avsz4();
    gte_stotz(&avg_z_15_16_7_11);
    gte_ldsz4(trans_vec_z[16], trans_vec_z[17], trans_vec_z[11], trans_vec_z[6]);
    gte_avsz4();
    gte_stotz(&avg_z_16_17_11_6);
    gte_ldsz4(trans_vec_z[17], trans_vec_z[18], trans_vec_z[6], trans_vec_z[12]);
    gte_avsz4();
    gte_stotz(&avg_z_17_18_6_12);
    gte_ldsz4(trans_vec_z[18], trans_vec_z[19], trans_vec_z[12], trans_vec_z[8]);
    gte_avsz4();
    gte_stotz(&avg_z_18_19_12_8);
    gte_ldsz4(trans_vec_z[7], trans_vec_z[11], trans_vec_z[20], trans_vec_z[21]);
    gte_avsz4();
    gte_stotz(&avg_z_7_11_20_21);
    gte_ldsz4(trans_vec_z[11], trans_vec_z[6], trans_vec_z[21], trans_vec_z[22]);
    gte_avsz4();
    gte_stotz(&avg_z_11_6_21_22);
    gte_ldsz4(trans_vec_z[6], trans_vec_z[12], trans_vec_z[22], trans_vec_z[23]);
    gte_avsz4();
    gte_stotz(&avg_z_6_12_22_23);
    gte_ldsz4(trans_vec_z[12], trans_vec_z[8], trans_vec_z[23], trans_vec_z[24]);
    gte_avsz4();
    gte_stotz(&avg_z_12_8_23_24);
    gte_ldsz4(trans_vec_z[20], trans_vec_z[21], trans_vec_z[2], trans_vec_z[13]);
    gte_avsz4();
    gte_stotz(&avg_z_20_21_2_13);
    gte_ldsz4(trans_vec_z[21], trans_vec_z[22], trans_vec_z[13], trans_vec_z[5]);
    gte_avsz4();
    gte_stotz(&avg_z_21_22_13_5);
    gte_ldsz4(trans_vec_z[22], trans_vec_z[23], trans_vec_z[5], trans_vec_z[14]);
    gte_avsz4();
    gte_stotz(&avg_z_22_23_5_14);
    gte_ldsz4(trans_vec_z[23], trans_vec_z[24], trans_vec_z[14], trans_vec_z[3]);
    gte_avsz4();
    gte_stotz(&avg_z_23_24_14_3);


    // Add to queue
    add_tex_quad(trans_vec_xy[0], trans_vec_xy[9], trans_vec_xy[15], trans_vec_xy[16], vtx0, vtx9, vtx15, vtx16, avg_z_0_9_15_16, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[9], trans_vec_xy[4], trans_vec_xy[16], trans_vec_xy[17], vtx9, vtx4, vtx16, vtx17, avg_z_9_4_16_17, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[4], trans_vec_xy[10], trans_vec_xy[17], trans_vec_xy[18], vtx4, vtx10, vtx17, vtx18, avg_z_4_10_17_18, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[10], trans_vec_xy[1], trans_vec_xy[18], trans_vec_xy[19], vtx10, vtx1, vtx18, vtx19, avg_z_10_1_18_19, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[15], trans_vec_xy[16], trans_vec_xy[7], trans_vec_xy[11], vtx15, vtx16, vtx7, vtx11, avg_z_15_16_7_11, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[16], trans_vec_xy[17], trans_vec_xy[11], trans_vec_xy[6], vtx16, vtx17, vtx11, vtx6, avg_z_16_17_11_6, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[17], trans_vec_xy[18], trans_vec_xy[6], trans_vec_xy[12], vtx17, vtx18, vtx6, vtx12, avg_z_17_18_6_12, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[18], trans_vec_xy[19], trans_vec_xy[12], trans_vec_xy[8], vtx18, vtx19, vtx12, vtx8, avg_z_18_19_12_8, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[7], trans_vec_xy[11], trans_vec_xy[20], trans_vec_xy[21], vtx7, vtx11, vtx20, vtx21, avg_z_7_11_20_21, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[11], trans_vec_xy[6], trans_vec_xy[21], trans_vec_xy[22], vtx11, vtx6, vtx21, vtx22, avg_z_11_6_21_22, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[6], trans_vec_xy[12], trans_vec_xy[22], trans_vec_xy[23], vtx6, vtx12, vtx22, vtx23, avg_z_6_12_22_23, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[12], trans_vec_xy[8], trans_vec_xy[23], trans_vec_xy[24], vtx12, vtx8, vtx23, vtx24, avg_z_12_8_23_24, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[20], trans_vec_xy[21], trans_vec_xy[2], trans_vec_xy[13], vtx20, vtx21, vtx2, vtx13, avg_z_20_21_2_13, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[21], trans_vec_xy[22], trans_vec_xy[13], trans_vec_xy[5], vtx21, vtx22, vtx13, vtx5, avg_z_21_22_13_5, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[22], trans_vec_xy[23], trans_vec_xy[5], trans_vec_xy[14], vtx22, vtx23, vtx5, vtx14, avg_z_22_23_5_14, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[23], trans_vec_xy[24], trans_vec_xy[14], trans_vec_xy[3], vtx23, vtx24, vtx14, vtx3, avg_z_23_24_14_3, verts[0].tex_id + tex_id_start, 0);

    // Filler triangles
    scalar_t max_z = trans_vec_z[0];
    if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
    if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2]; 
    if (trans_vec_z[3] > max_z) max_z = trans_vec_z[3];
    max_z >>= 2;

    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[4], vtx0, vtx1, vtx4, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[4], trans_vec_xy[9], vtx0, vtx4, vtx9, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[4], trans_vec_xy[1], trans_vec_xy[10], vtx4, vtx1, vtx10, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[1], trans_vec_xy[3], trans_vec_xy[8], vtx1, vtx3, vtx8, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[1], trans_vec_xy[8], trans_vec_xy[19], vtx1, vtx8, vtx19, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[8], trans_vec_xy[3], trans_vec_xy[24], vtx8, vtx3, vtx24, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[3], trans_vec_xy[2], trans_vec_xy[5], vtx3, vtx2, vtx5, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[3], trans_vec_xy[5], trans_vec_xy[14], vtx3, vtx5, vtx14, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[5], trans_vec_xy[2], trans_vec_xy[13], vtx5, vtx2, vtx13, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[2], trans_vec_xy[0], trans_vec_xy[7], vtx2, vtx0, vtx7, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[2], trans_vec_xy[7], trans_vec_xy[20], vtx2, vtx7, vtx20, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[7], trans_vec_xy[0], trans_vec_xy[15], vtx7, vtx0, vtx15, (max_z + 1), verts[0].tex_id + tex_id_start, 0);

    #undef vtx0
    #undef vtx1
    #undef vtx2
    #undef vtx3
    return;
}

static inline void subdivide_once_then_add_tex_quad(const vertex_3d_t* verts, svec2_t* trans_vec_xy, scalar_t* trans_vec_z, const int sub1_threshold) {
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
    add_tex_quad(trans_vec_xy[0], trans_vec_xy[4], trans_vec_xy[7], trans_vec_xy[8], verts[0], ab, da, center, avg_z_0478, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[4], trans_vec_xy[1], trans_vec_xy[8], trans_vec_xy[5], ab, verts[1], center, bc, avg_z_4185, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[7], trans_vec_xy[8], trans_vec_xy[2], trans_vec_xy[6], da, center, verts[2], cd, avg_z_7826, verts[0].tex_id + tex_id_start, 0);
    add_tex_quad(trans_vec_xy[8], trans_vec_xy[5], trans_vec_xy[6], trans_vec_xy[3], center, bc, cd, verts[3], avg_z_8563, verts[0].tex_id + tex_id_start, 0);

    // Filler triangles
    scalar_t max_z = trans_vec_z[0];
    if (trans_vec_z[1] > max_z) max_z = trans_vec_z[1];
    if (trans_vec_z[2] > max_z) max_z = trans_vec_z[2];
    if (trans_vec_z[3] > max_z) max_z = trans_vec_z[3];
    max_z >>= 2;

    add_tex_triangle(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[4], verts[0], verts[1], ab, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[1], trans_vec_xy[3], trans_vec_xy[5], verts[1], verts[3], bc, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[3], trans_vec_xy[2], trans_vec_xy[6], verts[3], verts[2], cd, (max_z + 1), verts[0].tex_id + tex_id_start, 0);
    add_tex_triangle(trans_vec_xy[2], trans_vec_xy[0], trans_vec_xy[7], verts[2], verts[0], da, (max_z + 1), verts[0].tex_id + tex_id_start, 0);

    return;
}

// Transform and add to queue a textured, shaded quad, with automatic subdivision based on size, fading to solid color in the distance.
static inline void draw_tex_quad3d_fancy(const vertex_3d_t* verts) {
    // Transform the first 3 vertices
    struct scratchpad {
        svec2_t trans_vec_xy[25];
        scalar_t trans_vec_z[25];
    };

    struct scratchpad* sp = (struct scratchpad*)SCRATCHPAD;

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
    gte_stsxy3c(&sp->trans_vec_xy[0]);
    gte_stsz3c(&sp->trans_vec_z[0]);
    gte_ldv0(&verts[3]);
    gte_rtps();
    gte_stsxy(&sp->trans_vec_xy[3]);
    gte_stsz(&sp->trans_vec_z[3]);
    gte_avsz4();
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || ((avg_z >> 0) <= 0)) return;

    // Cull if off screen
    int n_left = 1;
    int n_right = 1;
    int n_up = 1;
    int n_down = 1;
    for (size_t i = 0; i < 4; ++i) {
        if (sp->trans_vec_xy[i].x < 0) n_left <<= 1;
        if (sp->trans_vec_xy[i].x > res_x) n_right <<= 1;
        if (sp->trans_vec_xy[i].y < 0) n_up <<= 1;
        if (sp->trans_vec_xy[i].y > curr_res_y ) n_down <<= 1;
    }
    // If all vertices are off to one side of the screen, one of the values will be shifted left 4x.
    // When OR-ing the bits together, the result will be %0001????.
    // Shifting that value to the right 4x drops off the irrelevant ???? bits, returning 1 if all vertices are to one side, and 0 if not
    const int off_to_one_side = (n_left | n_right | n_up | n_down) >> 4;
    if (off_to_one_side) return;
    
    // If very close, subdivide twice
    const scalar_t sub2_threshold = TRI_THRESHOLD_MUL_SUB2 * (int32_t)verts[1].tex_id;
    if (avg_z < sub2_threshold) {
        subdivide_twice_then_add_tex_quad(verts, sp->trans_vec_xy, sp->trans_vec_z, sub2_threshold);
        return;
    }
    
    // If close, subdivice once
    const scalar_t sub1_threshold = TRI_THRESHOLD_MUL_SUB1 * (int32_t)verts[1].tex_id;
    if (avg_z < sub1_threshold) {
        subdivide_once_then_add_tex_quad(verts, sp->trans_vec_xy, sp->trans_vec_z, sub1_threshold);
        return;
    }

    // If normal distance, add quad normally
    if (avg_z < TRI_THRESHOLD_FADE_END) {
        // Calculate CLUT fade
        int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
        if (clut_fade > (N_CLUT_FADES-1)) clut_fade = (N_CLUT_FADES-1);
        else if (clut_fade < 0) clut_fade = 0;
        add_tex_quad(sp->trans_vec_xy[0], sp->trans_vec_xy[1], sp->trans_vec_xy[2], sp->trans_vec_xy[3], verts[0], verts[1], verts[2], verts[3], avg_z, verts[0].tex_id + tex_id_start, clut_fade);
        return;
    }

    // If far away, calculate colors and add untextured quad
    else {
        add_untex_quad(sp->trans_vec_xy[0], sp->trans_vec_xy[1], sp->trans_vec_xy[2], sp->trans_vec_xy[3], verts[0], verts[1], verts[2], verts[3], avg_z);
    }
}

// Transform and add to queue a textured, shaded quad, ignoring the fading and subdivision
static inline void draw_tex_quad3d_fast(const vertex_3d_t* verts) {
    // Transform the first 3 vertices
    svec2_t trans_vec_xy[4];
    scalar_t trans_vec_z[4];
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
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || ((avg_z + curr_ot_bias) <= 0)) return;

    // Cull if off screen
    int n_left = 1;
    int n_right = 1;
    int n_up = 1;
    int n_down = 1;
    for (size_t i = 0; i < 4; ++i) {
        if (trans_vec_xy[i].x < 0) n_left <<= 1;
        if (trans_vec_xy[i].x > res_x) n_right <<= 1;
        if (trans_vec_xy[i].y < 0) n_up <<= 1;
        if (trans_vec_xy[i].y > curr_res_y ) n_down <<= 1;
    }
    // If all vertices are off to one side of the screen, one of the values will be shifted left 4x.
    // When OR-ing the bits together, the result will be %0001????.
    // Shifting that value to the right 4x drops off the irrelevant ???? bits, returning 1 if all vertices are to one side, and 0 if not
    const int off_to_one_side = (n_left | n_right | n_up | n_down) >> 4;
    if (off_to_one_side) return;

    add_tex_quad(trans_vec_xy[0], trans_vec_xy[1], trans_vec_xy[2], trans_vec_xy[3], verts[0], verts[1], verts[2], verts[3], avg_z, verts[0].tex_id, 0);
}