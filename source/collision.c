#include <stdio.h>

#include "collision.h"
#include "level.h"
#include "mesh.h"
#include "math/fixed_point.h"
#include "structs.h"
#include "math/vec3.h"

#include <assert.h>
#include <string.h>
#define MAX_EPA_TRIES 8

void dump_current_simplex(size_t i, convex_hull_mesh_t* polytope, size_t closest_face) {
#ifdef _PC
    char path[1024] = {0};
    snprintf(path, 1023, "simplex_%i.obj", i);
    FILE* o = fopen(&path[0], "wb");
    fprintf(o, "o simplex_%lu\n", i);
    for (size_t i = 0; i < polytope->n_vertices; ++i) {
        fprintf(o, "v %.3f %.3f %.3f\n",
            ((float)polytope->vertices[i].x) / 4096.f,
            ((float)polytope->vertices[i].y) / 4096.f,
            ((float)polytope->vertices[i].z) / 4096.f
        );
    }
    for (size_t i = 0; i < polytope->n_faces; ++i) {
        fprintf(o, "vn %.3f %.3f %.3f\n",
            ((float)polytope->faces[i].normal.x) / 4096.f,
            ((float)polytope->faces[i].normal.y) / 4096.f,
            ((float)polytope->faces[i].normal.z) / 4096.f
        );
    }
    for (size_t i = 0; i < polytope->n_faces; ++i) {
        fprintf(o, "f %i//%i %i//%i %i//%i\n",
           polytope->faces[i].a + 1, i+1,
           polytope->faces[i].b + 1, i+1,
           polytope->faces[i].c + 1, i+1
        );
    }

    fprintf(o, "o simplex_%lu_closest_face\n", i);
    fprintf(o, "v %.3f %.3f %.3f\n",
        ((float)polytope->vertices[polytope->faces[closest_face].a].x) / 4096.f,
        ((float)polytope->vertices[polytope->faces[closest_face].a].y) / 4096.f,
        ((float)polytope->vertices[polytope->faces[closest_face].a].z) / 4096.f
    );
    fprintf(o, "v %.3f %.3f %.3f\n",
        ((float)polytope->vertices[polytope->faces[closest_face].a].x) / 4096.f,
        ((float)polytope->vertices[polytope->faces[closest_face].a].y) / 4096.f,
        ((float)polytope->vertices[polytope->faces[closest_face].a].z) / 4096.f
    );
    fprintf(o, "v %.3f %.3f %.3f\n",
        ((float)polytope->vertices[polytope->faces[closest_face].a].x) / 4096.f,
        ((float)polytope->vertices[polytope->faces[closest_face].a].y) / 4096.f,
        ((float)polytope->vertices[polytope->faces[closest_face].a].z) / 4096.f
    );
    fprintf(o, "vn %.3f %.3f %.3f\n",
        ((float)polytope->faces[closest_face].normal.x) / 4096.f,
        ((float)polytope->faces[closest_face].normal.y) / 4096.f,
        ((float)polytope->faces[closest_face].normal.z) / 4096.f
    );
    fprintf(o, "f 1//1 2//1 3//1\n");
    fclose(o);
#endif
}

// directions are normalized to make it more predictable with fixed point precision

#define SUPPORT(a, b, dir) vec3_sub(support_shape((a), (dir)), support_shape((b), vec3_neg(dir)))

vec3_t support_sphere(sphere_t sphere, vec3_t direction) {
    direction = vec3_normalize(direction);
    return vec3_add(sphere.center, vec3_muls(direction, sphere.radius));
}

vec3_t support_capsule(capsule_t capsule, vec3_t direction) {
    direction = vec3_normalize(direction);
    scalar_t score_a = vec3_dot(capsule.a, direction);
    scalar_t score_b = vec3_dot(capsule.b, direction);
    if (score_a > score_b) return vec3_add(capsule.a, vec3_muls(direction, capsule.radius));
    else return vec3_add(capsule.b, vec3_muls(direction, capsule.radius));
}

