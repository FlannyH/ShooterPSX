#ifndef COLLISION_H
#define COLLISION_H

#include "structs.h"
#include "texture.h"
#include "math/vec3.h"

#include <stdint.h>

int gjk(shape_t* shape1, shape_t* shape2);
vec3_t epa(shape_t* shape1, shape_t* shape2);

void move_sphere(sphere_t* shape, vec3_t move_by);
void move_capsule(capsule_t* shape, vec3_t move_by);
void move_triangle(triangle_t* shape, vec3_t move_by);
void move_aabb(aabb_t* shape, vec3_t move_by);
void move_shape(shape_t* shape, vec3_t move_by);

#endif // COLLISION_H
