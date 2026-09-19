#ifndef MESH_H
#define MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "collision.h"
#include "renderer.h"

#include <stdint.h>

// todo(model_format_shared_vertices): desc: implement new model format with shared vertices

// If the model has textures, they should be allocated before loading the model
model_t* model_load(const char *path, int on_stack, stack_t stack, texture_category_t tex_category, int optimized_for_single_render_per_frame);
model_t* model_load_collision_debug(const char *path, int on_stack, stack_t stack);
collision_mesh_t* model_load_collision(const char *path, int on_stack, stack_t stack);
aabb_t triangle_get_bounds(const triangle_3d_t *self);
aabb_t collision_triangle_get_bounds(const collision_triangle_3d_t* self);
mesh_t* model_find_mesh(const model_t* model, const char* mesh_name);

size_t get_face_normals(convex_hull_mesh_t* polytope, size_t start_index, vec3_t center);
size_t convex_hull_expand(convex_hull_mesh_t* shape, vec3_t support);

mesh_t* create_convex_hull_from_point_cloud(vec3_t* points, size_t count, size_t step_limit);

#endif

#ifdef __cplusplus
}
#endif
