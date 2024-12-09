#ifndef COLLISION_H
#define COLLISION_H

#include "fixed_point.h"
#include "structs.h"
#include "texture.h"

#include <stdint.h>

#define COL_SCALE 512 // 4096 = 1.0, 512 = 0.125. Need lower scale for less overflows

// BVH construction
level_collision_t bvh_from_file(const char* path, int on_stack, stack_t stack);
void bvh_debug_draw(const level_collision_t* bvh, int min_depth, int max_depth, pixel32_t color);
void bvh_debug_draw_nav_graph(const level_collision_t* bvh);

// BVH intersection
void bvh_intersect_ray(level_collision_t* self, ray_t ray, rayhit_t* hit);
void bvh_intersect_vertical_cylinder(level_collision_t* bvh, vertical_cylinder_t ray, rayhit_t* hit);

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

#endif // COLLISION_H
