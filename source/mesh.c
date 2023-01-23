#include "mesh.h"
#include "file.h"

Mesh* mesh_load(const char* path) {
    // Read the file
    char* file_data;
    size_t size;
    file_read(path, &file_data, &size);

    // Create a mesh out of it
    Mesh* mesh = (Mesh*)file_data;
    return mesh;
}