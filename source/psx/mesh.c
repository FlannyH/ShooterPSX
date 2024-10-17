#include "../memory.h"
#include "../renderer.h"
#include "../mesh.h"
#include "../file.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mesh.h"

extern pixel32_t textures_avg_colors[];
extern RECT textures[];
extern RECT palettes[];

void precompute_tex_triangle(POLY_GT3* prim, const vertex_3d_t* vertices, const int tex_id_start);
void precompute_untex_triangle(POLY_G3* prim, const vertex_3d_t* vertices, const int tex_id_start);
void precompute_tex_quad(POLY_GT4* prim, const vertex_3d_t* vertices, const int tex_id_start);
void precompute_untex_quad(POLY_G4* prim, const vertex_3d_t* vertices, const int tex_id_start);

model_t* model_load(const char* path, int on_stack, stack_t stack, int tex_id_start) {
    // Read the file and store it in the temporary stack
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, 1, STACK_TEMP);

    // Get header data
    model_header_t* model_header = (model_header_t*)file_data;

    // Ensure FMSH header is valid
    if (model_header->file_magic != MAGIC_FMSH) { // "FMSH"
        printf("[ERROR] Error loading model '%s', file header is invalid!\n", path);
        return 0;
    }

    // Find the data sections
    void* binary_section = &model_header[1];
    const mesh_desc_t* mesh_descriptions = (mesh_desc_t*)((intptr_t)binary_section + model_header->offset_mesh_desc);
    vertex_3d_t* vertex_data = (vertex_3d_t*)((intptr_t)binary_section + model_header->offset_vertex_data);

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
        printf("mesh with name: %s\n", string);

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
                precompute_tex_triangle(tex_prim, vertices, tex_id_start);
            }
            for (int i = 0; i < 3; ++i) {
                model->meshes[mesh_id].vtx_pos_and_size[vert_index + i] = (aligned_position_t) {
                    .x = vertices[i].x,
                    .y = vertices[i].y,
                    .z = vertices[i].z,
                    .poly_size = vertices[1].tex_id, // second vertex tex_id is polygon size, used for subdiv
                    .tex_id = (verts[0].tex_id >= 254) ? (verts[0].tex_id) : (verts[0].tex_id + tex_id_start) // first vertex tex_id is texture index
                };
            }
            vert_index += 3;
        }
        for (size_t poly_index = 0; poly_index < n_quads; ++poly_index) {
            const vertex_3d_t* vertices = &verts[vert_index];
            for (int i = 0; i < 2; ++i) {
                if (vertices[0].tex_id == 254) continue;
                POLY_GT4* tex_prim = &model->meshes[mesh_id].tex_quads[i][poly_index];
                precompute_tex_quad(tex_prim, vertices, tex_id_start);
            }
            for (int i = 0; i < 4; ++i) {
                model->meshes[mesh_id].vtx_pos_and_size[vert_index + i] = (aligned_position_t) {
                    .x = vertices[i].x,
                    .y = vertices[i].y,
                    .z = vertices[i].z,
                    .poly_size = vertices[1].tex_id, // second vertex tex_id is polygon size, used for subdiv
                    .tex_id = (verts[0].tex_id >= 254) ? (verts[0].tex_id) : (verts[0].tex_id + tex_id_start) // first vertex tex_id is texture index
                };
            }
            vert_index += 4;
        }
    }

    printf("done with model %s\n", path);
    return model;
}

model_t* model_load_collision_debug(const char* path, int on_stack, stack_t stack) {
    (void)path;
    (void)on_stack;
    (void)stack;
    return NULL; // todo

    // // Read the file
    // uint32_t* file_data;
    // size_t size;
    // file_read(path, &file_data, &size, on_stack, stack);

    // // Read collision header
    // collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // // Verify file magic
    // if (col_mesh->file_magic != MAGIC_FCOL) {
    //     printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
    //     return 0;
    // }

    // // Convert all vertices into visual vertices
	// model_t* model;
	// if (on_stack) {
	// 	model = mem_stack_alloc(sizeof(model_t), stack);
	// 	model->meshes = mem_stack_alloc(sizeof(mesh_t), stack);
    // 	model->meshes[0].vertices = mem_stack_alloc(sizeof(vertex_3d_t) * col_mesh->n_verts, stack);
	// }
	// else {
	// 	model = mem_alloc(sizeof(model_t), MEM_CAT_MODEL);
	// 	model->meshes = mem_alloc(sizeof(mesh_t), MEM_CAT_MESH);
    // 	model->meshes[0].vertices = mem_alloc(sizeof(vertex_3d_t) * col_mesh->n_verts, MEM_CAT_MESH);
	// }
    // model->n_meshes = 1;
    // model->meshes[0].n_quads = 0;
    // model->meshes[0].n_triangles = col_mesh->n_verts / 3;
    // model->meshes[0].bounds = (aabb_t) {
    //     .max = (vec3_t) {.x = INT32_MAX, .y = INT32_MAX, .z = INT32_MAX,},
    //     .min = (vec3_t) {.x = INT32_MIN, .y = INT32_MIN, .z = INT32_MIN,},
    // };

    // intptr_t binary = (intptr_t)(col_mesh + 1);
    // collision_triangle_3d_t* tris = (collision_triangle_3d_t*)(binary + col_mesh->triangle_data_offset);
    
    // vertex_3d_t* out = model->meshes[0].vertices;
    // for (size_t i = 0; i < col_mesh->n_verts / 3; i += 1) {
    //     // Calculate normal
    //     vec3_t normal = vec3_neg(tris[i].normal);
    //     normal.x += 4096; normal.x *= 127; normal.x /= 8192;
    //     normal.y += 4096; normal.y *= 127; normal.y /= 8192;
    //     normal.z += 4096; normal.z *= 127; normal.z /= 8192;

    //     for (size_t j = 0; j < 3; ++j) {
    //         out[(i*3)+j] = (vertex_3d_t){
    //             .r = normal.x,
    //             .g = normal.y,
    //             .b = normal.z,
    //             .u = 0,
    //             .v = 0,
    //             .tex_id = 255,
    //         };
    //     }
    //     out[(i*3)+0].x = tris[i].v0.x / COL_SCALE;
    //     out[(i*3)+0].y = tris[i].v0.y / COL_SCALE;
    //     out[(i*3)+0].z = tris[i].v0.z / COL_SCALE;
    //     out[(i*3)+1].x = tris[i].v1.x / COL_SCALE;
    //     out[(i*3)+1].y = tris[i].v1.y / COL_SCALE;
    //     out[(i*3)+1].z = tris[i].v1.z / COL_SCALE;
    //     out[(i*3)+2].x = tris[i].v2.x / COL_SCALE;
    //     out[(i*3)+2].y = tris[i].v2.y / COL_SCALE;
    //     out[(i*3)+2].z = tris[i].v2.z / COL_SCALE;
    // }

    // return model;
}

