#include "collision.h"
#include "file.h"
#include "renderer.h"
#include "texture.h"
#include "math/scalar.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include <assert.h>
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
    assert(aabb);

    return (point.x >= aabb->min.x)
    &&     (point.y >= aabb->min.y)
    &&     (point.z >= aabb->min.z)
    &&     (point.x <= aabb->max.x)
    &&     (point.y <= aabb->max.y)
    &&     (point.z <= aabb->max.z);
}

/// -------RAY COLLISION-------
int ray_aabb_intersect(const aabb_t* aabb, ray_t ray) {
    assert(aabb);

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

int ray_aabb_intersect_fancy(const aabb_t* aabb, ray_t ray, rayhit_t* hit) {
    assert(aabb);
    assert(hit);

    // If the ray starts inside the box, always intersect
    if (point_aabb_intersect(aabb, ray.position)) {
        hit->distance = 0;
        hit->position = ray.position;
        return 1;
    }

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

int ray_triangle_intersect(collision_triangle_3d_t* triangle, ray_t ray, rayhit_t* hit) {
    assert(triangle);
    assert(hit);

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

/// ----CAPSULE COLLISION----
// Approximation!
int vertical_capsule_aabb_intersect(const aabb_t* aabb, const vertical_capsule_t vertical_capsule) {
    assert(aabb);

    // Convert vertical capsule into AABB
    const aabb_t cyl_as_aabb = {
        .min = {
            .x = vertical_capsule.bottom.x - vertical_capsule.radius,
            .y = vertical_capsule.bottom.y - vertical_capsule.radius,
            .z = vertical_capsule.bottom.z - vertical_capsule.radius,
        },
        .max = {
            .x = vertical_capsule.bottom.x + vertical_capsule.radius,
            .y = vertical_capsule.bottom.y + vertical_capsule.height + vertical_capsule.radius,
            .z = vertical_capsule.bottom.z + vertical_capsule.radius,
        },
    };

    // AABB - AABB test
    return (aabb->max.x > cyl_as_aabb.min.x)
    &&     (aabb->min.x < cyl_as_aabb.max.x)
    &&     (aabb->max.y > cyl_as_aabb.min.y)
    &&     (aabb->min.y < cyl_as_aabb.max.y)
    &&     (aabb->max.z > cyl_as_aabb.min.z)
    &&     (aabb->min.z < cyl_as_aabb.max.z);
}

int vertical_capsule_aabb_intersect_fancy(const aabb_t* aabb, const vertical_capsule_t vertical_capsule, rayhit_t* hit) {
    assert(aabb);
    assert(hit);

    // Find Y range overlap between capsule line segment and AABB
    const scalar_t radius = vertical_capsule.radius;
    const scalar_t radius2 = scalar_mul(radius, radius);
    const scalar_t cap_seg_bottom = vertical_capsule.bottom.y;
    const scalar_t cap_seg_top = cap_seg_bottom + vertical_capsule.height;
    const scalar_t cap_seg_center = cap_seg_bottom + (vertical_capsule.height / 2);
    const scalar_t y_min = scalar_max(aabb->min.y, cap_seg_bottom);
    const scalar_t y_max = scalar_min(aabb->max.y, cap_seg_top);

    const int y_overlap = (y_min <= y_max);

    // If overlap, just check distance in 2D
    if (y_overlap) {
        const vec2_t cap_pos_2d = (vec2_t){
            vertical_capsule.bottom.x,
            vertical_capsule.bottom.z
        };
        const vec2_t aabb_closest_2d = (vec2_t){
            .x = scalar_clamp(cap_pos_2d.x, aabb->min.x, aabb->max.x),
            .y = scalar_clamp(cap_pos_2d.y, aabb->min.z, aabb->max.z),
        };
        const vec2_t aabb_cap = vec2_sub(cap_pos_2d, aabb_closest_2d);
        const scalar_t distance2 = vec2_magnitude_squared(aabb_cap);

        if (distance2 > radius2) return 0;

        hit->position = (vec3_t){
            aabb_closest_2d.x,
            scalar_clamp(cap_seg_center, aabb->min.y, aabb->max.y),
            aabb_closest_2d.y
        };
        hit->normal = vec3_normalize((vec3_t){
            cap_pos_2d.x - aabb_closest_2d.x,
            0,
            cap_pos_2d.y - aabb_closest_2d.y,
        });
        hit->distance = scalar_sqrt(distance2) - radius;
        return 1;
    }

    // Otherwise, find the closest Y-position on the capsule segment to the AABB
    const scalar_t y_closest = (cap_seg_top < aabb->min.y) ? cap_seg_top : cap_seg_bottom;

    // Find closest position on capsule
    const vec3_t cap_closest = (vec3_t){
        .x = vertical_capsule.bottom.x,
        .y = y_closest,
        .z = vertical_capsule.bottom.z,
    };

    // Find closest position on AABB by clamping the capsule point to its bounds
    const vec3_t aabb_closest = (vec3_t){
        .x = scalar_clamp(cap_closest.x, aabb->min.x, aabb->max.x),
        .y = scalar_clamp(cap_closest.y, aabb->min.y, aabb->max.y),
        .z = scalar_clamp(cap_closest.z, aabb->min.z, aabb->max.z),
    };

    // Calculate distance - if distance is outside capsule radius, there's no hit
    const scalar_t distance2 = vec3_magnitude_squared(vec3_sub(aabb_closest, cap_closest));
    if (distance2 > radius2) return 0;

    // Hit! Let's populate the hit info and return
    hit->distance = scalar_sqrt(distance2) - radius;
    hit->position = aabb_closest;
    hit->normal = vec3_normalize(vec3_sub(cap_closest, aabb_closest));
    return 1;
}

int vertical_capsule_triangle_intersect(collision_triangle_3d_t* triangle, vertical_capsule_t capsule, rayhit_t* hit) {
    assert(triangle);
    assert(hit);

    const scalar_t radius2 = scalar_mul(capsule.radius, capsule.radius);

    // Edge case: if the triangle isn't facing up or down at all, top down doesn't make sense
    if (abs(triangle->normal.y) < 32) {
        // Project capsule segment points PQ onto triangle ABC plane -> P' and Q'
        const vec3_t bottom_to_v0 = vec3_sub(triangle->v0, capsule.bottom);
        const scalar_t distance_to_plane = vec3_dot(bottom_to_v0, triangle->normal);
        const vec3_t p_proj = vec3_add(capsule.bottom, vec3_muls(triangle->normal, distance_to_plane));
        const vec3_t q_proj = {p_proj.x, p_proj.y + capsule.height, p_proj.z}; // cheeky optimization, not entirely accurate but good enough

        // Find points Ptri and Qtri on triangle ABC closest to P' and Q'
        const vec3_t v0_p = vec3_sub(p_proj, triangle->v0);
        const vec3_t v1_p = vec3_sub(p_proj, triangle->v1);
        const vec3_t v2_p = vec3_sub(p_proj, triangle->v2);
        const vec3_t v0_q = vec3_sub(q_proj, triangle->v0);
        const vec3_t v1_q = vec3_sub(q_proj, triangle->v1);
        const vec3_t v2_q = vec3_sub(q_proj, triangle->v2);
        const vec3_t v0_v1 = vec3_sub(triangle->v1, triangle->v0);
        const vec3_t v1_v2 = vec3_sub(triangle->v2, triangle->v1);
        const vec3_t v2_v0 = vec3_sub(triangle->v0, triangle->v2);
        const scalar_t p_edge0 = vec3_dot(vec3_cross(v1_p, v1_v2), triangle->normal);
        const scalar_t p_edge1 = vec3_dot(vec3_cross(v2_p, v2_v0), triangle->normal);
        const scalar_t p_edge2 = vec3_dot(vec3_cross(v0_p, v0_v1), triangle->normal);
        const scalar_t q_edge0 = vec3_dot(vec3_cross(v1_q, v1_v2), triangle->normal);
        const scalar_t q_edge1 = vec3_dot(vec3_cross(v2_q, v2_v0), triangle->normal);
        const scalar_t q_edge2 = vec3_dot(vec3_cross(v0_q, v0_v1), triangle->normal);

        vec3_t p_triangle = p_proj;
        vec3_t q_triangle = q_proj;

        // If P inside triangle
        if (p_edge0 < 0 && p_edge1 < 0 && p_edge2 < 0) {
            const vec3_t triangle_to_capsule = vec3_sub(capsule.bottom, p_triangle);
            const scalar_t distance2 = vec3_magnitude_squared(triangle_to_capsule);
            if (distance2 >= radius2) return 0;

            hit->distance = scalar_sqrt(distance2) - capsule.radius;
            hit->normal = triangle->normal;
            hit->position = p_triangle;

            return 1;
        }

        // If Q inside triangle
        if (q_edge0 < 0 && q_edge1 < 0 && q_edge2 < 0) {
            const vec3_t triangle_to_capsule = vec3_sub(capsule.bottom, p_triangle);
            const scalar_t distance2 = vec3_magnitude_squared(triangle_to_capsule);
            if (distance2 >= radius2) return 0;

            hit->distance = scalar_sqrt(distance2) - capsule.radius;
            hit->normal = triangle->normal;
            hit->position = q_triangle;

            return 1;
        }

        // Both are outside the triangle, get closest points
        // - p_triangle
        if (p_edge0 > 0) { // closest point is on edge v1_v2
            const scalar_t t = scalar_clamp(
                scalar_div(vec3_dot(v1_v2, v1_p), vec3_dot(v1_v2, v1_v2)),
                0, ONE
            );
            p_triangle = vec3_add(triangle->v1, vec3_muls(v1_v2, t));
        }
        else if (p_edge1 > 0) { // closest point is on edge v2_v0
            const scalar_t t = scalar_clamp(
                scalar_div(vec3_dot(v2_v0, v2_p), vec3_dot(v2_v0, v2_v0)),
                0, ONE
            );
            p_triangle = vec3_add(triangle->v2, vec3_muls(v2_v0, t));
        }
        else if (p_edge2 > 0) { // closest point is on edge v0_v1
            const scalar_t t = scalar_clamp(
                scalar_div(vec3_dot(v0_v1, v0_p), vec3_dot(v0_v1, v0_v1)),
                0, ONE
            );
            p_triangle = vec3_add(triangle->v0, vec3_muls(v0_v1, t));
        }
        // - q_triangle
        if (q_edge0 > 0) { // closest point is on edge v1_v2
            const scalar_t t = scalar_clamp(
                scalar_div(vec3_dot(v1_v2, v1_q), vec3_dot(v1_v2, v1_v2)),
                0, ONE
            );
            q_triangle = vec3_add(triangle->v1, vec3_muls(v1_v2, t));
        }
        else if (q_edge1 > 0) { // closest point is on edge v2_v0
            const scalar_t t = scalar_clamp(
                scalar_div(vec3_dot(v2_v0, v2_q), vec3_dot(v2_v0, v2_v0)),
                0, ONE
            );
            q_triangle = vec3_add(triangle->v2, vec3_muls(v2_v0, t));
        }
        else if (q_edge2 > 0) { // closest point is on edge v0_v1
            const scalar_t t = scalar_clamp(
                scalar_div(vec3_dot(v0_v1, v0_q), vec3_dot(v0_v1, v0_v1)),
                0, ONE
            );
            q_triangle = vec3_add(triangle->v0, vec3_muls(v0_v1, t));
        }

        // At this point we know the 2 points on the triangle closest to the ends of the capsule segment
        // Now we need to pick the points on the segment closest to those 2 triangle points to check the distance
        vec3_t p_clamped = capsule.bottom;
        p_clamped.y = scalar_clamp(p_triangle.y, capsule.bottom.y, capsule.bottom.y + capsule.height);
        vec3_t q_clamped = capsule.bottom;
        q_clamped.y = scalar_clamp(q_triangle.y, capsule.bottom.y, capsule.bottom.y + capsule.height);

        const vec3_t p_dir = vec3_sub(p_clamped, p_triangle);
        const vec3_t q_dir = vec3_sub(q_clamped, q_triangle);

        const scalar_t p_distance2 = vec3_magnitude_squared(p_dir);
        const scalar_t q_distance2 = vec3_magnitude_squared(q_dir);

        const scalar_t min_distance = scalar_min(p_distance2, q_distance2);

        if (min_distance < radius2) {
            if  (p_distance2 < q_distance2) {
                const scalar_t distance = scalar_sqrt(p_distance2);
                hit->normal = vec3_divs(p_dir, distance);
                hit->position = p_triangle;
                hit->distance = distance - capsule.radius;
            }
            else {
                const scalar_t distance = scalar_sqrt(q_distance2);
                hit->normal = vec3_divs(q_dir, distance);
                hit->position = q_triangle;
                hit->distance = distance - capsule.radius;
            }

            return 1;
        }

        return 0;
    }

    // Project to XZ 2D
    const vec2_t p = {capsule.bottom.x, capsule.bottom.z};
    const vec2_t v0 = {triangle->v0.x, triangle->v0.z};
    const vec2_t v1 = {triangle->v1.x, triangle->v1.z};
    const vec2_t v2 = {triangle->v2.x, triangle->v2.z};

    // Find closest point from capsule center to triangle
    // todo: double check winding order
    const vec2_t v0_p = vec2_sub(p, v0);
    const vec2_t v1_p = vec2_sub(p, v1);
    const vec2_t v2_p = vec2_sub(p, v2);
    const vec2_t v0_v1 = vec2_sub(v1, v0);
    const vec2_t v1_v2 = vec2_sub(v2, v1);
    const vec2_t v2_v0 = vec2_sub(v0, v2);
    const scalar_t edge0 = vec2_cross(v1_p, v1_v2);
    const scalar_t edge1 = vec2_cross(v2_p, v2_v0);
    const scalar_t edge2 = vec2_cross(v0_p, v0_v1);

    const vec3_t v0_3d = triangle->v0;
    const vec3_t v1_3d = triangle->v1;
    const vec3_t v2_3d = triangle->v2;

    // note: top down projection maps XYZ -> {X,Z} -> "XY", resulting in the cross 
    //       product sign being flipped relative to the full 3D case.
    
    // 0 negative -> inside triangle
    // 1 negative -> on edge
    // 2 negative -> on vertex, which is on either of the corresponding edges, just pick the first
    // 3 negative -> (impossible)
    
    vec3_t closest_point_triangle;

    if (edge0 >= 0 && edge1 >= 0 && edge2 >= 0) {
        // Find point on segment closest to triangle
        const vec3_t p_3d = capsule.bottom;
        const vec3_t n = triangle->normal;

        closest_point_triangle.x = p_3d.x;
        closest_point_triangle.z = p_3d.z;

        closest_point_triangle.y = v0_3d.y - scalar_div(
            scalar_mul(n.x, (p_3d.x - v0_3d.x)) + scalar_mul(n.z, (p_3d.z - v0_3d.z)),
            n.y
        );
    }

    else if (edge0 < 0) { // closest point is on edge v1_v2
        const scalar_t t = scalar_clamp(
            scalar_div(vec2_dot(v1_v2, v1_p), vec2_dot(v1_v2, v1_v2)),
            0, ONE
        );
        const vec3_t v1_v2_3d = vec3_sub(v2_3d, v1_3d);
        closest_point_triangle = vec3_add(v1_3d, vec3_muls(v1_v2_3d, t));
    }
    else if (edge1 < 0) { // closest point is on edge v2_v0
        const scalar_t t = scalar_clamp(
            scalar_div(vec2_dot(v2_v0, v2_p), vec2_dot(v2_v0, v2_v0)),
            0, ONE
        );
        const vec3_t v2_v0_3d = vec3_sub(v0_3d, v2_3d);
        closest_point_triangle = vec3_add(v2_3d, vec3_muls(v2_v0_3d, t));
    }
    else /*if (edge2 < 0)*/ { // closest point is on edge v0_v1
        const scalar_t t = scalar_clamp(
            scalar_div(vec2_dot(v0_v1, v0_p), vec2_dot(v0_v1, v0_v1)),
            0, ONE
        );
        const vec3_t v0_v1_3d = vec3_sub(v1_3d, v0_3d);
        closest_point_triangle = vec3_add(v0_3d, vec3_muls(v0_v1_3d, t));
    }

    const vec3_t closest_pos_on_line_segment = vec3_from_scalars(
        capsule.bottom.x,
        scalar_clamp(
            closest_point_triangle.y,
            capsule.bottom.y,
            capsule.bottom.y + capsule.height
        ),
        capsule.bottom.z
    );

    const vec3_t dir = vec3_sub(
        closest_pos_on_line_segment,
        closest_point_triangle
    );

    const scalar_t distance2 = vec3_magnitude_squared(dir);

    if (distance2 < radius2) {
        const scalar_t distance = scalar_sqrt(distance2);

        hit->position = closest_point_triangle;
        hit->distance = distance - capsule.radius;

        if (distance2 == 0) {
            hit->normal = triangle->normal;
        }
        else {
            hit->normal = vec3_normalize(vec3_divs(dir, distance));
        }

        return 1;
    }

    return 0;
}

void debug_draw(const level_collision_t* self, const bvh_node_t* node, const int min_depth, const int max_depth, const int curr_depth, const pixel32_t color) {
    const transform_t trans = { {0, 0, 0}, {0, 0, 0}, {ONE, ONE, ONE} };

    if (!self) return;

    // Draw box of this node - only if within the depth bounds
    if (curr_depth > max_depth) {
        return;
    }

    if (curr_depth >= min_depth) {
        renderer_debug_draw_aabb(&node->bounds, color, &trans);
    }

    // If this is a leaf node, stop here
    if (node->primitive_count != 0) {
        return;
    }

    // Draw child nodes
    debug_draw(self, &self->nodes[node->left_first + 0], min_depth, max_depth, curr_depth + 1, color);
    debug_draw(self, &self->nodes[node->left_first + 1], min_depth, max_depth, curr_depth + 1, color);
}

void bvh_debug_draw(const level_collision_t* bvh, const int min_depth, const int max_depth, const pixel32_t color) {
    if (bvh == NULL) return;
    if (bvh->root == NULL) return;
    debug_draw(bvh, bvh->root, min_depth, max_depth, 0, color);
}

void bvh_debug_draw_nav_graph(const level_collision_t* bvh) {
    if (!bvh) return;

    for (size_t i = 0; i < bvh->n_nav_graph_nodes; ++i) {
        svec3_t s_pos1 = bvh->nav_graph_nodes[i].position;
        s_pos1.y += 2;
        const vec3_t pos1 = vec3_from_svec3(s_pos1);

        for (size_t j = 0; j < 4; ++j) {
            const uint16_t id_neighbor = bvh->nav_graph_nodes[i].neighbor_ids[j];

            if (id_neighbor == 0xFFFF) break;

            svec3_t s_pos2 = bvh->nav_graph_nodes[id_neighbor].position;
            s_pos2.y += 2;
            const vec3_t pos2 = vec3_from_svec3(s_pos2);
            renderer_debug_draw_line(pos1, pos2, (pixel32_t){ .r = 255, .g = 0, .b = 0, .a = 80 }, &id_transform);
        }
    }
}
