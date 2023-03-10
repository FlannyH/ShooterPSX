#ifndef MESH_H
#define MESH_H
#include <stdint.h>

#include "collision.h"


model_t* model_load(const char* path);
aabb_t triangle_get_bounds(const triangle_3d_t* self);
aabb_t collision_triangle_get_bounds(const collision_triangle_3d_t* self);

#endif