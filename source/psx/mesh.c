#include "../mesh.h"

#include "renderer.h"
#include "../renderer.h"
#include "../memory.h"
#include "../common.h"
#include "../file.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void precompute_tex_triangle(POLY_GT3* prim, const vertex_3d_t* vertices, texture_category_t tex_category);
void precompute_tex_quad(POLY_GT4* prim, const vertex_3d_t* vertices, texture_category_t tex_category);

model_t* model_load(const char* path, int on_stack, stack_t stack, texture_category_t tex_category, int optimize_for_single_render_per_frame) {
    // Read the file and store it in the temporary stack
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, 1, STACK_TEMP);

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
    const vertex_3d_t* vertex_data = (vertex_3d_t*)((intptr_t)binary_section + model_header->offset_vertex_data);

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
    for (size_t mesh_id = 0; mesh_id < model_header->n_submeshes; ++mesh_id) {
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
        model->meshes[mesh_id].n_triangles = mesh_descriptions[mesh_id].n_triangles;
        model->meshes[mesh_id].n_quads = mesh_descriptions[mesh_id].n_quads;
        model->meshes[mesh_id].bounds.min.x = mesh_descriptions[mesh_id].x_min;
        model->meshes[mesh_id].bounds.max.x = mesh_descriptions[mesh_id].x_max;
        model->meshes[mesh_id].bounds.min.y = mesh_descriptions[mesh_id].y_min;
        model->meshes[mesh_id].bounds.max.y = mesh_descriptions[mesh_id].y_max;
        model->meshes[mesh_id].bounds.min.z = mesh_descriptions[mesh_id].z_min;
        model->meshes[mesh_id].bounds.max.z = mesh_descriptions[mesh_id].z_max;
        model->meshes[mesh_id].name = string;
        model->meshes[mesh_id].optimized_for_single_render_per_frame = optimize_for_single_render_per_frame;

        // Pre-allocate the GPU primitive buffers - let's hope this fits!
        const size_t n_tris = model->meshes[mesh_id].n_triangles;
        const size_t n_quads = model->meshes[mesh_id].n_quads;
        if (on_stack) {
            for (int i = 0; i < 2; ++i) {
                model->meshes[mesh_id].tex_tris[i] = mem_stack_alloc(n_tris * sizeof(POLY_GT3), stack);
                model->meshes[mesh_id].tex_quads[i] = mem_stack_alloc(n_quads * sizeof(POLY_GT4), stack);
            }
            model->meshes[mesh_id].vtx_pos_and_size = mem_stack_alloc((n_tris * 3 + n_quads * 4) * sizeof(aligned_position_t), stack);
        }
        else {
            for (int i = 0; i < 2; ++i) {
                model->meshes[mesh_id].tex_tris[i] = mem_alloc(n_tris * sizeof(POLY_GT3), MEM_CAT_MESH);
                model->meshes[mesh_id].tex_quads[i] = mem_alloc(n_quads * sizeof(POLY_GT4), MEM_CAT_MESH);
            }
            model->meshes[mesh_id].vtx_pos_and_size = mem_alloc((n_tris * 3 + n_quads * 4) * sizeof(aligned_position_t), MEM_CAT_MESH);
        }

        const vertex_3d_t* verts = &vertex_data[mesh_descriptions[mesh_id].vertex_start];

        size_t vert_index = 0;
        for (size_t poly_index = 0; poly_index < n_tris; ++poly_index) {
            const vertex_3d_t* vertices = &verts[vert_index];
            for (int i = 0; i < 2; ++i) {
                if (vertices[0].tex_id == 254) continue;
                POLY_GT3* tex_prim = &model->meshes[mesh_id].tex_tris[i][poly_index];
                precompute_tex_triangle(tex_prim, vertices, tex_category);
            }
            for (int i = 0; i < 3; ++i) {
                model->meshes[mesh_id].vtx_pos_and_size[vert_index + i] = (aligned_position_t) {
                    .x = vertices[i].x,
                    .y = vertices[i].y,
                    .z = vertices[i].z,
                    .poly_size = vertices[1].tex_id, // second vertex tex_id is polygon size, used for subdiv
                    .tex_id = verts[0].tex_id // first vertex tex_id is texture index
                };
            }
            vert_index += 3;
        }
        for (size_t poly_index = 0; poly_index < n_quads; ++poly_index) {
            const vertex_3d_t* vertices = &verts[vert_index];
            for (int i = 0; i < 2; ++i) {
                if (vertices[0].tex_id == 254) continue;
                POLY_GT4* tex_prim = &model->meshes[mesh_id].tex_quads[i][poly_index];
                precompute_tex_quad(tex_prim, vertices, tex_category);
            }
            for (int i = 0; i < 4; ++i) {
                model->meshes[mesh_id].vtx_pos_and_size[vert_index + i] = (aligned_position_t) {
                    .x = vertices[i].x,
                    .y = vertices[i].y,
                    .z = vertices[i].z,
                    .poly_size = vertices[1].tex_id, // second vertex tex_id is polygon size, used for subdiv
                    .tex_id = verts[0].tex_id // first vertex tex_id is texture index
                };
            }
            vert_index += 4;
        }
    }