vec3_t support_triangle(triangle_t triangle, vec3_t direction) {
    direction = vec3_normalize(direction);
    const scalar_t score_a = vec3_dot(triangle.v0, direction);
    const scalar_t score_b = vec3_dot(triangle.v1, direction);
    const scalar_t score_c = vec3_dot(triangle.v2, direction);
    if ((score_a > score_b) && (score_a > score_c)) return triangle.v0;
    else if (score_b > score_c) return triangle.v1;
    return triangle.v2;
}

vec3_t support_aabb(aabb_t aabb, vec3_t direction) {
    direction = vec3_normalize(direction);
    vec3_t result = {0};

    if (direction.x > 0) result.x = aabb.max.x;
    else result.x = aabb.min.x;

    if (direction.y > 0) result.y = aabb.max.y;
    else result.y = aabb.min.y;

    if (direction.z > 0) result.z = aabb.max.z;
    else result.z = aabb.min.z;

    return result;
}

vec3_t support_convex_hull(convex_hull_t convex_hull, vec3_t direction) {
    direction = vec3_normalize(direction);
    scalar_t highest_dot = vec3_dot(convex_hull.points[0], direction);
    vec3_t result = convex_hull.points[0];

    for (size_t i = 1; i < convex_hull.n_points; ++i) {
        const scalar_t dot = vec3_dot(convex_hull.points[i], direction);
        if (dot > highest_dot) {
            highest_dot = dot;
            result = convex_hull.points[i];
        }
    }

    return result;
}

vec3_t support_shape(shape_t* shape, vec3_t direction) {
    // direction with magnitude 0 is sus, but let's not crash and just return zero
    if (direction.x == 0 && direction.y == 0 && direction.z == 0) return (vec3_t){0};

    switch (shape->type) {
        case SHAPE_NONE:     return (vec3_t){0};
        case SHAPE_SPHERE:      return support_sphere(shape->sphere, direction);
        case SHAPE_CAPSULE:     return support_capsule(shape->capsule, direction);
        case SHAPE_TRIANGLE:    return support_triangle(shape->triangle, direction);
        case SHAPE_AABB:        return support_aabb(shape->aabb, direction);
        case SHAPE_CONVEX_HULL: return support_convex_hull(shape->convex_hull, direction);
        default:                return (vec3_t){0};
    }
}

#define SIMILAR_DIR(a, b) (vec3_dot((a), (b)) > 0)

int handle_line_case(convex_hull_mesh_t* polytope, vec3_t* dir) {
    // printf("line case\n");
    const vec3_t a = polytope->vertices[0];
    const vec3_t b = polytope->vertices[1];

    const vec3_t ab = vec3_normalize(vec3_sub(b, a));
    const vec3_t ao = vec3_normalize(vec3_neg(a));

    if (SIMILAR_DIR(ab, ao)) {
        // polytope->vertices[0] = a;
        // polytope->vertices[1] = b;
        // polytope->n_vertices = 2;
        *dir = vec3_cross_lh(vec3_cross_lh(ab, ao), ab);
    }
    else {
        polytope->vertices[0] = a;
        polytope->n_vertices = 1;
        *dir = ao;
    }

    return 0;
}

