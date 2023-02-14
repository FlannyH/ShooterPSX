#include "mesh.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

Model* model_load(const char* path) {
    // Read the file
    char* file_data;
    size_t size;
    file_read(path, &file_data, &size);

    // Get header data
    ModelHeader* model_header = (ModelHeader*)file_data;

    // Ensure FMSH header is valid
    if (model_header->file_magic != 0x48534D46) { // "FMSH"
        printf("[ERROR] Error loading model '%s', file header is invalid!\n", path);
        return 0;
    }

    // Find the data sections
    void* binary_section = &model_header[1];
    MeshDesc* mesh_descriptions = (MeshDesc*)((intptr_t)binary_section + model_header->offset_mesh_desc);
    Vertex3D* vertex_data = (Vertex3D*)((intptr_t)binary_section + model_header->offset_vertex_data);

    // Create a model object
    Model* model = malloc(sizeof(Model));
    model->n_meshes = model_header->n_submeshes;
    model->meshes = malloc(sizeof(Mesh) * model_header->n_submeshes);

    // Loop over each submesh and create a model
    for (size_t i = 0; i < model_header->n_submeshes; ++i) {
        // Create a mesh object
        model->meshes[i].n_vertices = mesh_descriptions[i].n_vertices;
        model->meshes[i].vertices = &vertex_data[mesh_descriptions[i].vertex_start];
        printf("from %04X to %04X\n", sizeof(ModelHeader) + &vertex_data[mesh_descriptions[i].vertex_start] - vertex_data, sizeof(ModelHeader) + &vertex_data[mesh_descriptions[i].vertex_start + model->meshes[i].n_vertices] - vertex_data);
    }

    return model;
}