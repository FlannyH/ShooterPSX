#ifndef COLLISION_H
#define COLLISION_H
#include <stdint.h>

#include "fixed_point.h"

typedef struct {
    vec3_t min;
    vec3_t max;
} aabb_t;

typedef struct {
    aabb_t bounds;                        // Axis aligned bounding box around all primitives inside this node
    uint16_t left_first;                // If this is a leaf, this is the index of the first primitive, otherwise, this is the index of the first of two child nodes
    uint16_t is_leaf : 1;               // The high bit of the count determines whether this is a node or a leaf
    uint16_t primitive_count : 15;      // Number of primitives in this leaf. If this node isn't a leaf, ignore this value
} bvh_node_t;

typedef struct {
    vec3_t position;
    vec3_t direction;
} ray_t;

typedef struct {
    int dummy;
} bvh_t;

void bvh_construct(bvh_t* self);
void bvh_intersect(bvh_t* self);

#endif // COLLISION_H
