#include "memory.h"
#include "mesh.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>

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
    PANIC_IF("Trying to get triangle bounds from nullptr!", self == NULL);

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
    PANIC_IF("Trying to get triangle bounds from nullptr!", self == NULL);

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
        mesh_t* mesh = &model->meshes[i];
        if (strings_are_equal(mesh->name, mesh_name)) {
            return mesh;
        }
    }
    printf("[ERROR] Could not find mesh with name '%s'!\n", mesh_name);
    return 0;
}