void precompute_tex_triangle(POLY_GT3* prim, const vertex_3d_t* vertices, const int tex_id_start) {
    const vertex_3d_t v0 = vertices[0];
    const vertex_3d_t v1 = vertices[1];
    const vertex_3d_t v2 = vertices[2];
    assert((palettes[v0.tex_id + tex_id_start].y & 0xF) == 0 && "Misaligned palette!");

    setPolyGT3(prim);
    setRGB0(prim, v0.r >> 1, v0.g >> 1, v0.b >> 1);
    setRGB1(prim, v1.r >> 1, v1.g >> 1, v1.b >> 1);
    setRGB2(prim, v2.r >> 1, v2.g >> 1, v2.b >> 1);

    // todo: maybe unhardcode this
    const uint8_t tex_offset_x = ((v0.tex_id + tex_id_start) & 0b00001100) << 4;
    const uint8_t tex_offset_y = ((v0.tex_id + tex_id_start) & 0b00000011) << 6;
    setUV3(prim,
        (v0.u >> 2) + tex_offset_x, (v0.v >> 2) + tex_offset_y,
        (v1.u >> 2) + tex_offset_x, (v1.v >> 2) + tex_offset_y,
        (v2.u >> 2) + tex_offset_x, (v2.v >> 2) + tex_offset_y
    );

    prim->clut = getClut(palettes[v0.tex_id + tex_id_start].x, palettes[v0.tex_id + tex_id_start].y); // note: when rendering, palette_y = palette_y & 0xF + clut_fade
    prim->tpage = getTPage(0, 0, textures[v0.tex_id + tex_id_start].x, textures[v0.tex_id + tex_id_start].y);
}

void precompute_tex_quad(POLY_GT4* prim, const vertex_3d_t* vertices, const int tex_id_start) {
    const vertex_3d_t v0 = vertices[0];
    const vertex_3d_t v1 = vertices[1];
    const vertex_3d_t v2 = vertices[2];
    const vertex_3d_t v3 = vertices[3];
    assert((palettes[v0.tex_id + tex_id_start].y & 0xF) == 0 && "Misaligned palette!");

    setPolyGT4(prim);
    setRGB0(prim, v0.r >> 1, v0.g >> 1, v0.b >> 1);
    setRGB1(prim, v1.r >> 1, v1.g >> 1, v1.b >> 1);
    setRGB2(prim, v2.r >> 1, v2.g >> 1, v2.b >> 1);
    setRGB3(prim, v3.r >> 1, v3.g >> 1, v3.b >> 1);

    // todo: maybe unhardcode this
    const uint8_t tex_offset_x = ((v0.tex_id + tex_id_start) & 0b00001100) << 4;
    const uint8_t tex_offset_y = ((v0.tex_id + tex_id_start) & 0b00000011) << 6;
    setUV4(prim,
        (v0.u >> 2) + tex_offset_x, (v0.v >> 2) + tex_offset_y,
        (v1.u >> 2) + tex_offset_x, (v1.v >> 2) + tex_offset_y,
        (v2.u >> 2) + tex_offset_x, (v2.v >> 2) + tex_offset_y,
        (v3.u >> 2) + tex_offset_x, (v3.v >> 2) + tex_offset_y
    );

    prim->clut = getClut(palettes[v0.tex_id + tex_id_start].x, palettes[v0.tex_id + tex_id_start].y); // note: when rendering, palette_y = palette_y & 0xF + clut_fade
    prim->tpage = getTPage(0, 0, textures[v0.tex_id + tex_id_start].x, textures[v0.tex_id + tex_id_start].y);
}

void precompute_untex_quad(POLY_G4* prim, const vertex_3d_t* vertices, const int tex_id_start) {
    const vertex_3d_t v0 = vertices[0];
    const vertex_3d_t v1 = vertices[1];
    const vertex_3d_t v2 = vertices[2];
    const vertex_3d_t v3 = vertices[3];

    setPolyG4(prim);
    setRGB0(prim,
        mul_8x8(v0.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v0.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v0.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB1(prim,
        mul_8x8(v1.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v1.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v1.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB2(prim,
        mul_8x8(v2.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v2.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v2.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
    setRGB2(prim,
        mul_8x8(v3.r, textures_avg_colors[v0.tex_id + tex_id_start].r),
        mul_8x8(v3.g, textures_avg_colors[v0.tex_id + tex_id_start].g),
        mul_8x8(v3.b, textures_avg_colors[v0.tex_id + tex_id_start].b)
    );
}
