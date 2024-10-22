#include <psxgte.h>
#include <inline_c.h>

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

// Vertically the primitive's CLUT to 16 pixels, and then add the fade level to the Y coordinate
#define SET_DISTANCE_FADE(primitive, fade_level) \
    (primitive)->clut &= 0b111110000111111; \
    (primitive)->clut |= fade_level << 6;

#define gte_stsz01( r0, r1 ) __asm__ volatile ( \
	"swc2	$17, 0( %0 );"	\
	"swc2	$18, 0( %1 );"	\
	:						\
	: "r"( r0 ), "r"( r1 )	\
	: "memory" )

// Also overwrites the primitive code, so do this before calling setPolyGT3() etc
#define COPY_COLOR(src, dest) *(uint32_t*)dest = *(uint32_t*)src;

#define COPY_POS(src, dest) *(uint32_t*)dest = *(uint32_t*)src;
#define COPY_UV(src, dest) *(uint16_t*)dest = *(uint16_t*)src;

static inline void halfway_rgb_uv(uint8_t const *const src_a, uint8_t const *const src_b, uint8_t* dest) {
    // hacky, but works specifically for GT3 and GT4's memory layouts
    struct rgb_uv_t {
        uint8_t r, g, b, code;
        uint16_t x, y;
        uint8_t u, v;
    };

    // lmao so many consts
    struct rgb_uv_t const *const src_a_cast = (struct rgb_uv_t const *const)src_a; 
    struct rgb_uv_t const *const src_b_cast = (struct rgb_uv_t const *const)src_b; 
    struct rgb_uv_t* dest_cast = (struct rgb_uv_t*)dest; 
    dest_cast->r = (src_a_cast->r / 2) + (src_b_cast->r / 2);
    dest_cast->g = (src_a_cast->g / 2) + (src_b_cast->g / 2);
    dest_cast->b = (src_a_cast->b / 2) + (src_b_cast->b / 2);
    dest_cast->u = (src_a_cast->u / 2) + (src_b_cast->u / 2);
    dest_cast->v = (src_a_cast->v / 2) + (src_b_cast->v / 2);
}

static inline void copy_rgb_uv(uint8_t const *const src, uint8_t* dest) {
    // hacky, but works specifically for GT3 and GT4's memory layouts
    struct rgb_uv_t {
        uint8_t r, g, b, code;
        uint16_t x, y;
        uint8_t u, v;
    };

    struct rgb_uv_t const *const src_cast = (struct rgb_uv_t const *const)src; 
    struct rgb_uv_t* dest_cast = (struct rgb_uv_t*)dest; 
    dest_cast->r = src_cast->r;
    dest_cast->g = src_cast->g;
    dest_cast->b = src_cast->b;
    dest_cast->u = src_cast->u;
    dest_cast->v = src_cast->v;
}

static inline int frustrum_cull_aabb(const vec3_t min, const vec3_t max) {
    return 0;
    // We will transform these to screen space
    aligned_position_t vertices[8] = {
        ((aligned_position_t) {.x = min.x, .y = min.y, .z = min.z}),
        ((aligned_position_t) {.x = min.x, .y = min.y, .z = max.z}),
        ((aligned_position_t) {.x = min.x, .y = max.y, .z = min.z}),
        ((aligned_position_t) {.x = min.x, .y = max.y, .z = max.z}),
        ((aligned_position_t) {.x = max.x, .y = min.y, .z = min.z}),
        ((aligned_position_t) {.x = max.x, .y = min.y, .z = max.z}),
        ((aligned_position_t) {.x = max.x, .y = max.y, .z = min.z}),
        ((aligned_position_t) {.x = max.x, .y = max.y, .z = max.z}),
    };

    // Transform vertices and store them back
    // 0, 1, 2
    gte_ldv3(&vertices[0].x, &vertices[1].x, &vertices[2].x);
    gte_rtpt();
    gte_stsxy3(&vertices[0].x, &vertices[1].x, &vertices[2].x);
    gte_stsz3(&vertices[0].z, &vertices[1].z, &vertices[2].z);
    // 3, 4, 5
    gte_ldv3(&vertices[3].x, &vertices[4].x, &vertices[5].x);
    gte_rtpt();
    gte_stsxy3(&vertices[3].x, &vertices[4].x, &vertices[5].x);
    gte_stsz3(&vertices[3].z, &vertices[4].z, &vertices[5].z);
    // 6, 7
    gte_ldv01(&vertices[6].x, &vertices[7].x);
    gte_rtpt();
    gte_stsxy01(&vertices[6].x, &vertices[7].x);
    gte_stsz01(&vertices[6].z, &vertices[7].z);

    // Find screen aligned bounding box
    aligned_position_t after_min = (aligned_position_t){.x = INT16_MAX, .y = INT16_MAX, .z = INT16_MAX};
    aligned_position_t after_max = (aligned_position_t){.x = INT16_MIN, .y = INT16_MIN, .z = INT16_MIN};
    
    for (size_t i = 0; i < 8; ++i) {
        if (vertices[i].x < after_min.x) after_min.x = vertices[i].x;
        if (vertices[i].x > after_max.x) after_max.x = vertices[i].x;
        if (vertices[i].y < after_min.y) after_min.y = vertices[i].y;
        if (vertices[i].y > after_max.y) after_max.y = vertices[i].y;
        if (vertices[i].z < after_min.z) after_min.z = vertices[i].z;
        if (vertices[i].z > after_max.z) after_max.z = vertices[i].z;
    }

    // If this screen aligned bounding box is off screen, do not draw this mesh
    #define FRUSCUL_PAD_X 0 // these are in case I ever want to add a safe zone
    #define FRUSCUL_PAD_Y 0 // or for debugging.
    if (after_max.x < 0+FRUSCUL_PAD_X) return 1; // mesh is to the left of the screen
    if (after_max.y < 0+FRUSCUL_PAD_Y) return 1; // mesh is above the screen
    if (after_max.z == 0) return 1; // mesh is behind the screen
    if (after_min.z > MESH_RENDER_DISTANCE) return 1; // mesh is too far away
    if (after_min.x > res_x-FRUSCUL_PAD_X) return 1; // mesh is to the right of the screen
    if (after_min.y > curr_res_y-FRUSCUL_PAD_Y) return 1; // mesh is below the screen
    #undef FRUSCUL_PAD_X
    #undef FRUSCUL_PAD_Y
    return 0;
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

static inline aligned_position_t get_halfway_position(const aligned_position_t v0, const aligned_position_t v1) {
    return (aligned_position_t) {
        .x = v0.x + ((v1.x - v0.x) >> 1),
        .y = v0.y + ((v1.y - v0.y) >> 1),
        .z = v0.z + ((v1.z - v0.z) >> 1),
    };
}

void draw_level1_subdivided_triangle(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx, scalar_t otz) {
    // 0-----1       0--3--1
    // |    /        |  | /
    // |  /     -->  5--4    + filler at the edges to fix gaps
    // |/            | /
    // 2             2
    //  
    // Subdivided triangle
    POLY_GT4* quad_0354 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT3* tri_314 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_542 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Filler triangles
    POLY_GT3* tri_250 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_031 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_142 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Store first 3 vertices' positions - we still have those in the GTE registers
    gte_stsxy0(&quad_0354->x0); // XY0
    gte_stsxy0(&tri_250->x2);
    gte_stsxy0(&tri_031->x0);
    gte_stsxy1(&tri_314->x1); // XY1
    gte_stsxy1(&tri_031->x2);
    gte_stsxy1(&tri_142->x0);
    gte_stsxy2(&tri_542->x2); // XY2
    gte_stsxy2(&tri_250->x0);
    gte_stsxy2(&tri_142->x2);

    // Generate extra positions for subdivision
    const aligned_position_t v3 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 0], mesh->vtx_pos_and_size[vert_idx + 1]);
    const aligned_position_t v4 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 1], mesh->vtx_pos_and_size[vert_idx + 2]);
    const aligned_position_t v5 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 2], mesh->vtx_pos_and_size[vert_idx + 0]);

    // Start transforming them - we'll get back here later
    gte_ldv3(&v3.x, &v4.x, &v5.x);
    gte_rtpt();

    // Copy over the vertex attributes we already have
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &quad_0354->r0); // v0
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &tri_250->r2);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &tri_031->r0);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri_314->r1); // v1
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri_031->r2);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri_142->r0);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri_542->r2); // v2
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri_250->r0);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri_142->r2);

    // The rest needs to be interpolated and duplicated
    halfway_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &mesh->tex_tris[0][poly_idx].r1, &quad_0354->r1);
    halfway_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &mesh->tex_tris[0][poly_idx].r2, &quad_0354->r3);
    halfway_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &mesh->tex_tris[0][poly_idx].r0, &quad_0354->r2);
    copy_rgb_uv(&quad_0354->r1, &tri_314->r0); // v3
    copy_rgb_uv(&quad_0354->r1, &tri_031->r1);
    copy_rgb_uv(&quad_0354->r3, &tri_314->r2); // v4
    copy_rgb_uv(&quad_0354->r3, &tri_542->r1);
    copy_rgb_uv(&quad_0354->r3, &tri_142->r1);
    copy_rgb_uv(&quad_0354->r2, &tri_542->r0); // v5
    copy_rgb_uv(&quad_0354->r2, &tri_250->r1);

    // Oh right we still have the positions in the GTE registers
    gte_stsxy0(&quad_0354->x1); // XY3
    gte_stsxy0(&tri_314->x0);
    gte_stsxy0(&tri_031->x1);
    gte_stsxy1(&quad_0354->x3); // XY4
    gte_stsxy1(&tri_314->x2);
    gte_stsxy1(&tri_542->x1);
    gte_stsxy1(&tri_142->x1);
    gte_stsxy2(&quad_0354->x2); // XY5
    gte_stsxy2(&tri_542->x0);
    gte_stsxy2(&tri_250->x1);

    // Copy constants
    const uint16_t clut = mesh->tex_tris[0][poly_idx].clut & 0b111110000111111; // reset distance fade palette index
    const uint16_t tpage = mesh->tex_tris[0][poly_idx].tpage;
    quad_0354->clut = clut;   quad_0354->tpage = tpage;
    tri_314->clut = clut;     tri_314->tpage = tpage;
    tri_542->clut = clut;     tri_542->tpage = tpage;
    tri_250->clut = clut;     tri_250->tpage = tpage;
    tri_031->clut = clut;     tri_031->tpage = tpage;
    tri_142->clut = clut;     tri_142->tpage = tpage;

    // Add to ordering table for rendering
    setPolyGT4(quad_0354);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_0354);
    setPolyGT3(tri_314);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_314);
    setPolyGT3(tri_542);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_542);
    setPolyGT3(tri_250);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_250);
    setPolyGT3(tri_031);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_031);
    setPolyGT3(tri_142);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_142);
}

