#ifndef COLLISION_H
#define COLLISION_H

#include "structs.h"
#include "texture.h"
#include "math/vec3.h"

#include <stdint.h>

typedef struct {
    size_t a, b, c;
    vec3_t normal;
    scalar_t distance;
} face_t;

typedef struct {
    size_t a, b;
} edge_t;

typedef struct {
    #define CONVEX_HULL_LIMIT 1024
    vec3_t vertices[CONVEX_HULL_LIMIT];
    size_t n_vertices;
    face_t faces[CONVEX_HULL_LIMIT];
    size_t n_faces;
} convex_hull_mesh_t;

int gjk(convex_hull_mesh_t* polytope, shape_t* shape1, shape_t* shape2);
vec3_t epa(convex_hull_mesh_t* polytope, shape_t* shape1, shape_t* shape2);

void move_sphere(sphere_t* shape, vec3_t move_by);
void move_capsule(capsule_t* shape, vec3_t move_by);
void move_triangle(triangle_t* shape, vec3_t move_by);
void move_aabb(aabb_t* shape, vec3_t move_by);
void move_convex_hull(convex_hull_t* shape, vec3_t move_by);
void move_shape(shape_t* shape, vec3_t move_by);

#endif // COLLISION_H