#ifdef _DEBUG_VERBOSE
    printf("done with model %s\n", path);
#endif
    return model;
}

model_t* model_load_collision_debug(const char* path, int on_stack, stack_t stack) {
    (void)path;
    (void)on_stack;
    (void)stack;
    return NULL; // unimplemented on PS1
}

void precompute_tex_triangle(POLY_GT3* prim, const vertex_3d_t* vertices, texture_category_t tex_category) {
    const vertex_3d_t v0 = vertices[0];
    const vertex_3d_t v1 = vertices[1];
    const vertex_3d_t v2 = vertices[2];
    const texture_entry_t* entry = renderer_get_texture_entry(tex_category, (int)v0.tex_id);

    setPolyGT3(prim);
    setRGB0(prim, v0.r >> 1, v0.g >> 1, v0.b >> 1);
    setRGB1(prim, v1.r >> 1, v1.g >> 1, v1.b >> 1);
    setRGB2(prim, v2.r >> 1, v2.g >> 1, v2.b >> 1);

    setUV3(prim,
        (v0.u >> 2) + entry->offset_u, (v0.v >> 2) + entry->offset_v,
        (v1.u >> 2) + entry->offset_u, (v1.v >> 2) + entry->offset_v,
        (v2.u >> 2) + entry->offset_u, (v2.v >> 2) + entry->offset_v
    );

    prim->clut = entry->clut; // note: when rendering, offset the Y coordinate by clut_fade for the distance fog effect
    prim->tpage = entry->tpage;
}

void precompute_tex_quad(POLY_GT4* prim, const vertex_3d_t* vertices, texture_category_t tex_category) {
    const vertex_3d_t v0 = vertices[0];
    const vertex_3d_t v1 = vertices[1];
    const vertex_3d_t v2 = vertices[2];
    const vertex_3d_t v3 = vertices[3];
    const texture_entry_t* entry = renderer_get_texture_entry(tex_category, (int)v0.tex_id);

    setPolyGT4(prim);
    setRGB0(prim, v0.r >> 1, v0.g >> 1, v0.b >> 1);
    setRGB1(prim, v1.r >> 1, v1.g >> 1, v1.b >> 1);
    setRGB2(prim, v2.r >> 1, v2.g >> 1, v2.b >> 1);
    setRGB3(prim, v3.r >> 1, v3.g >> 1, v3.b >> 1);

    setUV4(prim,
        (v0.u >> 2) + entry->offset_u, (v0.v >> 2) + entry->offset_v,
        (v1.u >> 2) + entry->offset_u, (v1.v >> 2) + entry->offset_v,
        (v2.u >> 2) + entry->offset_u, (v2.v >> 2) + entry->offset_v,
        (v3.u >> 2) + entry->offset_u, (v3.v >> 2) + entry->offset_v
    );

    prim->clut = entry->clut; // note: when rendering, offset the Y coordinate by clut_fade for the distance fog effect
    prim->tpage = entry->tpage;
}
