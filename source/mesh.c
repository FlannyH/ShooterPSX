#include "mesh.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

#include "file.h"

model_t* model_load(const char* path, int on_stack, stack_t stack) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

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
    for (size_t i = 0; i < model_header->n_submeshes; ++i) {
        // Get mesh name length
        uint32_t mesh_name_length = *mesh_name_cursor++;
        mesh_name_length |= (*mesh_name_cursor++) << 8;
        mesh_name_length |= (*mesh_name_cursor++) << 16;
        mesh_name_length |= (*mesh_name_cursor++) << 24;

        // Allocate memory for mesh name
        char* string;
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

#ifdef _WIN32
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
#endif
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
    collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // Verify file magic
    if (col_mesh->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
        return 0;
    }

    // Convert all vertices into visual vertices
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
    model->meshes[0].bounds = (aabb_t) {
        .max = (vec3_t) {.x = INT32_MAX, .y = INT32_MAX, .z = INT32_MAX,},
        .min = (vec3_t) {.x = INT32_MIN, .y = INT32_MIN, .z = INT32_MIN,},
    };

    col_mesh_file_vert_t* in = (col_mesh_file_vert_t*)(col_mesh + 1);
    vertex_3d_t* out = model->meshes[0].vertices;
    for (size_t i = 0; i < col_mesh->n_verts; i += 3) {
        // Calculate normal
        vec3_t a = { in[i + 0].x * 16, in[i + 0].y * 16, in[i + 0].z * 16 };
        vec3_t b = { in[i + 1].x * 16, in[i + 1].y * 16, in[i + 1].z * 16 };
        vec3_t c = { in[i + 2].x * 16, in[i + 2].y * 16, in[i + 2].z * 16 };
        vec3_t ab = vec3_sub(b, a);
        vec3_t ac = vec3_sub(c, a);
        vec3_t normal = vec3_normalize(vec3_cross(ab, ac));
        normal = vec3_add(normal, (vec3_t){ 4096, 4096, 4096 });
        normal = vec3_div(normal, (vec3_t){ 8192, 8192, 8192 });
        normal = vec3_min(normal, (vec3_t) { 4095, 4095, 4095 });

        for (size_t j = 0; j < 3; ++j) {
            out[i+j] = (vertex_3d_t){
                .r = normal.x >> 4,
                .g = normal.y >> 4,
                .b = normal.z >> 4,
                .u = 0,
                .v = 0,
                .tex_id = 0,
                .x = in[i+j].x,
                .y = in[i+j].y,
                .z = in[i+j].z,
            };
        }
    }

    return model;
}

collision_mesh_t* model_load_collision(const char* path, int on_stack, stack_t stack) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Read collision header
    collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // Verify file magic
    if (col_mesh->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
        return 0;
    }

    // Return the collision model
	collision_mesh_t* output;
	if (on_stack) output = mem_stack_alloc(sizeof(collision_mesh_t), stack);
	else output = mem_alloc(sizeof(collision_mesh_t), MEM_CAT_COLLISION);
    output->n_verts = col_mesh->n_verts;
    output->verts = (col_mesh_file_vert_t*)(col_mesh + 1);
    return output;
}

aabb_t triangle_get_bounds(const triangle_3d_t* self) {
    aabb_t result;
    result.min = vec3_min(
        vec3_min(
            vec3_from_int32s(self->v0.x, self->v0.y, self->v0.z),
            vec3_from_int32s(self->v1.x, self->v1.y, self->v1.z)
        ),
        vec3_from_int32s(self->v2.x, self->v2.y, self->v2.z)
    );
    result.max = vec3_max(
        vec3_max(
            vec3_from_int32s(self->v0.x, self->v0.y, self->v0.z),
            vec3_from_int32s(self->v1.x, self->v1.y, self->v1.z)
        ),
        vec3_from_int32s(self->v2.x, self->v2.y, self->v2.z)
    );
    return result;
}

vislist_t* model_load_vislist(const char* path, int on_stack, stack_t stack) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Get header data
    vislist_header_t* model_header = (vislist_header_t*)file_data;

    // Ensure FMSH header is valid
    if (model_header->file_magic != MAGIC_FVIS) { // "FVIS"
        printf("[ERROR] Error loading model '%s', file header is invalid!\n", path);
        return 0;
    }

    return (vislist_t*)(model_header + 1);
}

aabb_t collision_triangle_get_bounds(const collision_triangle_3d_t* self) {
    aabb_t result;
    result.min = vec3_min(
        vec3_min(
            self->v0,
            self->v1
        ),
        self->v2
    );
    result.max = vec3_max(
        vec3_max(
            self->v0,
            self->v1
        ),
        self->v2
    );
    return result;
}

int strings_are_equal(const char* a, const char* b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

mesh_t* model_find_mesh(const model_t* model, const char* mesh_name) {
    for (size_t i = 0; i < model->n_meshes; ++i) {
        const mesh_t* mesh = &model->meshes[i];
        if (strings_are_equal(mesh->name, mesh_name)) {
            return mesh;
        }
    }
    return 0;
}