#include "mesh.h"

#include <stdio.h>
#include <stdlib.h>

#include "file.h"

model_t* model_load(const char* path) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size);

    // Get header data
    model_header_t* model_header = (model_header_t*)file_data;

    // Ensure FMSH header is valid
    if (model_header->file_magic != 0x48534D46) { // "FMSH"
        printf("[ERROR] Error loading model '%s', file header is invalid!\n", path);
        return 0;
    }

    // Find the data sections
    void* binary_section = &model_header[1];
    const mesh_desc_t* mesh_descriptions = (mesh_desc_t*)((intptr_t)binary_section + model_header->offset_mesh_desc);
    vertex_3d_t* vertex_data = (vertex_3d_t*)((intptr_t)binary_section + model_header->offset_vertex_data);

    // Create a model object
    model_t* model = malloc(sizeof(model_t));
    model->n_meshes = model_header->n_submeshes;
    model->meshes = malloc(sizeof(mesh_t) * model_header->n_submeshes);

    // Loop over each submesh and create a model
    for (size_t i = 0; i < model_header->n_submeshes; ++i) {
        // Create a mesh object
        model->meshes[i].n_vertices = mesh_descriptions[i].n_vertices;
        model->meshes[i].vertices = &vertex_data[mesh_descriptions[i].vertex_start];
        model->meshes[i].x_min = mesh_descriptions[i].x_min;
        model->meshes[i].x_max = mesh_descriptions[i].x_max;
        model->meshes[i].y_min = mesh_descriptions[i].y_min;
        model->meshes[i].y_max = mesh_descriptions[i].y_max;
        model->meshes[i].z_min = mesh_descriptions[i].z_min;
        model->meshes[i].z_max = mesh_descriptions[i].z_max;
    }

    return model;
}