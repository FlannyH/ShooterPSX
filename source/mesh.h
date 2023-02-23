#ifndef MESH_H
#define MESH_H
#include <stdint.h>

#include "collision.h"


model_t* model_load(const char* path);
aabb_t triangle_get_bounds(triangle_3d_t* self);

#endif