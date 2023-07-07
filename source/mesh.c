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
    if (model_header->file_magic != MAGIC_FMSH) { // "FMSH"
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
        model->meshes[i].n_triangles = mesh_descriptions[i].n_triangles;
        model->meshes[i].n_quads = mesh_descriptions[i].n_quads;
        model->meshes[i].vertices = &vertex_data[mesh_descriptions[i].vertex_start];
        model->meshes[i].bounds.min.x = mesh_descriptions[i].x_min;
        model->meshes[i].bounds.max.x = mesh_descriptions[i].x_max;
        model->meshes[i].bounds.min.y = mesh_descriptions[i].y_min;
        model->meshes[i].bounds.max.y = mesh_descriptions[i].y_max;
        model->meshes[i].bounds.min.z = mesh_descriptions[i].z_min;
        model->meshes[i].bounds.max.z = mesh_descriptions[i].z_max;

#ifdef _WIN32
        // Swizzle the quads
        mesh_t* mesh = &model->meshes[i];
        for (size_t i = 0; i < mesh->n_quads; ++i) {
            const size_t index = (mesh->n_triangles * 3) + (i * 4);
            vertex_3d_t temp = mesh->vertices[index + 2];
            mesh->vertices[index + 2] = mesh->vertices[index + 3];
            mesh->vertices[index + 3] = temp;
        }
#endif
    }
    return model;
}

model_t* model_load_collision_debug(const char* path) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size);

    // Read collision header
    collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // Verify file magic
    if (col_mesh->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
        return 0;
    }

    // Convert all vertices into visual vertices
    model_t* model = malloc(sizeof(model_t));
    model->meshes = malloc(sizeof(mesh_t));
    model->n_meshes = 1,
    model->meshes[0].n_quads = 0;
    model->meshes[0].n_triangles = col_mesh->n_verts / 3;
    model->meshes[0].vertices = malloc(sizeof(vertex_3d_t) * col_mesh->n_verts);
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

collision_mesh_t* model_load_collision(const char* path) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size);

    // Read collision header
    collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // Verify file magic
    if (col_mesh->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
        return 0;
    }

    // Return the collision model
    collision_mesh_t* output = malloc(sizeof(collision_mesh_t));
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