void draw_level2_subdivided_triangle(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx, scalar_t otz) {
    // 0-----------1      0--3--4--5--1
    // |          /       |  |  |  | / 
    // |        /         6--7--8--9
    // |       /          |  |  | /
    // |     /       -->  A--B--C   + filler at the edges to fix gaps
    // |    /             |  | /
    // |  /               D--E
    // | /                | /
    // 2                  2

    // Subdivided triangle
    POLY_GT4* quad_0367 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_3478 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_4589 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_67AB = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_78BC = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_ABDE = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT3* tri_519 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_89C = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_BCE = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_DE2 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Filler triangles
    POLY_GT3* tri_043 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_415 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_C91 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_2EC = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_2DA = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_A60 = (POLY_GT3*)next_primitive;      next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Get all the interpolated positions
    #define v0 mesh->vtx_pos_and_size[vert_idx + 0]
    #define v1 mesh->vtx_pos_and_size[vert_idx + 1]
    #define v2 mesh->vtx_pos_and_size[vert_idx + 2]
    const aligned_position_t v4 = get_halfway_position(v0, v1);
    const aligned_position_t vC = get_halfway_position(v1, v2);
    const aligned_position_t vA = get_halfway_position(v2, v0);
    const aligned_position_t v3 = get_halfway_position(v4, v0);
    const aligned_position_t v5 = get_halfway_position(v4, v1);
    const aligned_position_t v9 = get_halfway_position(vC, v1);
    const aligned_position_t vE = get_halfway_position(vC, v2);
    const aligned_position_t vD = get_halfway_position(vA, v2);
    const aligned_position_t v6 = get_halfway_position(vA, v0);
    const aligned_position_t v8 = get_halfway_position(vC, v4);
    const aligned_position_t vB = get_halfway_position(vA, vC);
    const aligned_position_t v7 = get_halfway_position(vC, v0);
    #undef v0
    #undef v1
    #undef v2

    // Store 012 - we still have those in the GTE registers
    gte_stsxy0(&quad_0367->x0); // XY 0
    gte_stsxy0(&tri_043->x0);        
    gte_stsxy0(&tri_A60->x2);        
    gte_stsxy1(&tri_519->x1);   // XY 1    
    gte_stsxy1(&tri_415->x1);        
    gte_stsxy1(&tri_C91->x2);        
    gte_stsxy2(&tri_DE2->x2);   // XY 2    
    gte_stsxy2(&tri_2EC->x0);        
    gte_stsxy2(&tri_2DA->x0);        

    // Start transform 345
    gte_ldv3(&v3.x, &v4.x, &v5.x);
    gte_rtpt();

    // Copy attributes for 012
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &quad_0367->r0); // v0
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &tri_043->r0);             
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &tri_A60->r2);             
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri_519->r1);   // v1
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri_415->r1);             
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri_C91->r2);             
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri_DE2->r2);   // v2
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri_2EC->r0);             
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri_2DA->r0);             

    // Store transformed 345
    gte_stsxy0(&quad_0367->x1); // XY 3
    gte_stsxy0(&quad_3478->x0);     
    gte_stsxy0(&tri_043->x2);       
    gte_stsxy1(&quad_3478->x1); // XY 4
    gte_stsxy1(&quad_4589->x0);     
    gte_stsxy1(&tri_043->x1);       
    gte_stsxy1(&tri_415->x0);       
    gte_stsxy2(&quad_4589->x1); // XY 5
    gte_stsxy2(&tri_519->x0);       
    gte_stsxy2(&tri_415->x2);       

    // Start transform 678
    gte_ldv3(&v6.x, &v7.x, &v8.x);
    gte_rtpt();

    // For the sake of my own sanity for the rest of this function - goes for any subsequent defines too
    #define vtx_r0 mesh->tex_tris[0][poly_idx].r0
    #define vtx_r1 mesh->tex_tris[0][poly_idx].r1
    #define vtx_r2 mesh->tex_tris[0][poly_idx].r2

    // Interpolate and copy attributes for 4CA
    #define vtx_r4 quad_3478->r1
    #define vtx_rC quad_78BC->r3
    #define vtx_rA quad_67AB->r2
    halfway_rgb_uv(&vtx_r0, &vtx_r1, &quad_3478->r1);  // v4: between 0-1
    halfway_rgb_uv(&vtx_r1, &vtx_r2, &quad_78BC->r3);  // vC: between 1-2
    halfway_rgb_uv(&vtx_r2, &vtx_r0, &quad_67AB->r2);  // vA: between 2-0
    copy_rgb_uv(&vtx_r4, &quad_4589->r0); // v4
    copy_rgb_uv(&vtx_r4, &tri_043->r1);       
    copy_rgb_uv(&vtx_r4, &tri_415->r0);     
    copy_rgb_uv(&vtx_rC, &tri_89C->r2);  // vC
    copy_rgb_uv(&vtx_rC, &tri_BCE->r1);
    copy_rgb_uv(&vtx_rC, &tri_C91->r0);
    copy_rgb_uv(&vtx_rC, &tri_2EC->r2);
    copy_rgb_uv(&vtx_rA, &quad_ABDE->r0); // vA   
    copy_rgb_uv(&vtx_rA, &tri_2DA->r2);       
    copy_rgb_uv(&vtx_rA, &tri_A60->r0);

    // Store transformed 678
    gte_stsxy0(&quad_0367->x2);  // XY 6
    gte_stsxy0(&quad_67AB->x0); 
    gte_stsxy0(&tri_A60->x1); 
    gte_stsxy1(&quad_0367->x3);  // XY 7
    gte_stsxy1(&quad_3478->x2); 
    gte_stsxy1(&quad_67AB->x1); 
    gte_stsxy1(&quad_78BC->x0); 
    gte_stsxy2(&quad_3478->x3); // XY 8
    gte_stsxy2(&quad_4589->x2);  
    gte_stsxy2(&quad_78BC->x1); 
    gte_stsxy2(&tri_89C->x0); 

    // Start transform 9AB
    gte_ldv3(&v9.x, &vA.x, &vB.x);
    gte_rtpt();
    
    // Interpolate attributes for 359
    #define vtx_r3 quad_0367->r1
    #define vtx_r5 quad_4589->r1
    #define vtx_r9 quad_4589->r3
    halfway_rgb_uv(&vtx_r0, &vtx_r4, &quad_0367->r1);  // v3: between 0-4
    halfway_rgb_uv(&vtx_r4, &vtx_r1, &quad_4589->r1);  // v5: between 4-1
    halfway_rgb_uv(&vtx_r1, &vtx_rC, &quad_4589->r3);  // v9: between 1-C
    copy_rgb_uv(&vtx_r3, &quad_3478->r0); // v3
    copy_rgb_uv(&vtx_r3, &tri_043->r2);
    copy_rgb_uv(&vtx_r5, &tri_519->r0); // v5
    copy_rgb_uv(&vtx_r5, &tri_415->r2);
    copy_rgb_uv(&vtx_r9, &tri_519->r2); // v9
    copy_rgb_uv(&vtx_r9, &tri_89C->r1);
    copy_rgb_uv(&vtx_r9, &tri_C91->r1);

    // Store transformed 9AB
    gte_stsxy0(&quad_4589->x3); // XY 9
    gte_stsxy0(&tri_519->x2); 
    gte_stsxy0(&tri_89C->x1); 
    gte_stsxy0(&tri_C91->x1); 
    gte_stsxy1(&quad_67AB->x2);  // XY A
    gte_stsxy1(&quad_ABDE->x0); 
    gte_stsxy1(&tri_2DA->x2); 
    gte_stsxy1(&tri_A60->x0); 
    gte_stsxy2(&quad_67AB->x3); // XY B
    gte_stsxy2(&quad_78BC->x2); 
    gte_stsxy2(&quad_ABDE->x1);  
    gte_stsxy2(&tri_BCE->x0); 

    // Start transform CDE
    gte_ldv3(&vC.x, &vD.x, &vE.x);
    gte_rtpt();

    // Interpolate attributes for ED6
    #define vtx_rE quad_ABDE->r3
    #define vtx_rD quad_ABDE->r2
    #define vtx_r6 quad_0367->r2
    halfway_rgb_uv(&vtx_rC, &vtx_r2, &quad_ABDE->r3);  // vE: between C-2
    halfway_rgb_uv(&vtx_r2, &vtx_rA, &quad_ABDE->r2);  // vD: between 2-A
    halfway_rgb_uv(&vtx_rA, &vtx_r0, &quad_0367->r2);  // v6: between A-0
    copy_rgb_uv(&vtx_rE, &tri_BCE->r2); // vE
    copy_rgb_uv(&vtx_rE, &tri_DE2->r1);
    copy_rgb_uv(&vtx_rE, &tri_2EC->r1);
    copy_rgb_uv(&vtx_rD, &tri_DE2->r0); // vD
    copy_rgb_uv(&vtx_rD, &tri_2DA->r1);
    copy_rgb_uv(&vtx_r6, &quad_67AB->r0); // v6
    copy_rgb_uv(&vtx_r6, &tri_A60->r1);

    // Interpolate attributes for 8B7
    #define vtx_r8 quad_3478->r3
    #define vtx_rB quad_67AB->r3
    #define vtx_r7 quad_0367->r3
    halfway_rgb_uv(&vtx_r4, &vtx_rC, &quad_3478->r3);  // v8: between 4-C
    halfway_rgb_uv(&vtx_rA, &vtx_rC, &quad_67AB->r3);  // vB: between A-C
    halfway_rgb_uv(&vtx_r6, &vtx_r8, &quad_0367->r3);  // v7: between 6-8
    copy_rgb_uv(&vtx_r8, &quad_4589->r2); // v8
    copy_rgb_uv(&vtx_r8, &quad_78BC->r1);
    copy_rgb_uv(&vtx_r8, &tri_89C->r0);
    copy_rgb_uv(&vtx_rB, &quad_78BC->r2); // vB
    copy_rgb_uv(&vtx_rB, &quad_ABDE->r1);
    copy_rgb_uv(&vtx_rB, &tri_BCE->r0);
    copy_rgb_uv(&vtx_r7, &quad_3478->r2); // v7
    copy_rgb_uv(&vtx_r7, &quad_67AB->r1);
    copy_rgb_uv(&vtx_r7, &quad_78BC->r0);

    // Store transformed CDE
    gte_stsxy0(&quad_78BC->x3);  // XY C
    gte_stsxy0(&tri_89C->x2); 
    gte_stsxy0(&tri_BCE->x1); 
    gte_stsxy0(&tri_C91->x0); 
    gte_stsxy0(&tri_2EC->x2); 
    gte_stsxy1(&quad_ABDE->x2);  // XY D
    gte_stsxy1(&tri_DE2->x0); 
    gte_stsxy1(&tri_2DA->x1); 
    gte_stsxy2(&quad_ABDE->x3);  // XY E
    gte_stsxy2(&tri_BCE->x2); 
    gte_stsxy2(&tri_DE2->x1); 
    gte_stsxy2(&tri_2EC->x1); 

    // Copy constant fields
    const uint16_t clut = mesh->tex_tris[0][poly_idx].clut & 0b111110000111111; // reset distance fade palette index
    const uint16_t tpage = mesh->tex_tris[0][poly_idx].tpage;
    quad_0367->clut = clut;    quad_0367->tpage = tpage; 
    quad_3478->clut = clut;    quad_3478->tpage = tpage; 
    quad_4589->clut = clut;    quad_4589->tpage = tpage; 
    quad_67AB->clut = clut;    quad_67AB->tpage = tpage; 
    quad_78BC->clut = clut;    quad_78BC->tpage = tpage; 
    quad_ABDE->clut = clut;    quad_ABDE->tpage = tpage; 
    tri_519->clut = clut;      tri_519->tpage = tpage; 
    tri_89C->clut = clut;      tri_89C->tpage = tpage; 
    tri_BCE->clut = clut;      tri_BCE->tpage = tpage; 
    tri_DE2->clut = clut;      tri_DE2->tpage = tpage; 
    tri_043->clut = clut;      tri_043->tpage = tpage; 
    tri_415->clut = clut;      tri_415->tpage = tpage; 
    tri_C91->clut = clut;      tri_C91->tpage = tpage; 
    tri_2EC->clut = clut;      tri_2EC->tpage = tpage; 
    tri_2DA->clut = clut;      tri_2DA->tpage = tpage; 
    tri_A60->clut = clut;      tri_A60->tpage = tpage; 

    #undef vtx_r0
    #undef vtx_r1
    #undef vtx_r2
    #undef vtx_r3
    #undef vtx_r4
    #undef vtx_r5
    #undef vtx_r6
    #undef vtx_r7
    #undef vtx_r8
    #undef vtx_r9
    #undef vtx_rA
    #undef vtx_rB
    #undef vtx_rC
    #undef vtx_rD
    #undef vtx_rE
    #undef vtx_rF

    // Add to ordering table for rendering
    setPolyGT4(quad_0367);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_0367);
    setPolyGT4(quad_3478);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_3478);
    setPolyGT4(quad_4589);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_4589);
    setPolyGT4(quad_67AB);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_67AB);
    setPolyGT4(quad_78BC);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_78BC);
    setPolyGT4(quad_ABDE);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_ABDE);
    setPolyGT3(tri_519);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_519);
    setPolyGT3(tri_89C);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_89C);
    setPolyGT3(tri_BCE);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_BCE);
    setPolyGT3(tri_DE2);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_DE2);
    setPolyGT3(tri_043);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_043);
    setPolyGT3(tri_415);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_415);
    setPolyGT3(tri_C91);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_C91);
    setPolyGT3(tri_2EC);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_2EC);
    setPolyGT3(tri_2DA);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_2DA);
    setPolyGT3(tri_A60);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_A60);
}

