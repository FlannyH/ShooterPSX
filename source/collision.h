#ifndef COLLISION_H
#define COLLISION_H
#include <stdint.h>

#include "fixed_point.h"
#include "structs.h"
#include "texture.h"

#define COL_SCALE 512 // 4096 = 1.0, 512 = 0.125. Need lower scale for less overflows

// BVH construction
aabb_t bvh_get_bounds(const bvh_t* bvh, uint16_t first, uint16_t count);
void bvh_construct(bvh_t* bvh, const col_mesh_file_tri_t* primitives, const uint16_t n_primitives);
void bvh_subdivide(bvh_t* bvh, bvh_node_t* node, int recursion_depth);
void bvh_swap_primitives(uint16_t* a, uint16_t* b);
void bvh_partition(const bvh_t* bvh, axis_t axis, scalar_t pivot, uint16_t start, uint16_t count, uint16_t* split_index);
void bvh_from_mesh(bvh_t* bvh, const mesh_t* mesh);
void bvh_from_model(bvh_t* bvh, const collision_mesh_t* mesh);
void bvh_debug_draw(const bvh_t* bvh, int min_depth, int max_depth, pixel32_t color);

// BVH intersection
void bvh_intersect_ray(bvh_t* self, ray_t ray, rayhit_t* hit);
void bvh_intersect_vertical_cylinder(bvh_t* bvh, vertical_cylinder_t ray, rayhit_t* hit);

// Primitive intersection
int point_aabb_intersect(const aabb_t* aabb, vec3_t point);
int ray_aabb_intersect(const aabb_t* aabb, ray_t ray);
int ray_aabb_intersect_fancy(const aabb_t* aabb, ray_t ray, rayhit_t* hit);
int ray_triangle_intersect(collision_triangle_3d_t* triangle, ray_t ray, rayhit_t* hit);
int vertical_cylinder_aabb_intersect(const aabb_t* aabb, vertical_cylinder_t vertical_cylinder);
int vertical_cylinder_aabb_intersect_fancy(const aabb_t* aabb, const vertical_cylinder_t vertical_cylinder, rayhit_t* hit);
int vertical_cylinder_triangle_intersect(collision_triangle_3d_t* triangle, vertical_cylinder_t vertical_cylinder, rayhit_t* hit);


// Statistics
void collision_clear_stats(void);
extern int n_ray_aabb_intersects;
extern int n_ray_triangle_intersects;
extern int n_vertical_cylinder_aabb_intersects;
extern int n_vertical_cylinder_triangle_intersects;

#endif // COLLISION_H
