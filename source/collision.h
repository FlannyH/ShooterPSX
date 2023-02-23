#ifndef COLLISION_H
#define COLLISION_H
#include <stdint.h>

#include "fixed_point.h"
#include "structs.h"
#include "texture.h"

aabb_t bvh_get_bounds(const bvh_t* self, uint16_t first, uint16_t count);
void bvh_construct(bvh_t* self, triangle_3d_t* primitives, uint16_t n_primitives);
void bvh_subdivide(bvh_t* self, bvh_node_t* node, int recursion_depth);
void bvh_intersect(bvh_t* self);
void bvh_swap_primitives(uint16_t* a, uint16_t* b);
void bvh_partition(const bvh_t* self, axis_t axis, scalar_t pivot, uint16_t start, uint16_t count, uint16_t* split_index);
void bvh_from_mesh(bvh_t* self, const mesh_t* mesh);
void bvh_debug_draw(const bvh_t* self, int min_depth, int max_depth, pixel32_t color);

#endif // COLLISION_H