int handle_triangle_case(convex_hull_mesh_t* polytope, vec3_t* dir) {
    // printf("triangle case\n");

    const vec3_t a = polytope->vertices[0];
    const vec3_t b = polytope->vertices[1];
    const vec3_t c = polytope->vertices[2];

    const vec3_t ab = vec3_normalize(vec3_sub(b, a));
    const vec3_t ac = vec3_sub(c, a);
    const vec3_t ao = vec3_neg(a);

    const vec3_t abc = vec3_normalize(vec3_cross_lh(ab, ac));

    if (SIMILAR_DIR(vec3_cross_lh(abc, ac), ao)) {
        if (SIMILAR_DIR(ac, ao)) {
            // polytope->vertices[0] = a;
            polytope->vertices[1] = c;
            polytope->n_vertices = 2;
            *dir = vec3_normalize(vec3_cross_lh(vec3_cross_lh(ac, ao), ac));
        }
        else {
            // polytope->vertices[0] = a;
            // polytope->vertices[1] = b;
            polytope->n_vertices = 2;
            // *dir = s.dir;
            return handle_line_case(polytope, dir);
        }
    }
    else {
        if (SIMILAR_DIR(vec3_cross_lh(ab, abc), ao)) {
            // polytope->vertices[0] = a;
            // polytope->vertices[1] = b;
            polytope->n_vertices = 2;
            // *dir = s.dir;
            return handle_line_case(polytope, dir);
        }
        else {
            if (SIMILAR_DIR(abc, ao)) {
                // polytope->vertices[0] = a;
                // polytope->vertices[1] = b;
                // polytope->vertices[2] = c;
                // polytope->n_vertices = 3;
                *dir = abc;
            }
            else {
                // polytope->vertices[0] = a;
                polytope->vertices[1] = c;
                polytope->vertices[2] = b;
                // polytope->n_vertices = 3;
                *dir = vec3_neg(abc);
            }
        }
    }

    return 0;
}

int handle_tetrahedron_case(convex_hull_mesh_t* polytope, vec3_t* dir) {
    // printf("tetrahedron case\n");

    const vec3_t a = polytope->vertices[0];
    const vec3_t b = polytope->vertices[1];
    const vec3_t c = polytope->vertices[2];
    const vec3_t d = polytope->vertices[3];

    const vec3_t ab = vec3_normalize(vec3_sub(b, a));
    const vec3_t ac = vec3_normalize(vec3_sub(c, a));
    const vec3_t ad = vec3_normalize(vec3_sub(d, a));
    const vec3_t ao = vec3_neg(a);

    vec3_t abc = vec3_cross_lh(ab, ac);
    vec3_t acd = vec3_cross_lh(ac, ad);
    vec3_t adb = vec3_cross_lh(ad, ab);

    if (SIMILAR_DIR(abc, ao)) {
        // polytope->vertices[0] = a;
        // polytope->vertices[1] = b;
        // polytope->vertices[2] = c;
        polytope->n_vertices = 3;
        // *dir = *dir;
        return handle_triangle_case(polytope, dir);
    }
    if (SIMILAR_DIR(acd, ao)) {
        // polytope->vertices[0] = a;
        polytope->vertices[1] = c;
        polytope->vertices[2] = d;
        polytope->n_vertices = 3;
        // *dir = *dir;
        return handle_triangle_case(polytope, dir);
    }
    if (SIMILAR_DIR(adb, ao)) {
        // polytope->vertices[0] = a;
        polytope->vertices[1] = d;
        polytope->vertices[2] = b;
        polytope->n_vertices = 3;
        // *dir = *dir;
        return handle_triangle_case(polytope, dir);
    }
    return 1;
}

// returns if simplex surrounds origin, and updates the simplex to be ready for the next point
int update_simplex(convex_hull_mesh_t* polytope, vec3_t* dir) {
    switch (polytope->n_vertices) {
        case 2: return handle_line_case(polytope, dir);
        case 3: return handle_triangle_case(polytope, dir);
        case 4: return handle_tetrahedron_case(polytope, dir);
        default: return 0;
    }
}

int gjk(convex_hull_mesh_t* polytope, shape_t* shape1, shape_t* shape2) {
    if (!shape1) return 0;
    if (!shape2) return 0;
    if (shape1->type == SHAPE_NONE) return 0;
    if (shape2->type == SHAPE_NONE) return 0;

    // random initial direction
    vec3_t dir = vec3_from_floats(1.0f, 0.0f, 0.0f);
    vec3_t support = SUPPORT(shape1, shape2, dir);
    polytope->vertices[0] = support;
    polytope->n_vertices = 1;

    // next point is towards the origin
    dir = vec3_normalize(vec3_neg(support));

    int attempts = 16;
    while (attempts--) {
        support = SUPPORT(shape1, shape2, dir);

        if (vec3_dot(support, dir) <= 0.0f) {
            return 0;
        }

        polytope->vertices[3] = polytope->vertices[2];
        polytope->vertices[2] = polytope->vertices[1];
        polytope->vertices[1] = polytope->vertices[0];
        polytope->vertices[0] = support;
        polytope->n_vertices++;
        assert(polytope->n_vertices <= CONVEX_HULL_LIMIT);

        if (update_simplex(polytope, &dir)) {
            return 1;
        }
    }

    return 0;
}

