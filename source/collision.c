#include "collision.h"
#include "level.h"
#include "math/fixed_point.h"
#include "structs.h"
#include "math/vec3.h"

#include <string.h>
#define MAX_EPA_TRIES 32

// directions are normalized to make it more predictable with fixed point precision

typedef struct {
    size_t a, b, c;
    vec3_t normal;
} face_t;

typedef struct {
    size_t a, b;
} edge_t;

typedef struct {
    // gjk
    vec3_t a, b, c, d;
    vec3_t dir;
    scalar_t next_vtx_count;

    // epa
    vec3_t vertices[32];
    size_t n_vertices;
    face_t faces[32];
    size_t n_faces;
    edge_t edges[32];
    size_t n_edges;
} gjk_state_t;

gjk_state_t s = {0};

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

vec3_t support_shape(shape_t* shape, vec3_t direction) {
    switch (shape->type) {
        case SHAPE_NONE:     return (vec3_t){0};
        case SHAPE_SPHERE:   return support_sphere(shape->sphere, direction);
        case SHAPE_CAPSULE:  return support_capsule(shape->capsule, direction);
        case SHAPE_AABB:     return support_aabb(shape->aabb, direction);
        case SHAPE_TRIANGLE: return support_triangle(shape->triangle, direction);
        default:             return (vec3_t){0};
    }
}

#define SIMILAR_DIR(a, b) (vec3_dot((a), (b)) > 0)

int handle_triangle_case(void) {
    printf("triangle case\n");

    const vec3_t ab = vec3_normalize(vec3_sub(s.b, s.a));
    const vec3_t ac = vec3_normalize(vec3_sub(s.c, s.a));
    const vec3_t ao = vec3_normalize(vec3_neg(s.a));

    const vec3_t nrm = vec3_cross(ab, ac);

    if (SIMILAR_DIR(vec3_cross(ab, nrm), ao)) { // origin outside near edge AB
        s.next_vtx_count = 3;
        s.c = s.b;
        s.b = s.a;
        s.dir = vec3_cross(vec3_cross(ab, ao), ab);
        return 0;
    }
    if (SIMILAR_DIR(vec3_cross(nrm, ac), ao)) { // origin outside near edge AC
        s.next_vtx_count = 3;
        s.c = s.c;
        s.b = s.a;
        s.dir = vec3_cross(vec3_cross(ac, ao), ac);
        return 0;
    }
    if (SIMILAR_DIR(nrm, ao)) { // origin inside above triangle
        s.next_vtx_count = 4;
        s.d = s.c;
        s.c = s.b;
        s.b = s.a;
        s.dir = nrm;
    }
    else { // origin inside below or on triangle
        s.next_vtx_count = 4;
        s.d = s.b;
        s.c = s.c;
        s.b = s.a;
        s.dir = vec3_neg(nrm);
    }

    return 0;
}

int handle_tetrahedron_case(void) {
    printf("tetrahedron case\n");

    const vec3_t ab = vec3_normalize(vec3_sub(s.b, s.a));
    const vec3_t ac = vec3_normalize(vec3_sub(s.c, s.a));
    const vec3_t ad = vec3_normalize(vec3_sub(s.d, s.a));
    const vec3_t ao = vec3_normalize(vec3_neg(s.a));

    const vec3_t abc = vec3_cross(ab, ac);
    const vec3_t acd = vec3_cross(ac, ad);
    const vec3_t adb = vec3_cross(ad, ab);

    if (SIMILAR_DIR(abc, ao)) {
        return handle_triangle_case();
    }
    if (SIMILAR_DIR(acd, ao)) {
        s.b = s.c;
        s.c = s.d;
        return handle_triangle_case();
    }
    if (SIMILAR_DIR(adb, ao)) {
        s.c = s.b;
        s.b = s.d;
        return handle_triangle_case();
    }
    return 1;
}

// returns if simplex surrounds origin, and updates the simplex to be ready for the next point
int check_origin_and_update_simplex(void) {
    switch (s.next_vtx_count) {
        case 3: return handle_triangle_case();
        case 4: return handle_tetrahedron_case();
        default: return 0;
    }
}

int gjk(shape_t* shape1, shape_t* shape2) {
    if (shape1->type == SHAPE_NONE) return 0;
    if (shape2->type == SHAPE_NONE) return 0;
    memset(&s, 0, sizeof(gjk_state_t));

    // random initial direction
    s.dir = vec3_from_floats(1.0f, 0.0f, 0.0f);
    s.c = vec3_sub(support_shape(shape1, s.dir), support_shape(shape2, vec3_neg(s.dir)));

    // next point is towards the origin
    s.dir = vec3_normalize(vec3_neg(s.c));
    s.b = vec3_sub(support_shape(shape1, s.dir), support_shape(shape2, vec3_neg(s.dir)));

    // didn't pass the origin -> no collision
    if (vec3_dot(s.b, s.dir) <= 0.0f) return 0;

    // find direction to look for the 3rd point
    const vec3_t bc = vec3_normalize(vec3_sub(s.c, s.b));
    const vec3_t bo = vec3_normalize(vec3_neg(s.b));
    s.dir = vec3_cross(vec3_cross(bc, bo), bc);
    s.next_vtx_count = 3;

    for (int attempts = 0; attempts < 64; ++attempts) {
        s.a = vec3_sub(support_shape(shape1, s.dir), support_shape(shape2, vec3_neg(s.dir)));
        if (vec3_dot(s.a, s.dir) <= 0.0f) return 0; // didn't pass the origin -> no collision
        if (check_origin_and_update_simplex()) return 1; // if our simplex surrounds origin -> yes collision
    }
    return 0;
}

