
#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct {
    vec3_t min;
    vec3_t max;
} aabb_t;

typedef struct {
    aabb_t bounds;                        // Axis aligned bounding box around all primitives inside this node
    uint16_t left_first;                // If this is a leaf, this is the index of the first primitive, otherwise, this is the index of the first of two child nodes
    uint16_t is_leaf : 1;               // The high bit of the count determines whether this is a node or a leaf
    uint16_t primitive_count : 15;      // Number of primitives in this leaf. If this node isn't a leaf, ignore this value
} bvh_node_t;

typedef struct {
    vec3_t position;
    vec3_t direction;
} ray_t;

typedef enum {
    axis_x,
    axis_y,
    axis_z,
} axis_t;

typedef struct {
    int16_t x, y; // 2D position
    uint8_t r, g, b, pad; // 8-bit RGB color (and padding for alignment)
} vertex_2d_t;

typedef struct {
    int16_t x, y, z; // 3D position
    uint8_t r, g, b; // 8-bit RGB color
    uint8_t u, v, tex_id; // Texture Coordinates (and padding for alignment)
} vertex_3d_t;

typedef struct {
    vertex_3d_t v0, v1;
} line_3d_t;

typedef struct {
    vertex_3d_t v0, v1, v2;
} triangle_3d_t;

typedef struct {
    uint32_t n_vertices;
    vertex_3d_t* vertices;
    aabb_t bounds;
} mesh_t;

typedef struct {
    uint32_t file_magic;         // File identifier magic, always "FMSH"
    uint32_t n_submeshes;       // Number of submeshes in this model.
    uint32_t offset_mesh_desc;  // Offset into the binary section to the start of the array of MeshDesc structs.
    uint32_t offset_vertex_data;// Offset into the binary section to the start of the raw VertexPSX data.
} model_header_t;

typedef struct {
    uint16_t vertex_start;  // First vertex index for this model.
    uint16_t n_vertices;    // Number of vertices for this model.
    int16_t x_min;          // Axis aligned bounding box minimum X.
    int16_t x_max;          // Axis aligned bounding box maximum X.
    int16_t y_min;          // Axis aligned bounding box minimum Y.
    int16_t y_max;          // Axis aligned bounding box maximum Y.
    int16_t z_min;          // Axis aligned bounding box minimum Z.
    int16_t z_max;          // Axis aligned bounding box maximum Z.
} mesh_desc_t;

typedef struct {
    uint32_t n_meshes;
    mesh_t* meshes;
} model_t;

typedef struct {
    triangle_3d_t* primitives;
    uint16_t* indices;
    bvh_node_t* nodes;
    bvh_node_t* root;
    uint16_t n_primitives;
    uint16_t node_pointer;
} bvh_t;
#endif // STRUCTS_H