void draw_level1_subdivided_quad(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx, scalar_t otz) {
    // 0-----1       0--4--1
    // |     |       |  |  |
    // |     |  -->  5--8--6  + filler at the edges to fix gaps
    // |     |       |  |  |
    // 2-----3       2--7--3

    // Subdivided quad
    POLY_GT4* quad_0458 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_4186 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_5827 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_8673 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);

    // Filler triangles
    POLY_GT3* tri_014 = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_136 = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_327 = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_205 = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Copy position for 0, was stored earlier in `draw_tex_quad3d_fancy()`
    if (store_to_precomp_prims) {
        COPY_POS(&mesh->tex_quads[drawbuffer][poly_idx].x0, &quad_0458->x0); // XY 0
        COPY_POS(&mesh->tex_quads[drawbuffer][poly_idx].x0, &tri_014->x0);
        COPY_POS(&mesh->tex_quads[drawbuffer][poly_idx].x0, &tri_205->x1);
    }
    else {
        COPY_POS(&sxy_storage, &quad_0458->x0); // XY 0
        COPY_POS(&sxy_storage, &tri_014->x0);
        COPY_POS(&sxy_storage, &tri_205->x1);
    }

    // GTE still holds positions 123 - store them
    gte_stsxy0(&quad_4186->x1); // XY 1
    gte_stsxy0(&tri_014->x1);
    gte_stsxy0(&tri_136->x0);
    gte_stsxy1(&quad_5827->x2); // XY 2
    gte_stsxy1(&tri_327->x1);
    gte_stsxy1(&tri_205->x0);
    gte_stsxy2(&quad_8673->x3); // XY 3
    gte_stsxy2(&tri_136->x1);
    gte_stsxy2(&tri_327->x0);

    // Interpolate positions
    const aligned_position_t v4 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 0], mesh->vtx_pos_and_size[vert_idx + 1]);
    const aligned_position_t v5 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 0], mesh->vtx_pos_and_size[vert_idx + 2]);
    const aligned_position_t v6 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 1], mesh->vtx_pos_and_size[vert_idx + 3]);
    const aligned_position_t v7 = get_halfway_position(mesh->vtx_pos_and_size[vert_idx + 2], mesh->vtx_pos_and_size[vert_idx + 3]);
    const aligned_position_t v8 = get_halfway_position(v5, v6);

    // Start transform 456
    gte_ldv3(&v4.x, &v5.x, &v6.x);
    gte_rtpt();

    // Copy attributes for 0123
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r0, &quad_0458->r0); // v0
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r0, &tri_014->r0);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r0, &tri_205->r1);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r1, &quad_4186->r1); // v1
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r1, &tri_014->r1);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r1, &tri_136->r0);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r2, &quad_5827->r2); // v2
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r2, &tri_327->r1);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r2, &tri_205->r0);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r3, &quad_8673->r3); // v3
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r3, &tri_136->r1);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r3, &tri_327->r0);

    // Store positions 456
    gte_stsxy0(&quad_0458->x1); // XY 4
    gte_stsxy0(&quad_4186->x0);
    gte_stsxy0(&tri_014->x2);
    gte_stsxy1(&quad_0458->x2); // XY 5
    gte_stsxy1(&quad_5827->x0);
    gte_stsxy1(&tri_205->x2);
    gte_stsxy2(&quad_4186->x3); // XY 6
    gte_stsxy2(&quad_8673->x1);
    gte_stsxy2(&tri_136->x2);

    // Transform positions 78 - we do RTPT but ignore the last vertex
    gte_ldv01(&v7.x, &v8.x);
    gte_rtpt();

    // Copy attributes for 45678
    halfway_rgb_uv(&mesh->tex_quads[0][poly_idx].r0, &mesh->tex_quads[0][poly_idx].r1, &quad_0458->r1); // XY 4
    copy_rgb_uv(&quad_0458->r1, &quad_4186->r0);
    copy_rgb_uv(&quad_0458->r1, &tri_014->r2);
    halfway_rgb_uv(&mesh->tex_quads[0][poly_idx].r0, &mesh->tex_quads[0][poly_idx].r2, &quad_0458->r2); // XY 5
    copy_rgb_uv(&quad_0458->r2, &quad_5827->r0);
    copy_rgb_uv(&quad_0458->r2, &tri_205->r2);
    halfway_rgb_uv(&mesh->tex_quads[0][poly_idx].r1, &mesh->tex_quads[0][poly_idx].r3, &quad_4186->r3); // XY 6
    copy_rgb_uv(&quad_4186->r3, &quad_8673->r1);
    copy_rgb_uv(&quad_4186->r3, &tri_136->r2);
    halfway_rgb_uv(&mesh->tex_quads[0][poly_idx].r2, &mesh->tex_quads[0][poly_idx].r3, &quad_5827->r3); // XY 7
    copy_rgb_uv(&quad_5827->r3, &quad_8673->r2);
    copy_rgb_uv(&quad_5827->r3, &tri_327->r2);
    halfway_rgb_uv(&quad_0458->r2, &quad_4186->r3, &quad_0458->r3); // XY 8
    copy_rgb_uv(&quad_0458->r3, &quad_4186->r2);
    copy_rgb_uv(&quad_0458->r3, &quad_5827->r1);
    copy_rgb_uv(&quad_0458->r3, &quad_8673->r0);

    // Store positions 78
    gte_stsxy0(&quad_5827->x3); // XY 7
    gte_stsxy0(&quad_8673->x2);
    gte_stsxy0(&tri_327->x2);
    gte_stsxy1(&quad_0458->x3); // XY 8
    gte_stsxy1(&quad_4186->x2);
    gte_stsxy1(&quad_5827->x1);
    gte_stsxy1(&quad_8673->x0);

    // Copy constants
    const uint16_t clut = mesh->tex_quads[0][poly_idx].clut & 0b111110000111111; // reset distance fade palette index
    const uint16_t tpage = mesh->tex_quads[0][poly_idx].tpage;
    quad_0458->clut = clut;   quad_0458->tpage = tpage;
    quad_4186->clut = clut;   quad_4186->tpage = tpage;
    quad_5827->clut = clut;   quad_5827->tpage = tpage;
    quad_8673->clut = clut;   quad_8673->tpage = tpage;
    tri_014->clut = clut;     tri_014->tpage = tpage;
    tri_136->clut = clut;     tri_136->tpage = tpage;
    tri_327->clut = clut;     tri_327->tpage = tpage;
    tri_205->clut = clut;     tri_205->tpage = tpage;

    // Add to ordering table for rendering
    setPolyGT4(quad_0458);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_0458);
    setPolyGT4(quad_4186);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_4186);
    setPolyGT4(quad_5827);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_5827);
    setPolyGT4(quad_8673);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_8673);
    setPolyGT3(tri_014);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_014);
    setPolyGT3(tri_136);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_136);
    setPolyGT3(tri_327);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_327);
    setPolyGT3(tri_205);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_205);
}

