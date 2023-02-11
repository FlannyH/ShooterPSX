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
    uint8_t u, v, pad; // Texture Coordinates (and padding for alignment)
} Vertex3D;

typedef struct _mesh {
    uint32_t n_vertices; // The number of vertices in this mesh
    Vertex3D vertices[65536]; // Accessing elements with index >= n_vertices will cause a buffer overflow
} Mesh;

Mesh* mesh_load(const char* path);

#endif