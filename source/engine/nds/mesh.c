#include "mesh.h"

#include "../common.h"
#include "memory.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>

model_t* model_load(const char* path, int on_stack, stack_t stack, texture_category_t tex_category, int optimize_for_single_render_per_frame) {
    (void)optimize_for_single_render_per_frame;

    // todo(nds_renderer_fix): desc: fix nds renderer
    (void)tex_category;
    TODO();

    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Get header data
    model_header_t* model_header = (model_header_t*)file_data;

    // Ensure FMSH header is valid
    if (!file_data || model_header->file_magic != MAGIC_FMSH) { // "FMSH"
        printf("[ERROR] Error loading model '%s', file header is invalid!\n", path);
        return 0;
    }

    // Find the data sections
    void* binary_section = &model_header[1];
    const mesh_desc_t* mesh_descriptions = (mesh_desc_t*)((intptr_t)binary_section + model_header->offset_mesh_desc);
    vertex_3d_t* vertex_data = (vertex_3d_t*)((intptr_t)binary_section + model_header->offset_vertex_data);
    normal_t* normals = (normal_t*)((intptr_t)binary_section + model_header->offset_vertex_normals);

    // Create a model object
	model_t* model;
	if (on_stack) {
		model = mem_stack_alloc(sizeof(model_t), stack);
		model->meshes = mem_stack_alloc(sizeof(mesh_t) * model_header->n_submeshes, stack);
	}
	else {
		model = mem_alloc(sizeof(model_t), MEM_CAT_MODEL);
		model->meshes = mem_alloc(sizeof(mesh_t) * model_header->n_submeshes, MEM_CAT_MESH);
	}
    model->n_meshes = model_header->n_submeshes;

    // Loop over each submesh and create a model
    uint8_t* mesh_name_cursor = (uint8_t*)((intptr_t)binary_section + model_header->offset_mesh_names);
    for (size_t i = 0; i < model_header->n_submeshes; ++i) {
        // Get mesh name length
        uint32_t mesh_name_length = *mesh_name_cursor++;
        mesh_name_length |= (*mesh_name_cursor++) << 8;
        mesh_name_length |= (*mesh_name_cursor++) << 16;
        mesh_name_length |= (*mesh_name_cursor++) << 24;

        // Allocate memory for mesh name
        char* string = NULL;
        if (on_stack) {
            string = mem_stack_alloc(mesh_name_length + 1, stack);
        }
        else {
            string = mem_alloc(mesh_name_length + 1, MEM_CAT_MODEL);
        }

        // Copy the data into it
        for (size_t i = 0; i < mesh_name_length; ++i) {
            string[i] = *mesh_name_cursor++;
        }

        // Null-terminate it
        string[mesh_name_length] = 0;

        // Create a mesh object
        model->meshes[i].n_triangles = mesh_descriptions[i].n_triangles;
        model->meshes[i].n_quads = mesh_descriptions[i].n_quads;
        model->meshes[i].vertices = &vertex_data[mesh_descriptions[i].vertex_start];
        if (normals) {
            model->meshes[i].normals = &normals[mesh_descriptions[i].vertex_start];
        } else {
            model->meshes[i].normals = NULL;
        }
        model->meshes[i].bounds.min.x = mesh_descriptions[i].x_min;
        model->meshes[i].bounds.max.x = mesh_descriptions[i].x_max;
        model->meshes[i].bounds.min.y = mesh_descriptions[i].y_min;
        model->meshes[i].bounds.max.y = mesh_descriptions[i].y_max;
        model->meshes[i].bounds.min.z = mesh_descriptions[i].z_min;
        model->meshes[i].bounds.max.z = mesh_descriptions[i].z_max;
        model->meshes[i].name = string;
    }

    printf("done with model %s\n", path);
    return model;
}

model_t* model_load_collision_debug(const char* path, int on_stack, stack_t stack) {
    (void)path;
    (void)on_stack;
    (void)stack;
    return NULL; // unimplemented on DS
}