void draw_level2_subdivided_quad(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx, scalar_t otz) {
    // 0-----------1       0--8--4--9--1    
    // |           |       |  |  |  |  |    
    // |           |       F--K--G--L--A    
    // |           |       |  |  |  |  |  
    // |           |  -->  7--J--O--H--5  + filler at the edges to fix gaps           
    // |           |       |  |  |  |  |  
    // |           |       E--N--I--M--B    
    // |           |       |  |  |  |  |  
    // 2-----------3       2--D--6--C--3

    #define v0 mesh->vtx_pos_and_size[vert_idx + 0]
    #define v1 mesh->vtx_pos_and_size[vert_idx + 1]
    #define v2 mesh->vtx_pos_and_size[vert_idx + 2]
    #define v3 mesh->vtx_pos_and_size[vert_idx + 3]

    // Subdivided quad
    POLY_GT4* quad_08FK = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_84KG = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_49GL = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_91LA = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_FK7J = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_KGJO = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_GLOH = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_LAH5 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_7JEN = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_JONI = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_OHIM = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_H5MB = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_EN2D = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_NID6 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_IM6C = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    POLY_GT4* quad_MBC3 = (POLY_GT4*)next_primitive;    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);

    // Filler triangles
    POLY_GT3* tri_048 = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_419 = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_15A = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_53B = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_36C = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_62D = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_27E = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    POLY_GT3* tri_70F = (POLY_GT3*)next_primitive;    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);

    // Copy the first position from what we did before calling this function
    if (store_to_precomp_prims) {
        COPY_POS(&mesh->tex_quads[drawbuffer][poly_idx].x0, &quad_08FK->x0);
        COPY_POS(&mesh->tex_quads[drawbuffer][poly_idx].x0, &tri_048->x0);
        COPY_POS(&mesh->tex_quads[drawbuffer][poly_idx].x0, &tri_70F->x1);
    }
    else {
        COPY_POS(&sxy_storage, &quad_08FK->x0);
        COPY_POS(&sxy_storage, &tri_048->x0);
        COPY_POS(&sxy_storage, &tri_70F->x1);
    }
    COPY_COLOR(&mesh->tex_quads[0][poly_idx].r0, &quad_08FK->r0);
    COPY_COLOR(&mesh->tex_quads[0][poly_idx].r0, &tri_048->r0);
    COPY_COLOR(&mesh->tex_quads[0][poly_idx].r0, &tri_70F->r1);
    COPY_UV(&mesh->tex_quads[0][poly_idx].u0, &quad_08FK->u0);
    COPY_UV(&mesh->tex_quads[0][poly_idx].u0, &tri_048->u0);
    COPY_UV(&mesh->tex_quads[0][poly_idx].u0, &tri_70F->u1);

    #define vtx_r0 mesh->tex_quads[0][poly_idx].r0
    #define vtx_r1 mesh->tex_quads[0][poly_idx].r1
    #define vtx_r2 mesh->tex_quads[0][poly_idx].r2
    #define vtx_r3 mesh->tex_quads[0][poly_idx].r3
    #define vtx_r4 quad_84KG->r1
    #define vtx_r5 quad_LAH5->r3
    #define vtx_r6 quad_NID6->r3
    #define vtx_r7 quad_FK7J->r2
    #define vtx_r8 quad_08FK->r1
    #define vtx_r9 quad_49GL->r1
    #define vtx_rA quad_91LA->r3
    #define vtx_rB quad_H5MB->r3
    #define vtx_rC quad_IM6C->r3
    #define vtx_rD quad_EN2D->r3
    #define vtx_rE quad_7JEN->r2
    #define vtx_rF quad_08FK->r2
    #define vtx_rG quad_84KG->r3
    #define vtx_rH quad_GLOH->r3
    #define vtx_rI quad_JONI->r3
    #define vtx_rJ quad_FK7J->r3
    #define vtx_rK quad_08FK->r3
    #define vtx_rL quad_49GL->r3
    #define vtx_rM quad_OHIM->r3
    #define vtx_rN quad_7JEN->r3
    #define vtx_rO quad_KGJO->r3

    // Store positions 123
    gte_stsxy0(&quad_91LA->x1); // XY 1
    gte_stsxy0(&tri_419->x1);
    gte_stsxy0(&tri_15A->x0);
    gte_stsxy1(&quad_EN2D->x2); // XY 2
    gte_stsxy1(&tri_62D->x1);
    gte_stsxy1(&tri_27E->x0);
    gte_stsxy2(&quad_MBC3->x3); // XY 3
    gte_stsxy2(&tri_53B->x1);
    gte_stsxy2(&tri_36C->x0);

    // Copy attributes for 123
    copy_rgb_uv(&vtx_r1, &quad_91LA->r1); // v1
    copy_rgb_uv(&vtx_r1, &tri_419->r1);
    copy_rgb_uv(&vtx_r1, &tri_15A->r0);
    copy_rgb_uv(&vtx_r2, &quad_EN2D->r2); // v2
    copy_rgb_uv(&vtx_r2, &tri_62D->r1);
    copy_rgb_uv(&vtx_r2, &tri_27E->r0);
    copy_rgb_uv(&vtx_r3, &quad_MBC3->r3); // v3
    copy_rgb_uv(&vtx_r3, &tri_53B->r1);
    copy_rgb_uv(&vtx_r3, &tri_36C->r0);

    // Interpolate vertex positions for 456
    const aligned_position_t v4 = get_halfway_position(v0, v1);
    const aligned_position_t v5 = get_halfway_position(v1, v3);
    const aligned_position_t v6 = get_halfway_position(v2, v3);

    // Transform 456
    gte_ldv3(&v4.x, &v5.x, &v6.x);
    gte_rtpt();

    // Copy attributes for 456
    halfway_rgb_uv(&vtx_r0, &vtx_r1, &vtx_r4);
    halfway_rgb_uv(&vtx_r1, &vtx_r3, &vtx_r5);
    halfway_rgb_uv(&vtx_r2, &vtx_r3, &vtx_r6);
    copy_rgb_uv(&vtx_r4, &quad_84KG->r1); // v4
    copy_rgb_uv(&vtx_r4, &quad_49GL->r0);
    copy_rgb_uv(&vtx_r4, &tri_048->r1);
    copy_rgb_uv(&vtx_r4, &tri_419->r0);
    copy_rgb_uv(&vtx_r5, &quad_LAH5->r3); // v5
    copy_rgb_uv(&vtx_r5, &quad_H5MB->r1);
    copy_rgb_uv(&vtx_r5, &tri_15A->r1);
    copy_rgb_uv(&vtx_r5, &tri_53B->r0);
    copy_rgb_uv(&vtx_r6, &quad_NID6->r3); // v6
    copy_rgb_uv(&vtx_r6, &quad_IM6C->r2);
    copy_rgb_uv(&vtx_r6, &tri_36C->r1);
    copy_rgb_uv(&vtx_r6, &tri_62D->r0);

    // Store positions 456
    gte_stsxy0(&quad_84KG->x1); // XY 4
    gte_stsxy0(&quad_49GL->x0);
    gte_stsxy0(&tri_048->x1);
    gte_stsxy0(&tri_419->x0);
    gte_stsxy1(&quad_LAH5->x3); // XY 5
    gte_stsxy1(&quad_H5MB->x1);
    gte_stsxy1(&tri_15A->x1);
    gte_stsxy1(&tri_53B->x0);
    gte_stsxy2(&quad_NID6->x3); // XY 6
    gte_stsxy2(&quad_IM6C->x2);
    gte_stsxy2(&tri_36C->x1);
    gte_stsxy2(&tri_62D->x0);

    // Interpolate vertex positions for 789
    const aligned_position_t v7 = get_halfway_position(v0, v2);
    const aligned_position_t v8 = get_halfway_position(v0, v4);
    const aligned_position_t v9 = get_halfway_position(v4, v1);

    // Transform 789
    gte_ldv3(&v7.x, &v8.x, &v9.x);
    gte_rtpt();

    // Copy attributes for 789
    halfway_rgb_uv(&vtx_r0, &vtx_r2, &vtx_r7);
    halfway_rgb_uv(&vtx_r0, &vtx_r4, &vtx_r8);
    halfway_rgb_uv(&vtx_r4, &vtx_r1, &vtx_r9);
    copy_rgb_uv(&vtx_r7, &quad_FK7J->r2); // v7
    copy_rgb_uv(&vtx_r7, &quad_7JEN->r0);
    copy_rgb_uv(&vtx_r7, &tri_27E->r1);
    copy_rgb_uv(&vtx_r7, &tri_70F->r0);
    copy_rgb_uv(&vtx_r8, &quad_08FK->r1); // v8
    copy_rgb_uv(&vtx_r8, &quad_84KG->r0);
    copy_rgb_uv(&vtx_r8, &tri_048->r2);
    copy_rgb_uv(&vtx_r9, &quad_49GL->r1); // v9
    copy_rgb_uv(&vtx_r9, &quad_91LA->r0);
    copy_rgb_uv(&vtx_r9, &tri_419->r2);

    // Store positions 789
    gte_stsxy0(&quad_FK7J->x2); // XY 7
    gte_stsxy0(&quad_7JEN->x0);
    gte_stsxy0(&tri_27E->x1);
    gte_stsxy0(&tri_70F->x0);
    gte_stsxy1(&quad_08FK->x1); // XY 8
    gte_stsxy1(&quad_84KG->x0);
    gte_stsxy1(&tri_048->x2);
    gte_stsxy2(&quad_49GL->x1); // XY 9
    gte_stsxy2(&quad_91LA->x0);
    gte_stsxy2(&tri_419->x2);

    // Interpolate vertex positions for ABC
    const aligned_position_t vA = get_halfway_position(v1, v5);
    const aligned_position_t vB = get_halfway_position(v5, v3);
    const aligned_position_t vC = get_halfway_position(v6, v3);

    // Transform ABC
    gte_ldv3(&vA.x, &vB.x, &vC.x);
    gte_rtpt();

    // Copy attributes for ABC
    halfway_rgb_uv(&vtx_r1, &vtx_r5, &vtx_rA);
    halfway_rgb_uv(&vtx_r5, &vtx_r3, &vtx_rB);
    halfway_rgb_uv(&vtx_r6, &vtx_r3, &vtx_rC);
    copy_rgb_uv(&vtx_rA, &quad_91LA->r3); // vA
    copy_rgb_uv(&vtx_rA, &quad_LAH5->r1);
    copy_rgb_uv(&vtx_rA, &tri_15A->r2);
    copy_rgb_uv(&vtx_rB, &quad_H5MB->r3); // vB
    copy_rgb_uv(&vtx_rB, &quad_MBC3->r1);
    copy_rgb_uv(&vtx_rB, &tri_53B->r2);
    copy_rgb_uv(&vtx_rC, &quad_IM6C->r3); // vC
    copy_rgb_uv(&vtx_rC, &quad_MBC3->r2);
    copy_rgb_uv(&vtx_rC, &tri_36C->r2);

    // Store positions ABC
    gte_stsxy0(&quad_91LA->x3); // XY A
    gte_stsxy0(&quad_LAH5->x1);
    gte_stsxy0(&tri_15A->x2);
    gte_stsxy1(&quad_H5MB->x3); // XY B
    gte_stsxy1(&quad_MBC3->x1);
    gte_stsxy1(&tri_53B->x2);
    gte_stsxy2(&quad_IM6C->x3); // XY C
    gte_stsxy2(&quad_MBC3->x2);
    gte_stsxy2(&tri_36C->x2);

    // Interpolate vertex positions for DEF
    const aligned_position_t vD = get_halfway_position(v2, v6);
    const aligned_position_t vE = get_halfway_position(v7, v2);
    const aligned_position_t vF = get_halfway_position(v0, v7);

    // Transform DEF
    gte_ldv3(&vD.x, &vE.x, &vF.x);
    gte_rtpt();

    // Copy attributes for DEF
    halfway_rgb_uv(&vtx_r2, &vtx_r6, &vtx_rD);
    halfway_rgb_uv(&vtx_r7, &vtx_r2, &vtx_rE);
    halfway_rgb_uv(&vtx_r0, &vtx_r7, &vtx_rF);
    copy_rgb_uv(&vtx_rD, &quad_EN2D->r3); // vD
    copy_rgb_uv(&vtx_rD, &quad_NID6->r2);
    copy_rgb_uv(&vtx_rD, &tri_62D->r2);
    copy_rgb_uv(&vtx_rE, &quad_7JEN->r2); // vE
    copy_rgb_uv(&vtx_rE, &quad_EN2D->r0);
    copy_rgb_uv(&vtx_rE, &tri_27E->r2);
    copy_rgb_uv(&vtx_rF, &quad_08FK->r2); // vF
    copy_rgb_uv(&vtx_rF, &quad_FK7J->r0);
    copy_rgb_uv(&vtx_rF, &tri_70F->r2);

    // Store positions DEF
    gte_stsxy0(&quad_EN2D->x3); // XY D
    gte_stsxy0(&quad_NID6->x2);
    gte_stsxy0(&tri_62D->x2);
    gte_stsxy1(&quad_7JEN->x2); // XY E
    gte_stsxy1(&quad_EN2D->x0);
    gte_stsxy1(&tri_27E->x2);
    gte_stsxy2(&quad_08FK->x2); // XY F
    gte_stsxy2(&quad_FK7J->x0);
    gte_stsxy2(&tri_70F->x2);

    // Interpolate vertex positions for GHI
    const aligned_position_t vG = get_halfway_position(vF, vA);
    const aligned_position_t vH = get_halfway_position(v9, vC);
    const aligned_position_t vI = get_halfway_position(vE, vB);

    // Transform GHI
    gte_ldv3(&vG.x, &vH.x, &vI.x);
    gte_rtpt();

    // Copy attributes for GHI
    halfway_rgb_uv(&vtx_rF, &vtx_rA, &vtx_rG);
    halfway_rgb_uv(&vtx_r9, &vtx_rC, &vtx_rH);
    halfway_rgb_uv(&vtx_rE, &vtx_rB, &vtx_rI);
    copy_rgb_uv(&vtx_rG, &quad_84KG->r3); // vG
    copy_rgb_uv(&vtx_rG, &quad_49GL->r2);
    copy_rgb_uv(&vtx_rG, &quad_KGJO->r1);
    copy_rgb_uv(&vtx_rG, &quad_GLOH->r0);
    copy_rgb_uv(&vtx_rH, &quad_GLOH->r3); // vH
    copy_rgb_uv(&vtx_rH, &quad_LAH5->r2);
    copy_rgb_uv(&vtx_rH, &quad_OHIM->r1);
    copy_rgb_uv(&vtx_rH, &quad_H5MB->r0);
    copy_rgb_uv(&vtx_rI, &quad_JONI->r3); // vI
    copy_rgb_uv(&vtx_rI, &quad_OHIM->r2);
    copy_rgb_uv(&vtx_rI, &quad_NID6->r1);
    copy_rgb_uv(&vtx_rI, &quad_IM6C->r0);

    // Store positions GHI
    gte_stsxy0(&quad_84KG->x3); // XY G
    gte_stsxy0(&quad_49GL->x2);
    gte_stsxy0(&quad_KGJO->x1);
    gte_stsxy0(&quad_GLOH->x0);
    gte_stsxy1(&quad_GLOH->x3); // XY H
    gte_stsxy1(&quad_LAH5->x2);
    gte_stsxy1(&quad_OHIM->x1);
    gte_stsxy1(&quad_H5MB->x0);
    gte_stsxy2(&quad_JONI->x3); // XY I
    gte_stsxy2(&quad_OHIM->x2);
    gte_stsxy2(&quad_NID6->x1);
    gte_stsxy2(&quad_IM6C->x0);

    // Interpolate vertex positions for JKL
    const aligned_position_t vJ = get_halfway_position(v8, vD);
    const aligned_position_t vK = get_halfway_position(v8, vJ);
    const aligned_position_t vL = get_halfway_position(v9, vH);

    // Transform JKL
    gte_ldv3(&vJ.x, &vK.x, &vL.x);
    gte_rtpt();

    // Copy attributes for JKL
    halfway_rgb_uv(&vtx_r8, &vtx_rD, &vtx_rJ);
    halfway_rgb_uv(&vtx_r8, &vtx_rJ, &vtx_rK);
    halfway_rgb_uv(&vtx_r9, &vtx_rH, &vtx_rL);
    copy_rgb_uv(&vtx_rJ, &quad_FK7J->r3); // vJ
    copy_rgb_uv(&vtx_rJ, &quad_KGJO->r2);
    copy_rgb_uv(&vtx_rJ, &quad_7JEN->r1);
    copy_rgb_uv(&vtx_rJ, &quad_JONI->r0);
    copy_rgb_uv(&vtx_rK, &quad_08FK->r3); // vK
    copy_rgb_uv(&vtx_rK, &quad_84KG->r2);
    copy_rgb_uv(&vtx_rK, &quad_FK7J->r1);
    copy_rgb_uv(&vtx_rK, &quad_KGJO->r0);
    copy_rgb_uv(&vtx_rL, &quad_49GL->r3); // vL
    copy_rgb_uv(&vtx_rL, &quad_91LA->r2);
    copy_rgb_uv(&vtx_rL, &quad_GLOH->r1);
    copy_rgb_uv(&vtx_rL, &quad_LAH5->r0);

    // Store positions JKL
    gte_stsxy0(&quad_FK7J->x3); // XY J
    gte_stsxy0(&quad_KGJO->x2);
    gte_stsxy0(&quad_7JEN->x1);
    gte_stsxy0(&quad_JONI->x0);
    gte_stsxy1(&quad_08FK->x3); // XY K
    gte_stsxy1(&quad_84KG->x2);
    gte_stsxy1(&quad_FK7J->x1);
    gte_stsxy1(&quad_KGJO->x0);
    gte_stsxy2(&quad_49GL->x3); // XY L
    gte_stsxy2(&quad_91LA->x2);
    gte_stsxy2(&quad_GLOH->x1);
    gte_stsxy2(&quad_LAH5->x0);

    // Interpolate vertex positions for MNO
    const aligned_position_t vM = get_halfway_position(vC, vH);
    const aligned_position_t vN = get_halfway_position(vE, vI);
    const aligned_position_t vO = get_halfway_position(v4, v6);

    // Transform MNO
    gte_ldv3(&vM.x, &vN.x, &vO.x);
    gte_rtpt();

    // Copy attributes for MNO
    halfway_rgb_uv(&vtx_rC, &vtx_rH, &vtx_rM);
    halfway_rgb_uv(&vtx_rE, &vtx_rI, &vtx_rN);
    halfway_rgb_uv(&vtx_r4, &vtx_r6, &vtx_rO);
    copy_rgb_uv(&vtx_rM, &quad_OHIM->r3); // vM
    copy_rgb_uv(&vtx_rM, &quad_H5MB->r2);
    copy_rgb_uv(&vtx_rM, &quad_IM6C->r1);
    copy_rgb_uv(&vtx_rM, &quad_MBC3->r0);
    copy_rgb_uv(&vtx_rN, &quad_7JEN->r3); // vN
    copy_rgb_uv(&vtx_rN, &quad_JONI->r2);
    copy_rgb_uv(&vtx_rN, &quad_EN2D->r1);
    copy_rgb_uv(&vtx_rN, &quad_NID6->r0);
    copy_rgb_uv(&vtx_rO, &quad_KGJO->r3); // vO
    copy_rgb_uv(&vtx_rO, &quad_GLOH->r2);
    copy_rgb_uv(&vtx_rO, &quad_JONI->r1);
    copy_rgb_uv(&vtx_rO, &quad_OHIM->r0);

    // Store positions MNO
    gte_stsxy0(&quad_OHIM->x3); // XY M
    gte_stsxy0(&quad_H5MB->x2);
    gte_stsxy0(&quad_IM6C->x1);
    gte_stsxy0(&quad_MBC3->x0);
    gte_stsxy1(&quad_7JEN->x3); // XY N
    gte_stsxy1(&quad_JONI->x2);
    gte_stsxy1(&quad_EN2D->x1);
    gte_stsxy1(&quad_NID6->x0);
    gte_stsxy2(&quad_KGJO->x3); // XY O
    gte_stsxy2(&quad_GLOH->x2);
    gte_stsxy2(&quad_JONI->x1);
    gte_stsxy2(&quad_OHIM->x0);

    // Copy constant fields
    const uint16_t clut = mesh->tex_quads[0][poly_idx].clut & 0b111110000111111; // reset distance fade palette index
    const uint16_t tpage = mesh->tex_quads[0][poly_idx].tpage;
    quad_08FK->clut = clut;    quad_08FK->tpage = tpage;
    quad_84KG->clut = clut;    quad_84KG->tpage = tpage;
    quad_49GL->clut = clut;    quad_49GL->tpage = tpage;
    quad_91LA->clut = clut;    quad_91LA->tpage = tpage;
    quad_FK7J->clut = clut;    quad_FK7J->tpage = tpage;
    quad_KGJO->clut = clut;    quad_KGJO->tpage = tpage;
    quad_GLOH->clut = clut;    quad_GLOH->tpage = tpage;
    quad_LAH5->clut = clut;    quad_LAH5->tpage = tpage;
    quad_7JEN->clut = clut;    quad_7JEN->tpage = tpage;
    quad_JONI->clut = clut;    quad_JONI->tpage = tpage;
    quad_OHIM->clut = clut;    quad_OHIM->tpage = tpage;
    quad_H5MB->clut = clut;    quad_H5MB->tpage = tpage;
    quad_EN2D->clut = clut;    quad_EN2D->tpage = tpage;
    quad_NID6->clut = clut;    quad_NID6->tpage = tpage;
    quad_IM6C->clut = clut;    quad_IM6C->tpage = tpage;
    quad_MBC3->clut = clut;    quad_MBC3->tpage = tpage;
    tri_048->clut = clut;    tri_048->tpage = tpage;
    tri_419->clut = clut;    tri_419->tpage = tpage;
    tri_15A->clut = clut;    tri_15A->tpage = tpage;
    tri_53B->clut = clut;    tri_53B->tpage = tpage;
    tri_36C->clut = clut;    tri_36C->tpage = tpage;
    tri_62D->clut = clut;    tri_62D->tpage = tpage;
    tri_27E->clut = clut;    tri_27E->tpage = tpage;
    tri_70F->clut = clut;    tri_70F->tpage = tpage;

    #undef vtx_r0
    #undef vtx_r1
    #undef vtx_r2
    #undef vtx_r3
    #undef vtx_r4
    #undef vtx_r5
    #undef vtx_r6
    #undef vtx_r7
    #undef vtx_r8
    #undef vtx_r9
    #undef vtx_r10
    #undef vtx_r11
    #undef vtx_r12
    #undef vtx_r13
    #undef vtx_r14
    #undef vtx_r15
    #undef vtx_r16
    #undef vtx_r17
    #undef vtx_r18
    #undef vtx_r19
    #undef vtx_r20
    #undef vtx_r21
    #undef vtx_r22
    #undef vtx_r23
    #undef vtx_r24
    #undef v0
    #undef v1
    #undef v2
    #undef v3

    // Add to ordering table for rendering
    setPolyGT4(quad_08FK);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_08FK);
    setPolyGT4(quad_84KG);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_84KG);
    setPolyGT4(quad_49GL);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_49GL);
    setPolyGT4(quad_91LA);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_91LA);
    setPolyGT4(quad_FK7J);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_FK7J);
    setPolyGT4(quad_KGJO);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_KGJO);
    setPolyGT4(quad_GLOH);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_GLOH);
    setPolyGT4(quad_LAH5);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_LAH5);
    setPolyGT4(quad_7JEN);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_7JEN);
    setPolyGT4(quad_JONI);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_JONI);
    setPolyGT4(quad_OHIM);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_OHIM);
    setPolyGT4(quad_H5MB);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_H5MB);
    setPolyGT4(quad_EN2D);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_EN2D);
    setPolyGT4(quad_NID6);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_NID6);
    setPolyGT4(quad_IM6C);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_IM6C);
    setPolyGT4(quad_MBC3);  addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, quad_MBC3);
    setPolyGT3(tri_048);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_048);
    setPolyGT3(tri_419);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_419);
    setPolyGT3(tri_15A);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_15A);
    setPolyGT3(tri_53B);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_53B);
    setPolyGT3(tri_36C);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_36C);
    setPolyGT3(tri_62D);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_62D);
    setPolyGT3(tri_27E);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_27E);
    setPolyGT3(tri_70F);    addPrim(ord_tbl[drawbuffer] + otz + curr_ot_bias, tri_70F);
}

