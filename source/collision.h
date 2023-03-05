#ifndef COLLISION_H
#define COLLISION_H
#include <stdint.h>

#include "fixed_point.h"
#include "structs.h"
#include "texture.h"

// BVH construction
aabb_t bvh_get_bounds(const bvh_t* self, uint16_t first, uint16_t count);
void bvh_construct(bvh_t* self, const triangle_3d_t* primitives, uint16_t n_primitives);
void bvh_subdivide(bvh_t* self, bvh_node_t* node, int recursion_depth);
void bvh_swap_primitives(uint16_t* a, uint16_t* b);
void bvh_partition(const bvh_t* self, axis_t axis, scalar_t pivot, uint16_t start, uint16_t count, uint16_t* split_index);
void bvh_from_mesh(bvh_t* self, const mesh_t* mesh);
void bvh_from_model(bvh_t* self, const model_t* mesh);
void bvh_debug_draw(const bvh_t* self, int min_depth, int max_depth, pixel32_t color);

// BVH intersection
void bvh_intersect_ray(bvh_t* self, ray_t ray, rayhit_t* hit);
void bvh_intersect_sphere(bvh_t* self, sphere_t ray, rayhit_t* hit);

// Primitive intersection
int ray_aabb_intersect(const aabb_t* self, ray_t ray);
int ray_triangle_intersect(const collision_triangle_3d_t* self, ray_t ray, rayhit_t* hit);
int sphere_aabb_intersect(const aabb_t* self, sphere_t sphere);
int sphere_triangle_intersect(const collision_triangle_3d_t* self, sphere_t sphere, rayhit_t* hit);

#endif // COLLISION_H
