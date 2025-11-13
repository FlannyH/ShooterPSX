#include "collision.h"
#include "file.h"
#include "renderer.h"
#include "texture.h"
#include <string.h>

level_collision_t bvh_from_file(const char* path, int on_stack, stack_t stack) {
    // Load file
    uint32_t* data = NULL;
    size_t size;
    file_read(path, &data, &size, on_stack, stack);

    // Find data and return to user
    const collision_mesh_header_t* header = (collision_mesh_header_t*)data;
    const intptr_t binary = (intptr_t)(header + 1);

    // Verify file magic
    if (!data || header->file_magic != MAGIC_FCOL) {
        printf("[ERROR] Error loading collision mesh '%s'\n", path);
        return (level_collision_t){0};
    }

    return (level_collision_t) {
        .primitives = (collision_triangle_3d_t*)(binary + header->triangle_data_offset),
        .indices = (uint16_t*)(binary + header->bvh_indices_offset),
        .nodes = (bvh_node_t*)(binary + header->bvh_nodes_offset),
        .nav_graph_nodes = (nav_node_t*)(binary + header->nav_graph_offset + 2),
        .n_primitives = header->n_nodes,
        .n_nav_graph_nodes = *(uint16_t*)(binary + header->nav_graph_offset)
    };
}

/// ------POINT COLLISION------
int point_aabb_intersect(const aabb_t* aabb, vec3_t point) {
#ifdef _DEBUG
    if (!aabb) return 0;
#endif

    return (point.x >= aabb->min.x)
    &&     (point.y >= aabb->min.y)
    &&     (point.z >= aabb->min.z)
    &&     (point.x <= aabb->max.x)
    &&     (point.y <= aabb->max.y)
    &&     (point.z <= aabb->max.z);
}

/// -------RAY COLLISION-------
// todo: verify this works as intended
int ray_aabb_intersect(const aabb_t* aabb, ray_t ray) {
#ifdef _DEBUG
    if (!aabb) return 0;
#endif

    const scalar_t tx1 = scalar_mul(aabb->min.x - ray.position.x, ray.inv_direction.x);
    const scalar_t tx2 = scalar_mul(aabb->max.x - ray.position.x, ray.inv_direction.x);

    scalar_t tmin = scalar_min(tx1, tx2);
    scalar_t tmax = scalar_max(tx1, tx2);

    const scalar_t ty1 = scalar_mul(aabb->min.y - ray.position.y, ray.inv_direction.y);
    const scalar_t ty2 = scalar_mul(aabb->max.y - ray.position.y, ray.inv_direction.y);

    tmin = scalar_max(scalar_min(ty1, ty2), tmin);
    tmax = scalar_min(scalar_max(ty1, ty2), tmax);

    const scalar_t tz1 = scalar_mul(aabb->min.z - ray.position.z, ray.inv_direction.z);
    const scalar_t tz2 = scalar_mul(aabb->max.z - ray.position.z, ray.inv_direction.z);

    tmin = scalar_max(scalar_min(tz1, tz2), tmin);
    tmax = scalar_min(scalar_max(tz1, tz2), tmax);
    
    return tmax >= tmin && tmax >= 0;
}

// todo: verify this works as intended
int ray_aabb_intersect_fancy(const aabb_t* aabb, ray_t ray, rayhit_t* hit) {
#ifdef _DEBUG
    if (!aabb) return 0;
    if (!hit) return 0;
#endif

    // If the ray starts inside the box, always intersect
    if (point_aabb_intersect(aabb, ray.position)) {
        hit->distance = 0;
        hit->position = ray.position;
        return 1;
    }

    // Otherwise, follow the other algorithm
    const scalar_t tx1 = scalar_mul(aabb->min.x - ray.position.x, ray.inv_direction.x);
    const scalar_t tx2 = scalar_mul(aabb->max.x - ray.position.x, ray.inv_direction.x);

    scalar_t tmin = scalar_min(tx1, tx2);
    scalar_t tmax = scalar_max(tx1, tx2);

    const scalar_t ty1 = scalar_mul(aabb->min.y - ray.position.y, ray.inv_direction.y);
    const scalar_t ty2 = scalar_mul(aabb->max.y - ray.position.y, ray.inv_direction.y);

    tmin = scalar_max(scalar_min(ty1, ty2), tmin);
    tmax = scalar_min(scalar_max(ty1, ty2), tmax);

    const scalar_t tz1 = scalar_mul(aabb->min.z - ray.position.z, ray.inv_direction.z);
    const scalar_t tz2 = scalar_mul(aabb->max.z - ray.position.z, ray.inv_direction.z);

    tmin = scalar_max(scalar_min(tz1, tz2), tmin);
    tmax = scalar_min(scalar_max(tz1, tz2), tmax);
    
    // And store the result in the rayhit
    if (tmax >= tmin && tmax >= 0) {
        hit->distance = tmin;
        hit->position = vec3_add(ray.position, vec3_muls(ray.direction, tmin));
        return 1;
    };

    return 0;
}

