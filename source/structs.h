
#ifndef STRUCTS_H
#define STRUCTS_H
#include "vec3.h"

typedef struct {
    vec3_t min;
    vec3_t max;
} aabb_t;

typedef struct {
    aabb_t bounds; // Axis aligned bounding box around all primitives inside this node
    uint16_t left_first; // If this is a leaf, this is the index of the first primitive, otherwise, this is the index of the first of two child nodes
    uint16_t primitive_count; // Number of primitives in this leaf. If the primitive count is 0, this node isn't a leaf, in which case ignore this value
} bvh_node_t;

typedef struct {
    vec3_t position;
    vec3_t direction;
    vec3_t inv_direction;
    scalar_t length;
} ray_t;

typedef struct {
    vec3_t center;
    scalar_t radius;
    scalar_t radius_squared;
} sphere_t;

typedef struct {
    vec3_t bottom;
    scalar_t radius;
    scalar_t radius_squared;
    scalar_t height;
    int is_wall_check;
} vertical_cylinder_t;

typedef struct {
    int16_t x, y, z; // 3D position
    uint8_t r, g, b; // 8-bit RGB color
    uint8_t u, v, tex_id; // Texture Coordinates
} vertex_3d_t;

typedef struct {
    int8_t x, y, z, padding;
} normal_t;

typedef struct {
    vertex_3d_t v0, v1;
} line_3d_t;

typedef struct {
    vertex_3d_t v0, v1, v2;
} triangle_3d_t;

typedef struct {
    vec3_t v0, v1, v2;
    vec3_t normal;
} collision_triangle_3d_t;

#ifdef _PSX 
#include <psxgpu.h>

typedef struct {
    union {
        struct {
            int16_t x, y, z;
            uint8_t poly_size;
            uint8_t tex_id;
        };
        struct {
            uint32_t vxy;
            uint32_t vz;
        };
    };
} aligned_position_t;

typedef struct {
    uint16_t n_triangles;
    uint16_t n_quads;
    POLY_GT3* tex_tris[2]; // double buffered
    POLY_GT4* tex_quads[2]; // double buffered
    aligned_position_t* vtx_pos_and_size;
    aabb_t bounds;
    char* name;
    int optimized_for_single_render_per_frame;
} mesh_t;
#else
typedef struct {
    uint16_t n_triangles;
    uint16_t n_quads;
    unsigned int vbo_vertices;
    unsigned int vbo_normals;
    unsigned int vao;
    vertex_3d_t* vertices;
    normal_t* normals;
    aabb_t bounds;
    char* name;
} mesh_t;
#endif

#define MAGIC_FMSH 0x48534D46
typedef struct {
    uint32_t file_magic;            // File identifier magic, always "FMSH"
    uint32_t n_submeshes;           // Number of submeshes in this model.
    uint32_t offset_mesh_desc;      // Offset into the binary section to the start of the array of MeshDesc structs.
    uint32_t offset_vertex_data;    // Offset into the binary section to the start of the raw VertexPSX data.
    uint32_t offset_mesh_names;     // Offset into the binary section to the start of the mesh names
    uint32_t offset_vertex_normals; // Offset into the binary section to the start of the vertex normals
    uint32_t offset_lightmap_uv;    // Offset into the binary section to the start of the lightmap UV data. Vertices correspond directly to the VertexPSX array. If this offset is 0xFFFFFFFF, there is no lightmap
    uint32_t offset_lightmap_tex;   // Offset into the binary section to the start of the lightmap texture data
} model_header_t;

typedef struct {
    uint16_t vertex_start; // First vertex index for this model.
    uint16_t n_triangles;  // Number of vertices for this model.
    uint16_t n_quads;      // Number of vertices for this model.
    int16_t x_min;         // Axis aligned bounding box minimum X.
    int16_t x_max;         // Axis aligned bounding box maximum X.
    int16_t y_min;         // Axis aligned bounding box minimum Y.
    int16_t y_max;         // Axis aligned bounding box maximum Y.
    int16_t z_min;         // Axis aligned bounding box minimum Z.
    int16_t z_max;         // Axis aligned bounding box maximum Z.
    int16_t _pad;          // (align to 4 bytes)
} mesh_desc_t;

typedef struct {
    uint32_t n_meshes;
    mesh_t* meshes;
} model_t;

typedef struct {
    svec3_t position;
    uint16_t neighbor_ids[4];
} nav_node_t;

typedef struct {
    collision_triangle_3d_t* primitives;
    uint16_t* indices;
    union {
        bvh_node_t* nodes;
        bvh_node_t* root;
    };
    nav_node_t* nav_graph_nodes;
    uint16_t n_primitives;
    uint16_t n_nav_graph_nodes;
} level_collision_t;

typedef enum {
    none,
} col_mat_t;

typedef enum {
    RAY_HIT_TYPE_TRIANGLE,
    RAY_HIT_TYPE_ENTITY_HITBOX,
} ray_hit_type_t;

typedef struct {
    vec3_t position;
    vec3_t normal;
    scalar_t distance;
    scalar_t distance_along_normal;
    col_mat_t collision_material;
    ray_hit_type_t type;
    union {
        struct {
            collision_triangle_3d_t* triangle;
        } tri;
        struct {
            uint8_t entity_index;
            uint8_t box_index;
            uint8_t not_move_player_along : 1;
        } entity_hitbox;
    };
} rayhit_t;

#define MAGIC_FCOL 0x4C4F4346
typedef struct {
    uint32_t file_magic;           // File magic: "FCOL"
    uint32_t n_verts;              // The number of vertices in this collision mesh 
    uint32_t n_nodes;              // The number of nodes in the collision mesh's BVH 
    uint32_t triangle_data_offset; // Offset to raw triangle data 
    uint32_t terrain_id_offset;    // Offset to array of 8 bit terrain IDs for each triangle
    uint32_t bvh_nodes_offset;     // Offset to the precalculated BVH's node pool 
    uint32_t bvh_indices_offset;   // Offset to the precalculated BVH's index array 
    uint32_t nav_graph_offset;     // Offset to the precalculated navigation graph for the enemies 
} collision_mesh_header_t;

typedef struct {
  int16_t x, y, z;
  uint16_t terrain_id;  
} col_mesh_file_vert_t;

typedef struct {
  col_mesh_file_vert_t v0, v1, v2;
} col_mesh_file_tri_t;

typedef struct {
    uint32_t n_verts;
    col_mesh_file_vert_t* verts;
} collision_mesh_t;

typedef struct {
    vec3_t position; // Position (4096 is 1.0 meter)
    vec3_t rotation; // Angles in fixed-point format (131072 units = 360 degrees)
    vec3_t scale;    // Scale (4096 = 1.0)
} transform_t;

#endif // STRUCTS_H