vec3_t epa(convex_hull_mesh_t* polytope, shape_t* shape1, shape_t* shape2) {
    // take the simplex geturned from gjk as the start polytope
    assert(polytope->n_vertices == 4);

    polytope->faces[0] = (face_t){ 0, 1, 2, {0}, INT32_MAX };
    polytope->faces[1] = (face_t){ 0, 3, 1, {0}, INT32_MAX };
    polytope->faces[2] = (face_t){ 0, 2, 3, {0}, INT32_MAX };
    polytope->faces[3] = (face_t){ 1, 3, 2, {0}, INT32_MAX };
    polytope->n_faces = 4;

    vec3_t interior_point = {0};
    size_t closest_face_index = get_face_normals(polytope, 0, interior_point);

    for (size_t attempt = 1; attempt <= MAX_EPA_TRIES; ++attempt) {
        const face_t closest_face = polytope->faces[closest_face_index];
        const vec3_t support = SUPPORT(shape1, shape2, closest_face.normal);
        const scalar_t distance_support_to_origin = vec3_dot(closest_face.normal, support);


        dump_current_simplex(attempt, polytope, closest_face_index);

        if ((distance_support_to_origin - closest_face.distance) < SCALAR(0.1f)) {
            goto eh_close_enough;
        }

        convex_hull_expand(polytope, support);

        closest_face_index = get_face_normals(polytope, 0, interior_point);
    }

    // dump_current_simplex(MAX_EPA_TRIES, polytope);
    // the closest face seems to lie close enough to the minkowski difference's boundaries
    // we can assume this will give us the real penetration direction and distance, so let's return that
    eh_close_enough:

    if (polytope->faces[closest_face_index].distance > SCALAR(100)) {
        printf("seems sus.\n");
    }
    if (polytope->faces[closest_face_index].distance < SCALAR(0)) {
        printf("seems turbo <0 sus.\n");
    }

    return vec3_muls(polytope->faces[closest_face_index].normal, polytope->faces[closest_face_index].distance + SCALAR(0.01f));
}

void move_sphere(sphere_t* shape, vec3_t move_by) {
    shape->center = vec3_add(shape->center, move_by);
}

void move_capsule(capsule_t* shape, vec3_t move_by) {
    shape->a = vec3_add(shape->a, move_by);
    shape->b = vec3_add(shape->b, move_by);
}

void move_triangle(triangle_t* shape, vec3_t move_by) {
    shape->v0 = vec3_add(shape->v0, move_by);
    shape->v1 = vec3_add(shape->v1, move_by);
    shape->v2 = vec3_add(shape->v2, move_by);
}

void move_aabb(aabb_t* shape, vec3_t move_by) {
    shape->min = vec3_add(shape->min, move_by);
    shape->max = vec3_add(shape->max, move_by);
}

void move_convex_hull(convex_hull_t* shape, vec3_t move_by) {
    for (size_t i = 0; i < shape->n_points; ++i) {
        shape->points[i] = vec3_add(shape->points[i], move_by);
    }
}

void move_shape(shape_t* shape, vec3_t move_by) {
    switch (shape->type) {
        case SHAPE_SPHERE: move_sphere(&shape->sphere, move_by); break;
        case SHAPE_CAPSULE: move_capsule(&shape->capsule, move_by); break;
        case SHAPE_TRIANGLE: move_triangle(&shape->triangle, move_by); break;
        case SHAPE_AABB: move_aabb(&shape->aabb, move_by); break;
        case SHAPE_CONVEX_HULL: move_convex_hull(&shape->convex_hull, move_by); break;
        default: break;
    }
}