// Transform and add to queue a textured, shaded triangle, with automatic subdivision based on size, fading to solid color in the distance.
static inline void draw_tex_triangle3d_fancy(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx) {
    // If this is an occluder, don't render it
    if (mesh->vtx_pos_and_size[vert_idx + 0].tex_id == 254) return;
    
    // Transform the 3 vertices
    gte_ldv3(
        &mesh->vtx_pos_and_size[vert_idx + 0].x,
        &mesh->vtx_pos_and_size[vert_idx + 1].x,
        &mesh->vtx_pos_and_size[vert_idx + 2].x
    );
    gte_rtpt();

    // Backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);
    if (p <= 0) return;

    // Get depth
    gte_avsz3();
    int avg_z;
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || (avg_z <= 0)) return;
    
    // If very close, subdivide twice
    const int32_t sub2_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB2_30 : TRI_THRESHOLD_MUL_SUB2_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub2_threshold) {
        draw_level2_subdivided_triangle(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // // If close, subdivice once
    const int32_t sub1_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB1_30 : TRI_THRESHOLD_MUL_SUB1_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub1_threshold) {
        draw_level1_subdivided_triangle(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // If quad is far away, render textured, fading into single color when further away
    // Calculate CLUT fade
    int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
    if (clut_fade > (N_CLUT_FADES-1)) clut_fade = (N_CLUT_FADES-1);
    else if (clut_fade < 0) clut_fade = 0;

    // Store data into primitive struct
    gte_stsxy3_gt3(&mesh->tex_tris[drawbuffer][poly_idx]);
    SET_DISTANCE_FADE(&mesh->tex_tris[drawbuffer][poly_idx], clut_fade);
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, &mesh->tex_tris[drawbuffer][poly_idx]);
    return;
}

// Transform and add to queue a textured, shaded quad, with automatic subdivision based on size, fading to solid color in the distance.
static inline void draw_tex_quad3d_fancy(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx) {
    // If this is an occluder, don't render it
    if (mesh->vtx_pos_and_size[vert_idx + 0].tex_id == 254) return;

    // Transform the first 3 vertices
    gte_ldv3(
        &mesh->vtx_pos_and_size[vert_idx + 0].x,
        &mesh->vtx_pos_and_size[vert_idx + 1].x,
        &mesh->vtx_pos_and_size[vert_idx + 2].x
    );
    gte_rtpt();

    // Backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);
    if (p <= 0) return;

    // Store the first vertex, as it will be pushed out by the RTPS call down a couple lines
    gte_stsxy0(&mesh->tex_quads[drawbuffer][poly_idx].x0);

    // Transform the last vertex
    gte_ldv0(&mesh->vtx_pos_and_size[vert_idx + 3].x);
    gte_rtps();

    // Get depth
    gte_avsz4();
    int avg_z;
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || (avg_z <= 0)) return;

    // Store the other 3 vertices
    gte_stsxy0(&mesh->tex_quads[drawbuffer][poly_idx].x1);
    gte_stsxy1(&mesh->tex_quads[drawbuffer][poly_idx].x2);
    gte_stsxy2(&mesh->tex_quads[drawbuffer][poly_idx].x3);

    // Cull if off screen - subdivision is expensive
    int n_left = 1;
    int n_right = 1;
    int n_up = 1;
    int n_down = 1;

    #define poly mesh->tex_quads[drawbuffer][poly_idx]
    if (poly.x0 < 0)            n_left <<= 1;   
    if (poly.y0 < 0)            n_up <<= 1;
    if (poly.x0 > res_x)        n_right <<= 1;  
    if (poly.y0 > curr_res_y)   n_down <<= 1;
    if (poly.x1 < 0)            n_left <<= 1;   
    if (poly.y1 < 0)            n_up <<= 1;
    if (poly.x1 > res_x)        n_right <<= 1;  
    if (poly.y1 > curr_res_y)   n_down <<= 1;
    if (poly.x2 < 0)            n_left <<= 1;   
    if (poly.y2 < 0)            n_up <<= 1;
    if (poly.x2 > res_x)        n_right <<= 1;  
    if (poly.y2 > curr_res_y)   n_down <<= 1;
    if (poly.x3 < 0)            n_left <<= 1;   
    if (poly.y3 < 0)            n_up <<= 1;
    if (poly.x3 > res_x)        n_right <<= 1;  
    if (poly.y3 > curr_res_y)   n_down <<= 1;
    #undef poly

    // If all vertices are off to one side of the screen, one of the values will be shifted left 4x.
    // When OR-ing the bits together, the result will be %0001????.
    // Shifting that value to the right 4x drops off the irrelevant ???? bits, returning 1 if all vertices are to one side, and 0 if not
    const int off_to_one_side = (n_left | n_right | n_up | n_down) >> 4;
    if (off_to_one_side) return;

    // If very close, subdivide twice
    const int32_t sub2_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB2_30 : TRI_THRESHOLD_MUL_SUB2_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub2_threshold) {
        draw_level2_subdivided_quad(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // // If close, subdivice once
    const int32_t sub1_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB1_30 : TRI_THRESHOLD_MUL_SUB1_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub1_threshold) {
        draw_level1_subdivided_quad(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // If quad is far away, render textured, fading into single color when further away
    // Calculate CLUT fade
    int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
    if (clut_fade > (N_CLUT_FADES-1)) clut_fade = (N_CLUT_FADES-1);
    else if (clut_fade < 0) clut_fade = 0;

    // Store final bit of data into primitive struct
    SET_DISTANCE_FADE(&mesh->tex_quads[drawbuffer][poly_idx], clut_fade);
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, &mesh->tex_quads[drawbuffer][poly_idx]);
    return;
}

// Transform and add to queue a textured, shaded triangle, with automatic subdivision based on size, fading to solid color in the distance.
static inline void draw_tex_triangle3d_fancy_no_precomp(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx) {
    // If this is an occluder, don't render it
    if (mesh->vtx_pos_and_size[vert_idx + 0].tex_id == 254) return;
    
    // Transform the 3 vertices
    gte_ldv3(
        &mesh->vtx_pos_and_size[vert_idx + 0].x,
        &mesh->vtx_pos_and_size[vert_idx + 1].x,
        &mesh->vtx_pos_and_size[vert_idx + 2].x
    );
    gte_rtpt();

    // Backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);
    if (p <= 0) return;

    // Get depth
    gte_avsz3();
    int avg_z;
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || (avg_z <= 0)) return;
    
    // If very close, subdivide twice
    const int32_t sub2_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB2_30 : TRI_THRESHOLD_MUL_SUB2_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub2_threshold) {
        draw_level2_subdivided_triangle(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // // If close, subdivice once
    const int32_t sub1_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB1_30 : TRI_THRESHOLD_MUL_SUB1_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub1_threshold) {
        draw_level1_subdivided_triangle(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // If quad is far away, render textured, fading into single color when further away
    // Calculate CLUT fade
    int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
    if (clut_fade > (N_CLUT_FADES-1)) clut_fade = (N_CLUT_FADES-1);
    else if (clut_fade < 0) clut_fade = 0;

    // Store data into primitive struct
    POLY_GT3* tri = (POLY_GT3*)next_primitive;      
    next_primitive += sizeof(POLY_GT3) / sizeof(*next_primitive);
    setPolyGT3(tri);
    gte_stsxy3_gt3(tri);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r0, &tri->r0);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r1, &tri->r1);
    copy_rgb_uv(&mesh->tex_tris[0][poly_idx].r2, &tri->r2);
    tri->clut = mesh->tex_quads[0][poly_idx].clut;
    tri->tpage = mesh->tex_quads[0][poly_idx].tpage;
    SET_DISTANCE_FADE(tri, clut_fade);
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, tri);
    return;
}