void remove_edge_at_index(size_t index) {
    edge_t* dst = &s.edges[index];
    edge_t* src = &s.edges[index + 1];
    size_t count = (--s.n_edges) - index;
    memmove(dst, src, count * sizeof(edge_t));
}

void remove_face_at_index(size_t index) {
    face_t* dst = &s.faces[index];
    face_t* src = &s.faces[index + 1];
    size_t count = (--s.n_faces) - index;
    memmove(dst,src, count * sizeof(face_t));
}

void add_unique_edge(edge_t edge) {
    size_t i = 0;

    while (i < s.n_edges) {
        const edge_t existing_edge = s.edges[i];
        const int is_reverse = (edge.a == existing_edge.b) && (edge.b == existing_edge.a);
        if (is_reverse) {
            break;
        }
        ++i;
    }
    if (i != s.n_edges) {
        remove_edge_at_index(i);
    }
    else {
        s.edges[s.n_edges++] = edge;
    }
}

int find_in_edges(size_t* index, const edge_t edge) {
    for (size_t i = 0; i < s.n_edges; ++i) {
        if (s.edges[i].a == edge.a
         && s.edges[i].b == edge.b) {
             *index = i;
             return 1;
         }
    }
    return 0;
}

vec3_t calculate_normal(size_t face_id) {
    const vec3_t ab = vec3_normalize(vec3_sub(s.vertices[s.faces[face_id].b], s.vertices[s.faces[face_id].a]));
    const vec3_t ac = vec3_normalize(vec3_sub(s.vertices[s.faces[face_id].c], s.vertices[s.faces[face_id].a]));
    return vec3_normalize(vec3_cross(ab, ac));
}

void epa_unique_edge(edge_t edge) {
    size_t index = 0;
    if (find_in_edges(&index, (edge_t){edge.b, edge.a})) {
        remove_edge_at_index(index);
    }
    else {
        s.edges[s.n_edges++] = edge;
    }
}

vec3_t epa(shape_t* shape1, shape_t* shape2) {
    s.vertices[0] = s.a;
    s.vertices[1] = s.b;
    s.vertices[2] = s.c;
    s.vertices[3] = s.d;
    s.n_vertices = 4;

    s.faces[0] = (face_t){ 0, 1, 2, {0} };
    s.faces[1] = (face_t){ 0, 3, 1, {0} };
    s.faces[2] = (face_t){ 0, 2, 3, {0} };
    s.faces[3] = (face_t){ 1, 3, 2, {0} };
    s.n_faces = 4;

    for (size_t i = 0; i < s.n_faces; ++i) {
        s.faces[i].normal = calculate_normal(i);
    }

    scalar_t min_distance = INT32_MAX;
    vec3_t min_normal = {0};

    while (min_distance == INT32_MAX) {
        // find closest face to the origin
        for (size_t i_face = 0; i_face < s.n_faces; ++i_face) {
            const vec3_t nrm = s.faces[i_face].normal;
            const vec3_t a = s.vertices[s.faces[i_face].a];
            scalar_t distance = vec3_dot(nrm, a);
            if (distance < min_distance) {
                min_distance = distance;
                min_normal = nrm;
            }
        }

        // if the support point tells us there's more minkowski diff in that direction -> extendo
        const vec3_t support = vec3_sub(support_shape(shape1, min_normal), support_shape(shape2, vec3_neg(min_normal)));
        const scalar_t distance = vec3_dot(min_normal, support);
        if (scalar_abs(distance - min_distance) > SCALAR(1.0f)) {
            min_distance = INT32_MAX;

            // remove all faces in the same direction and rebuild polytope
            s.n_edges = 0;
            size_t i_face = 0;
            while (i_face < s.n_faces) {
                if (vec3_dot(s.faces[i_face].normal, vec3_sub(support, s.vertices[s.faces[i_face].a])) > 0) {
                    epa_unique_edge((edge_t){s.faces[i_face].a, s.faces[i_face].b});
                    epa_unique_edge((edge_t){s.faces[i_face].b, s.faces[i_face].c});
                    epa_unique_edge((edge_t){s.faces[i_face].c, s.faces[i_face].a});
                    remove_face_at_index(i_face);
                    continue;
                }
                ++i_face;
            }

            const size_t new_vtx_i = s.n_vertices++;
            s.vertices[new_vtx_i] = support;

            for (size_t edge_i = 0; edge_i < s.n_edges; ++edge_i) {
                const size_t new_face_i = s.n_faces++;
                s.faces[new_face_i].a = s.edges[edge_i].a;
                s.faces[new_face_i].b = s.edges[edge_i].b;
                s.faces[new_face_i].c = new_vtx_i;
                s.faces[new_face_i].normal = calculate_normal(new_face_i);
            }
        }
    }

    return vec3_muls(min_normal, min_distance + SCALAR(0.003f));
}
