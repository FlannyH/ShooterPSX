#include "memory.h"
#include "mesh.h"
#include "file.h"

#include <stdlib.h>
#include <stdio.h>

model_t* model_load(const char* path, int on_stack, stack_t stack, int tex_id_start, int optimize_for_single_render_per_frame) {
    (void)tex_id_start;
    (void)optimize_for_single_render_per_frame;
    
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Get header data
    const model_header_t* model_header = (model_header_t*)file_data;

    // Ensure FMSH header is valid
    if (model_header->file_magic != MAGIC_FMSH) { // "FMSH"
        printf("[ERROR] Error loading model '%s', file header is invalid!\n", path);
        return 0;
    }

    // Find the data sections
    const void* binary_section = &model_header[1];
    const mesh_desc_t* mesh_descriptions = (mesh_desc_t*)((intptr_t)binary_section + model_header->offset_mesh_desc);
    vertex_3d_t* vertex_data = (vertex_3d_t*)((intptr_t)binary_section + model_header->offset_vertex_data);

    // Create a model
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

    // Loop over each submesh and populate the model
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
        model->meshes[i].bounds.min.x = mesh_descriptions[i].x_min;
        model->meshes[i].bounds.max.x = mesh_descriptions[i].x_max;
        model->meshes[i].bounds.min.y = mesh_descriptions[i].y_min;
        model->meshes[i].bounds.max.y = mesh_descriptions[i].y_max;
        model->meshes[i].bounds.min.z = mesh_descriptions[i].z_min;
        model->meshes[i].bounds.max.z = mesh_descriptions[i].z_max;
        model->meshes[i].name = string;

        // Convert quads to triangles
        mesh_t* mesh = &model->meshes[i];
        vertex_3d_t* new_verts = mem_alloc(((mesh->n_triangles * 3) + (mesh->n_quads * 6)) * sizeof(vertex_3d_t), MEM_CAT_MODEL);

        // Copy the vertex texture ids to each vertex instead of just the first. OpenGL is annoying about this.
        size_t k = 0;
        size_t l = 0;
        for (size_t j = 0; j < mesh->n_triangles; ++j) {
            mesh->vertices[k + 1].tex_id = mesh->vertices[k + 0].tex_id;
            mesh->vertices[k + 2].tex_id = mesh->vertices[k + 0].tex_id;
            new_verts[l + 0] = mesh->vertices[k + 0];
            new_verts[l + 1] = mesh->vertices[k + 1];
            new_verts[l + 2] = mesh->vertices[k + 2];
            k += 3;
            l += 3;
        }
        for (size_t j = 0; j < mesh->n_quads; ++j) {
            mesh->vertices[k + 1].tex_id = mesh->vertices[k + 0].tex_id;
            mesh->vertices[k + 2].tex_id = mesh->vertices[k + 0].tex_id;
            mesh->vertices[k + 3].tex_id = mesh->vertices[k + 0].tex_id;
            new_verts[l + 0] = mesh->vertices[k + 0];
            new_verts[l + 1] = mesh->vertices[k + 1];
            new_verts[l + 2] = mesh->vertices[k + 2];
            new_verts[l + 3] = mesh->vertices[k + 2];
            new_verts[l + 4] = mesh->vertices[k + 1];
            new_verts[l + 5] = mesh->vertices[k + 3];
            k += 4;
            l += 6;
        }
        mesh->vertices = new_verts;
        mesh->n_triangles += mesh->n_quads * 2;
        mesh->n_quads = 0;
    }
    printf("done with model %s\n", path);
    return model;
}

model_t* model_load_collision_debug(const char* path, int on_stack, stack_t stack) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Read collision header
    const collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // Verify file magic
    if (col_mesh->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
        return 0;
    }

    // Allocate space for visual mesh
	model_t* model;
	if (on_stack) {
		model = mem_stack_alloc(sizeof(model_t), stack);
		model->meshes = mem_stack_alloc(sizeof(mesh_t), stack);
    	model->meshes[0].vertices = mem_stack_alloc(sizeof(vertex_3d_t) * col_mesh->n_verts, stack);
	}
	else {
		model = mem_alloc(sizeof(model_t), MEM_CAT_MODEL);
		model->meshes = mem_alloc(sizeof(mesh_t), MEM_CAT_MESH);
    	model->meshes[0].vertices = mem_alloc(sizeof(vertex_3d_t) * col_mesh->n_verts, MEM_CAT_MESH);
	}
    model->n_meshes = 1;
    model->meshes[0].n_quads = 0;
    model->meshes[0].n_triangles = col_mesh->n_verts / 3;

    // Since collision model is only meant to be see in the level 
    // editor, don't bother calculating bounding boxes for culling
    model->meshes[0].bounds = (aabb_t) {
        .max = (vec3_t) {.x = INT32_MAX, .y = INT32_MAX, .z = INT32_MAX,},
        .min = (vec3_t) {.x = INT32_MIN, .y = INT32_MIN, .z = INT32_MIN,},
    };

    // Find the collision triangle data
    const intptr_t binary = (intptr_t)(col_mesh + 1);
    const collision_triangle_3d_t* tris = (collision_triangle_3d_t*)(binary + col_mesh->triangle_data_offset);
    
    // We need graphics vertices to be able to render the 
    // model, so let's convert them one triangle at a time
    vertex_3d_t* out = model->meshes[0].vertices;

    for (size_t i = 0; i < col_mesh->n_verts / 3; i += 1) {
        // Generate vertex colors based on the triangle's normal
        vec3_t normal = vec3_neg(tris[i].normal);
        normal.x += 4096; normal.x *= 127; normal.x /= 8192;
        normal.y += 4096; normal.y *= 127; normal.y /= 8192;
        normal.z += 4096; normal.z *= 127; normal.z /= 8192;

        for (size_t j = 0; j < 3; ++j) {
            out[(i*3)+j] = (vertex_3d_t){
                .r = normal.x,
                .g = normal.y,
                .b = normal.z,
                .u = 0,
                .v = 0,
                .tex_id = 255,
            };
        }

        // Scale collision space vertex position data to graphics space
        out[(i*3)+0].x = tris[i].v0.x / COL_SCALE;
        out[(i*3)+0].y = tris[i].v0.y / COL_SCALE;
        out[(i*3)+0].z = tris[i].v0.z / COL_SCALE;
        out[(i*3)+1].x = tris[i].v1.x / COL_SCALE;
        out[(i*3)+1].y = tris[i].v1.y / COL_SCALE;
        out[(i*3)+1].z = tris[i].v1.z / COL_SCALE;
        out[(i*3)+2].x = tris[i].v2.x / COL_SCALE;
        out[(i*3)+2].y = tris[i].v2.y / COL_SCALE;
        out[(i*3)+2].z = tris[i].v2.z / COL_SCALE;
    }

    if (!on_stack) mem_free(file_data);

    return model;
}
