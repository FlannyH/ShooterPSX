#include "memory.h"
#include "mesh.h"
#include "file.h"
#include "common.h"
#include "renderer.h"
#include "structs.h"
#include "texture.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

collision_mesh_t* model_load_collision(const char* path, int on_stack, stack_t stack) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    if (!file_read(path, &file_data, &size, on_stack, stack)) {
        return 0;
    }

    // Read collision header
    const collision_mesh_header_t* col_mesh = (collision_mesh_header_t*)file_data;

    // Verify file magic
    if (col_mesh->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
        return 0;
    }

    // Return the collision model
	collision_mesh_t* output;
	if (on_stack) output = mem_stack_alloc(sizeof(collision_mesh_t), stack);
	else output = mem_alloc(sizeof(collision_mesh_t), MEM_CAT_COLLISION);
    output->n_verts = col_mesh->n_verts;
    output->verts = (col_mesh_file_vert_t*)(col_mesh + 1);
    return output;
}

aabb_t triangle_get_bounds(const triangle_3d_t* self) {
    PANIC_IF("Trying to get triangle bounds from nullptr!", self == NULL);

    return (aabb_t) {
        .min = vec3_min(
            vec3_min(
                vec3_from_int32s(self->v0.x, self->v0.y, self->v0.z),
                vec3_from_int32s(self->v1.x, self->v1.y, self->v1.z)
            ),
            vec3_from_int32s(self->v2.x, self->v2.y, self->v2.z)
        ),
        .max = vec3_max(
            vec3_max(
                vec3_from_int32s(self->v0.x, self->v0.y, self->v0.z),
                vec3_from_int32s(self->v1.x, self->v1.y, self->v1.z)
            ),
            vec3_from_int32s(self->v2.x, self->v2.y, self->v2.z)
        )
    };
}

aabb_t collision_triangle_get_bounds(const collision_triangle_3d_t* self) {
    PANIC_IF("Trying to get triangle bounds from nullptr!", self == NULL);

    return (aabb_t){
        .min = vec3_min(vec3_min(self->v0, self->v1), self->v2),
        .max = vec3_max(vec3_max(self->v0, self->v1), self->v2)
    };
}

int strings_are_equal(const char* a, const char* b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) return 0;
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0');
}

// todo(mesh_find_hash_opt): desc: calculate and compare hashes instead of string compare
mesh_t* model_find_mesh(const model_t* model, const char* mesh_name) {
    for (size_t i = 0; i < model->n_meshes; ++i) {
        mesh_t* mesh = &model->meshes[i];
        if (strings_are_equal(mesh->name, mesh_name)) {
            return mesh;
        }
    }
    printf("[ERROR] Could not find mesh with name '%s'!\n", mesh_name);
    return 0;
}

void remove_edge_at_index(edge_t* edges, size_t* n_edges, size_t index) {
    if (index >= (*n_edges)) return;
    edge_t* dst = &edges[index];
    edge_t* src = &edges[index + 1];
    size_t count = (*n_edges) - index - 1;
    memmove(dst, src, count * sizeof(edge_t));
    (*n_edges) -= 1;
}

void add_unique_edge(edge_t* edges, size_t* n_edges, edge_t new_edge) {
    size_t i = 0;

    while (i < *n_edges) {
        const edge_t existing_edge = edges[i];
        const int is_reverse = (new_edge.a == existing_edge.b) && (new_edge.b == existing_edge.a);
        if (is_reverse) {
            break;
        }
        ++i;
    }
    if (i != *n_edges) {
        remove_edge_at_index(edges, n_edges, i);
    }
    else {
        edges[*n_edges] = new_edge;
        (*n_edges) += 1;
    }
}

size_t get_face_normals(convex_hull_mesh_t* polytope, size_t start_index, vec3_t center) {
    size_t min_face = 0;
    scalar_t min_distance = INT32_MAX;

    for (size_t i = start_index; i < polytope->n_faces; ++i) {
        const vec3_t a = polytope->vertices[polytope->faces[i].a];
        const vec3_t b = polytope->vertices[polytope->faces[i].b];
        const vec3_t c = polytope->vertices[polytope->faces[i].c];

        const vec3_t ab = vec3_normalize(vec3_sub(b, a));
        const vec3_t ac = vec3_sub(c, a);

        const vec3_t center_to_a = vec3_sub(a, center);

        vec3_t normal = vec3_normalize(vec3_cross_lh(ab, ac));
        scalar_t distance = vec3_dot(normal, center_to_a);
        if (normal.x == 0 && normal.z == 0 && normal.y == 0) {
            normal = vec3_normalize(vec3_cross_lh(ab, ac));
        }

        if (distance < 0) {
            normal = vec3_neg(normal);
            distance = -distance;
            size_t old_b = polytope->faces[i].b;
            polytope->faces[i].b = polytope->faces[i].c;
            polytope->faces[i].c = old_b;
        }

        polytope->faces[i].normal = normal;
        polytope->faces[i].distance = distance;

        if (distance < min_distance) {
            min_face = i;
            min_distance = distance;
        }
    }

    return min_face;
}

void convex_hull_expand(convex_hull_mesh_t* shape, vec3_t new_point) {
    size_t n_faces_added = 0;
    face_t faces_added[CONVEX_HULL_LIMIT] = {0};

    // commit new point
    const size_t new_point_i = shape->n_vertices++;
    assert(shape->n_vertices <= CONVEX_HULL_LIMIT);
    shape->vertices[new_point_i] = new_point;

    size_t n_edges = 0;
    edge_t edges[CONVEX_HULL_LIMIT] = {0};

    // iterate over the old faces only, lest we remove the faces we just added
    for (int fi = 0; fi < shape->n_faces; ++fi) {
        vec3_t face_normal = shape->faces[fi].normal;
        vec3_t point_on_face = shape->vertices[shape->faces[fi].a];

        // if the new point can see this face
        if (vec3_dot(face_normal, vec3_sub(new_point, point_on_face)) > 0) {
            // find which edges are on the boundary of that gap, by counting edges that
            // only show up once
            add_unique_edge(edges, &n_edges, (edge_t){shape->faces[fi].a, shape->faces[fi].b});
            add_unique_edge(edges, &n_edges, (edge_t){shape->faces[fi].b, shape->faces[fi].c});
            add_unique_edge(edges, &n_edges, (edge_t){shape->faces[fi].c, shape->faces[fi].a});

            // remove the face from the convex hull, leaving a gap
            shape->faces[fi] = shape->faces[shape->n_faces - 1];
            fi--; shape->n_faces--;
        }
    }

    // then close the gap by extruding the edges to the new point
    for (size_t i = 0; i < n_edges; ++i) {
        faces_added[n_faces_added++] = (face_t){ edges[i].a, edges[i].b, new_point_i };
        assert(n_faces_added <= CONVEX_HULL_LIMIT);
    }

    // commit the new faces to the shape
    for (size_t i = 0; i < n_faces_added; ++i) {
        shape->faces[shape->n_faces++] = faces_added[i];
        assert(shape->n_faces <= CONVEX_HULL_LIMIT);
    }
}