// Transform and add to queue a textured, shaded quad, with automatic subdivision based on size, fading to solid color in the distance.
static inline void draw_tex_quad3d_fancy_no_precomp(const mesh_t* mesh, const size_t vert_idx, const size_t poly_idx) {
    // If this is an occluder, don't render it
    if (mesh->vtx_pos_and_size[vert_idx + 0].tex_id == 254) return;

    // Transform the first 3 vertices
    gte_ldv3(
        &mesh->vtx_pos_and_size[vert_idx + 0].x,
        &mesh->vtx_pos_and_size[vert_idx + 1].x,
        &mesh->vtx_pos_and_size[vert_idx + 2].x
    );
    gte_rtpt();

    // Backface culling
    int p;
    gte_nclip();
    gte_stopz(&p);
    if (p <= 0) return;

    // Store the first vertex, as it will be pushed out by the RTPS call down a couple lines
    POLY_GT4* poly = (POLY_GT4*)next_primitive;      
    next_primitive += sizeof(POLY_GT4) / sizeof(*next_primitive);
    gte_stsxy0(&sxy_storage);
    gte_stsxy0(&poly->x0);

    // Transform the last vertex
    gte_ldv0(&mesh->vtx_pos_and_size[vert_idx + 3].x);
    gte_rtps();

    // Get depth
    gte_avsz4();
    int avg_z;
    gte_stotz(&avg_z);

    // Depth culling
    if ((avg_z + curr_ot_bias) >= ORD_TBL_LENGTH || (avg_z <= 0)) return;

    // Store the other 3 vertices
    gte_stsxy0(&poly->x1);
    gte_stsxy1(&poly->x2);
    gte_stsxy2(&poly->x3);

    // Cull if off screen - subdivision is expensive
    int n_left = 1;
    int n_right = 1;
    int n_up = 1;
    int n_down = 1;

    if (poly->x0 < 0)            n_left <<= 1;   
    if (poly->y0 < 0)            n_up <<= 1;
    if (poly->x0 > res_x)        n_right <<= 1;  
    if (poly->y0 > curr_res_y)   n_down <<= 1;
    if (poly->x1 < 0)            n_left <<= 1;   
    if (poly->y1 < 0)            n_up <<= 1;
    if (poly->x1 > res_x)        n_right <<= 1;  
    if (poly->y1 > curr_res_y)   n_down <<= 1;
    if (poly->x2 < 0)            n_left <<= 1;   
    if (poly->y2 < 0)            n_up <<= 1;
    if (poly->x2 > res_x)        n_right <<= 1;  
    if (poly->y2 > curr_res_y)   n_down <<= 1;
    if (poly->x3 < 0)            n_left <<= 1;   
    if (poly->y3 < 0)            n_up <<= 1;
    if (poly->x3 > res_x)        n_right <<= 1;  
    if (poly->y3 > curr_res_y)   n_down <<= 1;

    // If all vertices are off to one side of the screen, one of the values will be shifted left 4x.
    // When OR-ing the bits together, the result will be %0001????.
    // Shifting that value to the right 4x drops off the irrelevant ???? bits, returning 1 if all vertices are to one side, and 0 if not
    const int off_to_one_side = (n_left | n_right | n_up | n_down) >> 4;
    if (off_to_one_side) return;

    // If very close, subdivide twice
    const int32_t sub2_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB2_30 : TRI_THRESHOLD_MUL_SUB2_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub2_threshold) {
        draw_level2_subdivided_quad(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // // If close, subdivice once
    const int32_t sub1_threshold = ((vsync_enable == 2) ? TRI_THRESHOLD_MUL_SUB1_30 : TRI_THRESHOLD_MUL_SUB1_60) * (int32_t)mesh->vtx_pos_and_size[vert_idx + 0].poly_size;
    if (avg_z < sub1_threshold) {
        draw_level1_subdivided_quad(mesh, vert_idx, poly_idx, avg_z);
        return;
    }

    // If quad is far away, render textured, fading into single color when further away
    // Calculate CLUT fade
    int16_t clut_fade = ((N_CLUT_FADES-1) * (avg_z - TRI_THRESHOLD_FADE_START)) / (TRI_THRESHOLD_FADE_END - TRI_THRESHOLD_FADE_START);
    if (clut_fade > (N_CLUT_FADES-1)) clut_fade = (N_CLUT_FADES-1);
    else if (clut_fade < 0) clut_fade = 0;

    // Store final bit of data into primitive struct
    setPolyGT4(poly);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r0, &poly->r0);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r1, &poly->r1);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r2, &poly->r2);
    copy_rgb_uv(&mesh->tex_quads[0][poly_idx].r3, &poly->r3);
    poly->clut = mesh->tex_quads[0][poly_idx].clut;
    poly->tpage = mesh->tex_quads[0][poly_idx].tpage;
    SET_DISTANCE_FADE(poly, clut_fade);
    addPrim(ord_tbl[drawbuffer] + avg_z + curr_ot_bias, poly);
    return;
}