// todo: verify this works as intended
int ray_triangle_intersect(collision_triangle_3d_t* triangle, ray_t ray, rayhit_t* hit) {
#ifdef _DEBUG
    if (!triangle) return 0;
    if (!hit) return 0;
#endif

#define SHIFT_COUNT 5
    const vec3_t vtx0 = vec3_shift_right(triangle->v0, SHIFT_COUNT);
    const vec3_t vtx1 = vec3_shift_right(triangle->v1, SHIFT_COUNT);
    const vec3_t vtx2 = vec3_shift_right(triangle->v2, SHIFT_COUNT);
    const vec3_t ray_pos = vec3_shift_right(ray.position, SHIFT_COUNT);

    const vec3_t edge1 = vec3_sub(vtx1, vtx0);
    const vec3_t edge2 = vec3_sub(vtx2, vtx0);
    const vec3_t h = vec3_cross(ray.direction, edge2);
    const scalar_t det = vec3_dot(edge1, h);

    if (det == 0) {
        hit->distance = INT32_MAX;
        return 0;
    }

    const scalar_t inv_det = scalar_div(ONE, det);
    const vec3_t v0_ray = vec3_sub(ray_pos, vtx0);
    const scalar_t u = scalar_mul(inv_det, vec3_dot(v0_ray, h));

    if (u < 0 || u > ONE) {
        hit->distance = INT32_MAX;
        return 0;
    }

    const vec3_t q = vec3_cross(v0_ray, edge1);
    const scalar_t v = scalar_mul(inv_det, vec3_dot(ray.direction, q));

    if (v < 0 || u + v > ONE) {
        hit->distance = INT32_MAX;
        return 0;
    }

    const scalar_t t = scalar_mul(inv_det, vec3_dot(edge2, q));

    if (t > 0) {
        hit->position = vec3_add(ray.position, vec3_muls(ray.direction, t << SHIFT_COUNT));
        hit->distance = t << SHIFT_COUNT;
        hit->normal = triangle->normal;
        hit->type = RAY_HIT_TYPE_TRIANGLE;
        hit->tri.triangle = triangle;
        renderer_debug_draw_line(hit->tri.triangle->v0, hit->tri.triangle->v1, pink, &id_transform);
        renderer_debug_draw_line(hit->tri.triangle->v1, hit->tri.triangle->v2, pink, &id_transform);
        renderer_debug_draw_line(hit->tri.triangle->v2, hit->tri.triangle->v0, pink, &id_transform);
        return 1;
    }
    return 0;
#undef SHIFT_COUNT
}

void handle_node_intersection_ray(level_collision_t* self, const bvh_node_t* current_node, const ray_t ray, rayhit_t* hit, const int rec_depth) {
    // Intersect current node
    if (ray_aabb_intersect(&current_node->bounds, ray)) {
        // If it's a leaf
        if (current_node->primitive_count != 0) {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++) {
                // If hit
                if (ray_triangle_intersect(&self->primitives[self->indices[i]], ray, &sub_hit)) {
                    // If lowest distance
                    if (sub_hit.distance < hit->distance && sub_hit.distance >= 0) {
                        // Copy the hit info into the output hit for the BVH traversal
                        memcpy(hit, &sub_hit, sizeof(rayhit_t));
                        hit->type = RAY_HIT_TYPE_TRIANGLE;
                        hit->tri.triangle = &self->primitives[self->indices[i]];
                    }
                }
            }
            return;
        }

        //Otherwise, intersect child nodes
        handle_node_intersection_ray(self, &self->nodes[current_node->left_first + 0], ray, hit, rec_depth + 1);
        handle_node_intersection_ray(self, &self->nodes[current_node->left_first + 1], ray, hit, rec_depth + 1);
    }
}

void bvh_intersect_ray(level_collision_t* self, ray_t ray, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    if (self == NULL) return;
    if (self->root == NULL) return;
    handle_node_intersection_ray(self, self->root, ray, hit, 0);
}
