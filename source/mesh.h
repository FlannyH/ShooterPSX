#ifndef MESH_H
#define MESH_H
#include <stdint.h>

typedef struct _vertex_2d {
    int16_t x, y; // 2D position
    uint8_t r, g, b, _pad; // 8-bit RGB color (and padding for alignment)
} Vertex2D;

typedef struct _vertex_3d {
    int16_t x, y, z; // 3D position
    uint8_t r, g, b; // 8-bit RGB color
    uint8_t u, v, tex_id; // Texture Coordinates (and padding for alignment)
} Vertex3D;

typedef struct _mesh {
    uint32_t n_vertices;
    Vertex3D* vertices;
} Mesh;

typedef struct _model_header {
    uint32_t file_magic;         // File identifier magic, always "FMSH"
    uint32_t n_submeshes;       // Number of submeshes in this model.
    uint32_t offset_mesh_desc;  // Offset into the binary section to the start of the array of MeshDesc structs.
    uint32_t offset_vertex_data;// Offset into the binary section to the start of the raw VertexPSX data.
} ModelHeader;

typedef struct _mesh_desc {
    uint16_t vertex_start;  // First vertex index for this model.
    uint16_t n_vertices;    // Number of vertices for this model.
    int16_t x_min;          // Axis aligned bounding box minimum X.
    int16_t x_max;          // Axis aligned bounding box maximum X.
    int16_t y_min;          // Axis aligned bounding box minimum Y.
    int16_t y_max;          // Axis aligned bounding box maximum Y.
    int16_t z_min;          // Axis aligned bounding box minimum Z.
    int16_t z_max;          // Axis aligned bounding box maximum Z.
} MeshDesc;

typedef struct _model {
    uint32_t n_meshes;
    Mesh* meshes;
} Model;

Model* model_load(const char* path);

#endif