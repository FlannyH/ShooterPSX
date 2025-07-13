#include "collision.h"

#include "fixed_point.h"
#include "renderer.h"
#include "memory.h"
#include "file.h"
#include "mesh.h"
#include "vec3.h"
#include "vec2.h"

#include <stdlib.h>
#include <string.h>

int n_ray_aabb_intersects = 0;
int n_ray_triangle_intersects = 0;
int n_vertical_cylinder_aabb_intersects = 0;
int n_vertical_cylinder_triangle_intersects = 0;

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

void handle_node_intersection_vertical_cylinder(level_collision_t* self, const bvh_node_t* current_node, const vertical_cylinder_t vertical_cylinder, rayhit_t* hit, const int rec_depth) {
    // Intersect current node
    if (vertical_cylinder_aabb_intersect(&current_node->bounds, vertical_cylinder)) {
        if (current_node->primitive_count != 0) {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++) {
                // If hit
                if (vertical_cylinder_triangle_intersect(&self->primitives[self->indices[i]], vertical_cylinder, &sub_hit)) {
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
        handle_node_intersection_vertical_cylinder(self, &self->nodes[current_node->left_first + 0], vertical_cylinder, hit, rec_depth + 1);
        handle_node_intersection_vertical_cylinder(self, &self->nodes[current_node->left_first + 1], vertical_cylinder, hit, rec_depth + 1);
    }
} 

void bvh_intersect_ray(level_collision_t* self, ray_t ray, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    if (self == NULL) return;
    if (self->root == NULL) return;
    handle_node_intersection_ray(self, self->root, ray, hit, 0);
}

void bvh_intersect_vertical_cylinder(level_collision_t* bvh, vertical_cylinder_t cyl, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    if (bvh == NULL) return;
    if (bvh->root == NULL) return;
    handle_node_intersection_vertical_cylinder(bvh, bvh->root, cyl, hit, 0);
}

void debug_draw(const level_collision_t* self, const bvh_node_t* node, const int min_depth, const int max_depth, const int curr_depth, const pixel32_t color) {
    const transform_t trans = { {0, 0, 0}, {0, 0, 0}, {-ONE, -ONE, -ONE} };

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

int ray_aabb_intersect_fancy(const aabb_t* aabb, ray_t ray, rayhit_t* hit) {
#ifdef _DEBUG
    if (!aabb) return 0;
    if (!hit) return 0;
#endif

    n_ray_aabb_intersects++;
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

int ray_aabb_intersect(const aabb_t* aabb, ray_t ray) {
#ifdef _DEBUG
    if (!aabb) return 0;
#endif

    n_ray_aabb_intersects++;
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

int ray_triangle_intersect(collision_triangle_3d_t* triangle, ray_t ray, rayhit_t* hit) {
#ifdef _DEBUG
    if (!triangle) return 0;
    if (!hit) return 0;
#endif

    n_ray_triangle_intersects++;
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
        return 1;
    }
    return 0;
#undef SHIFT_COUNT
}

// Approximation!
int vertical_cylinder_aabb_intersect(const aabb_t* aabb, const vertical_cylinder_t vertical_cylinder) {
#ifdef _DEBUG
    if (!aabb) return 0;
#endif

    n_vertical_cylinder_aabb_intersects++;

    // Check the Y axis first. If this does not overlap, there can not be a collision.
    if ((vertical_cylinder.bottom.y + vertical_cylinder.height) < aabb->min.y) return 0; // If top of cylinder is below the AABB, no intersect
    if (vertical_cylinder.bottom.y >= aabb->max.y) return 0; // If bottom of cylinder is above the AABB, no intersect
    
    // The rest can be done in 2D
    // Create a new AABB that's extended by the cylinder's radius. This is an approximation, but we use AABB's for BVH traversal only so it's fine
    const scalar_t min_x = aabb->min.x - vertical_cylinder.radius;
    const scalar_t max_x = aabb->max.x + vertical_cylinder.radius;
    const scalar_t min_z = aabb->min.z - vertical_cylinder.radius;
    const scalar_t max_z = aabb->max.z + vertical_cylinder.radius;
    const scalar_t point_x = vertical_cylinder.bottom.x;
    const scalar_t point_z = vertical_cylinder.bottom.z;

    // Check if it's inside
    return(
        point_x >= min_x &&
        point_x <= max_x &&
        point_z >= min_z &&
        point_z <= max_z
    );
}

int vertical_cylinder_aabb_intersect_fancy(const aabb_t* aabb, const vertical_cylinder_t vertical_cylinder, rayhit_t* hit) {
#ifdef _DEBUG
    if (!aabb) return 0;
    if (!hit) return 0;
#endif

    // Check the Y axis first. If this does not overlap, there can not be a collision.
    if ((vertical_cylinder.bottom.y + vertical_cylinder.height) < aabb->min.y) return 0; // If top of cylinder is below the AABB, no intersect
    if (vertical_cylinder.bottom.y >= aabb->max.y) return 0; // If bottom of cylinder is above the AABB, no intersect

    // The rest can now be done in 2D. Z becomes Y. Find the closest position on the AABB to the cylinder. 
    const vec2_t cylinder_center_pos = (vec2_t){vertical_cylinder.bottom.x, vertical_cylinder.bottom.z};
    const vec2_t closest_pos_on_aabb = {
        .x = scalar_clamp(vertical_cylinder.bottom.x, aabb->min.x, aabb->max.x),
        .y = scalar_clamp(vertical_cylinder.bottom.z, aabb->min.z, aabb->max.z),
    };

    // Now we get the distance from that point to the actual cylinder center
    const scalar_t aabb_cyl_distance_sq = vec2_magnitude_squared(vec2_sub(closest_pos_on_aabb, cylinder_center_pos));
    if (aabb_cyl_distance_sq < vertical_cylinder.radius_squared && (aabb_cyl_distance_sq >= 0)) {
        const vec2_t aabb_to_cylinder = vec2_sub(cylinder_center_pos, closest_pos_on_aabb);
        const scalar_t distance = vec2_magnitude(aabb_to_cylinder);
        const vec2_t normal = vec2_divs(aabb_to_cylinder, distance);

        if (vertical_cylinder.is_wall_check) {
            hit->normal = (vec3_t){normal.x, 0, normal.y};
            hit->position = (vec3_t){closest_pos_on_aabb.x, vertical_cylinder.bottom.y, closest_pos_on_aabb.y};
            hit->distance = vec2_magnitude(aabb_to_cylinder);
            return 1;
        }

        if ((vertical_cylinder.bottom.y + vertical_cylinder.height / 2) > (aabb->min.y + aabb->max.y / 2)) {
            hit->normal = (vec3_t){0, 4096, 0};
            hit->distance = vertical_cylinder.bottom.y - aabb->max.y;
            hit->position = (vec3_t){closest_pos_on_aabb.x, aabb->max.y, closest_pos_on_aabb.y};
        }
        else {
            hit->normal = (vec3_t){0, -4096, 0};
            hit->distance = aabb->min.y - (vertical_cylinder.bottom.y + vertical_cylinder.height);
            hit->position = (vec3_t){closest_pos_on_aabb.x, aabb->min.y, closest_pos_on_aabb.y};
        }
        return 1;
    }
    return 0;
}

scalar_t edge_function(const vec2_t a, const vec2_t b, const vec2_t p) {
    const vec2_t a_p = vec2_sub(p, a);
    const vec2_t a_b = vec2_sub(b, a);
    return vec2_cross(a_p, a_b);
}

scalar_t get_progress_of_p_on_ab(vec2_t a, vec2_t b, vec2_t p) {
    // Calculate progress along the edge
    const vec2_t ab = vec2_sub(b, a);
    const vec2_t ap = vec2_sub(p, a);
    const scalar_t ap_dot_ab = vec2_dot(ap, ab);
    const scalar_t length_ab = vec2_magnitude_squared(ab);
    scalar_t progress_along_edge = scalar_div(ap_dot_ab, length_ab);

    // Clamp it between 0.0 and 1.0
    progress_along_edge = scalar_clamp(progress_along_edge, 0, 4096);
    return progress_along_edge;
}

// for some reason the function used for the 3d triangles doesn't work in 2d? so i made my own instead
// u_out is 1.0 p lies on v0, and v_out is 1.0 if p lies on v1
vec2_t find_closest_point_on_triangle_2d(vec2_t v0, vec2_t v1, vec2_t v2, vec2_t p, scalar_t* u_out, scalar_t* v_out) {
    PANIC_IF("u_out or w_out is null!", (!u_out) || (!v_out));

    // This is what we hope to return after this
    vec2_t closest_point;

    // Calculate edge0
    const vec2_t v1_p = vec2_sub(p, v1);
    const vec2_t v1_v2 = vec2_sub(v2, v1);
    const scalar_t edge0 = vec2_cross(v1_p, v1_v2);

    // Calculate edge1
    const vec2_t v2_p = vec2_sub(p, v2);
    const vec2_t v2_v0 = vec2_sub(v0, v2);
    const scalar_t edge1 = vec2_cross(v2_p, v2_v0);

    // Calculate edge2
    const vec2_t v0_p = vec2_sub(p, v0);
    const vec2_t v0_v1 = vec2_sub(v1, v0);
    const scalar_t edge2 = vec2_cross(v0_p, v0_v1);

    // Are we inside triangle?
    if (edge0 >= 0 && edge1 >= 0 && edge2 >= 0) {
        // Normalize barycoords
        const vec2_t v1_v0 = vec2_sub(v0, v1);
        const scalar_t area = vec2_cross(v1_v0, v1_v2);
        *u_out = scalar_div(edge0, area);
        *v_out = scalar_div(edge1, area);

        // Return P as is
        return p;
    }

    //It's impossible for all the numbers to be negative
    if ((edge0 < 0 && edge1 < 0 && edge2 < 0)) {
        vec2_t error;
        error.x = INT32_MAX;
        error.y = 1;
        return error;
    }

    // Otherwise, project onto the first negative edge we find
    scalar_t progress;

    // A = v1, B = v2
    if (edge0 < 0) {
        progress = get_progress_of_p_on_ab(v1, v2, p);
        closest_point = vec2_add(v1, vec2_mul(vec2_sub(v2, v1), vec2_from_scalar(progress)));
        *u_out = 0;
        *v_out = 4096 - progress;

    }
    // A = v2, B = v0
    else if (edge1 < 0) {
        progress = get_progress_of_p_on_ab(v2, v0, p);
        closest_point = vec2_add(v2, vec2_mul(vec2_sub(v0, v2), vec2_from_scalar(progress)));
        *u_out = progress;
        *v_out = 0;
    }
    // A = v0, B = v1
    else /*if (edge2 < 0)*/ {
        progress = get_progress_of_p_on_ab(v0, v1, p);
        closest_point = vec2_add(v0, vec2_mul(vec2_sub(v1, v0), vec2_from_scalar(progress)));
        *u_out = 4096 - progress;
        *v_out = progress;
    }

    // Calculate the point and return it
    return closest_point;
}

scalar_t get_progress_of_p_on_ab_3d(vec3_t a, vec3_t b, vec3_t p) {
    // Calculate progress along the edge
    const vec3_t ab = vec3_sub(b, a);
    const vec3_t ap = vec3_sub(p, a);
    const scalar_t ap_dot_ab = vec3_dot(ap, ab);
    const scalar_t length_ab = vec3_magnitude_squared(ab);
    scalar_t progress_along_edge = scalar_div(ap_dot_ab, length_ab);

    // Clamp it between 0.0 and 1.0
    progress_along_edge = scalar_clamp(progress_along_edge, 0, 4096);
    return progress_along_edge;
}

// Assumes `p` lies on the triangle's plane
vec3_t find_closest_point_on_triangle_3d(collision_triangle_3d_t* triangle, vec3_t p) {
    // Calculate edge0 
    const vec3_t v1_p = vec3_sub(p, triangle->v1);
    const vec3_t v1_v2 = vec3_sub(triangle->v2, triangle->v1);
    const vec3_t edge0 = vec3_cross(v1_v2, v1_p);
    const scalar_t signed_distance_from_12 = vec3_dot(edge0, triangle->normal);

    // Calculate edge1
    const vec3_t v2_p = vec3_sub(p, triangle->v2);
    const vec3_t v2_v0 = vec3_sub(triangle->v0, triangle->v2);
    const vec3_t edge1 = vec3_cross(v2_v0, v2_p);
    const scalar_t signed_distance_from_20 = vec3_dot(edge1, triangle->normal);

    // Calculate edge2
    const vec3_t v0_p = vec3_sub(p, triangle->v0);
    const vec3_t v0_v1 = vec3_sub(triangle->v1, triangle->v0);
    const vec3_t edge2 = vec3_cross(v0_v1, v0_p);
    const scalar_t signed_distance_from_01 = vec3_dot(edge2, triangle->normal);

    // Point is inside triangle
    if (signed_distance_from_12 >= 0 
    &&  signed_distance_from_20 >= 0
    &&  signed_distance_from_01 >= 0
    ) {
        return p;
    }

    // A's dorito zone - closest point is A
    if (signed_distance_from_12 >= 0 
    &&  signed_distance_from_20 < 0
    &&  signed_distance_from_01 < 0
    ) {
        return triangle->v0;
    }
    
    // B's dorito zone - closest point is B
    if (signed_distance_from_12 < 0 
    &&  signed_distance_from_20 >= 0
    &&  signed_distance_from_01 < 0
    ) {
        return triangle->v1;
    }
    
    // C's dorito zone - closest point is C
    if (signed_distance_from_12 < 0 
    &&  signed_distance_from_20 < 0
    &&  signed_distance_from_01 >= 0
    ) {
        return triangle->v2;
    }

    // AB's chonko zone - closest point is on AB
    if (signed_distance_from_12 >= 0 
    &&  signed_distance_from_20 >= 0
    &&  signed_distance_from_01 < 0
    ) {
        const scalar_t t = get_progress_of_p_on_ab_3d(triangle->v0, triangle->v1, p);
        return vec3_add(triangle->v0, vec3_muls(v0_v1, t));
    }

    // BC's chonko zone - closest point is on BC
    if (signed_distance_from_12 < 0 
    &&  signed_distance_from_20 >= 0
    &&  signed_distance_from_01 >= 0
    ) {
        const scalar_t t = get_progress_of_p_on_ab_3d(triangle->v1, triangle->v2, p);
        return vec3_add(triangle->v1, vec3_muls(v1_v2, t));
    }

    // CA's chonko zone - closest point is on CA. There is no other logical combination so if we get here it has to be this
    /*if (signed_distance_from_12 >= 0 
    &&  signed_distance_from_20 < 0
    &&  signed_distance_from_01 >= 0
    ) */ {
        const scalar_t t = get_progress_of_p_on_ab_3d(triangle->v2, triangle->v0, p);
        return vec3_add(triangle->v2, vec3_muls(v2_v0, t));
    }
}

int vertical_cylinder_triangle_intersect(collision_triangle_3d_t* triangle, vertical_cylinder_t vertical_cylinder, rayhit_t* hit) {
#ifdef _DEBUG
    if (!triangle) return 0;
    if (!hit) return 0;
#endif

    n_vertical_cylinder_triangle_intersects++;

    if (vertical_cylinder.is_wall_check) {
        if (triangle->normal.y > 0) {
            return 0;
        }
    }
    else {
        if (triangle->normal.y <= 0) {
            return 0;
        }
    }

    // Project everything into 2D top-down
    const vec2_t v0 = { triangle->v0.x, triangle->v0.z };
    const vec2_t v1 = { triangle->v1.x, triangle->v1.z };
    const vec2_t v2 = { triangle->v2.x, triangle->v2.z };
    const vec2_t position = { vertical_cylinder.bottom.x, vertical_cylinder.bottom.z };

    // Find closest point
    scalar_t u, v, w;
    const vec2_t closest_pos_on_triangle = find_closest_point_on_triangle_2d(v0, v1, v2, position, &u, &v);

    // If closest point returned a faulty value, ignore it
    if (closest_pos_on_triangle.x == INT32_MAX) {
        hit->distance = INT32_MAX;
        return 0;
    }
    w = 4096 - u - v;

    // We found the closest point! Does the circle intersect it?
    const scalar_t distance_to_closest_point = vec2_magnitude_squared(vec2_sub(position, closest_pos_on_triangle));
    if (distance_to_closest_point >= vertical_cylinder.radius_squared) {
        hit->distance = INT32_MAX;
        return 0;
    }

    // It does! calculate the Y coordinate
    vec3_t closest_pos_3d;
    closest_pos_3d.x = closest_pos_on_triangle.x;
    closest_pos_3d.z = closest_pos_on_triangle.y;

    // Edge case! If the triangle is facing horizontally, the 2D projection will be degenerate.
    if (scalar_abs(triangle->normal.y) < 20) {
        // Figure out what Y range we have
        // Project v0-point onto v0-v1
        const vec2_t v0_point = vec2_sub(closest_pos_on_triangle, v0);
        const vec2_t v1_point = vec2_sub(closest_pos_on_triangle, v1);
        const vec2_t v2_point = vec2_sub(closest_pos_on_triangle, v2);
        const vec2_t v0_v1 = vec2_sub(v1, v0);
        const vec2_t v1_v2 = vec2_sub(v2, v1);
        const vec2_t v2_v0 = vec2_sub(v0, v2);
        const scalar_t t_v0v1 = scalar_div(vec2_dot(v0_point, v0_v1), vec2_magnitude_squared(v0_v1));
        const scalar_t t_v1v2 = scalar_div(vec2_dot(v1_point, v1_v2), vec2_magnitude_squared(v1_v2));
        const scalar_t t_v2v0 = scalar_div(vec2_dot(v2_point, v2_v0), vec2_magnitude_squared(v2_v0));
        scalar_t min_y = INT32_MAX;
        scalar_t max_y = INT32_MIN;
        if (!is_infinity(t_v0v1) && t_v0v1 >= 0 && t_v0v1 <= 4096) {
            const scalar_t y = triangle->v0.y + scalar_mul(triangle->v1.y - triangle->v0.y, t_v0v1);
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
        if (!is_infinity(t_v1v2) && t_v1v2 >= 0 && t_v1v2 <= 4096) {
            const scalar_t y = triangle->v1.y + scalar_mul(triangle->v2.y - triangle->v1.y, t_v1v2);
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
        if (!is_infinity(t_v2v0) && t_v2v0 >= 0 && t_v2v0 <= 4096) {
            const scalar_t y = triangle->v2.y + scalar_mul(triangle->v0.y - triangle->v2.y, t_v2v0);
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }

        // If the ranges do not overlap, no collision happened
        if (max_y < vertical_cylinder.bottom.y || min_y > (vertical_cylinder.bottom.y + vertical_cylinder.height)) {
            hit->distance = INT32_MAX;
            return 0;
        }

        // Find the lowest point that is in the range
        closest_pos_3d.y = scalar_max(min_y, vertical_cylinder.bottom.y);
    }
    else {
        closest_pos_3d.y = triangle->v0.y;
        closest_pos_3d.y = (closest_pos_3d.y + scalar_mul(v, (triangle->v1.y - triangle->v0.y)));
        closest_pos_3d.y = (closest_pos_3d.y + scalar_mul(w, (triangle->v2.y - triangle->v0.y)));
    }

    // Is this Y coordinate within the cylinder's range?
    const scalar_t min = vertical_cylinder.bottom.y;
    const scalar_t max = (min + vertical_cylinder.height);
    if (closest_pos_3d.y >= min && closest_pos_3d.y <= max) {
        // Return this point
        hit->position = closest_pos_3d;
        if (triangle->normal.y == 0) {
            vec2_t center_to_intersect = vec2_sub(position, closest_pos_on_triangle);
            scalar_t magnitude_cti = vec2_magnitude(center_to_intersect);
            center_to_intersect = vec2_divs(center_to_intersect, scalar_max(1, magnitude_cti));
            hit->normal = (vec3_t){ center_to_intersect.x, 0, center_to_intersect.y };
            if (vec3_dot(hit->normal, triangle->normal) < 0) {
                hit->distance = INT32_MAX;
                return 0;
            }
            hit->distance = magnitude_cti;
        }
        else {
            hit->normal = triangle->normal;
            hit->distance = scalar_sqrt(distance_to_closest_point);
        }
        hit->type = RAY_HIT_TYPE_TRIANGLE;
        hit->tri.triangle = triangle;
        return 1;
    }

    hit->distance = INT32_MAX;
    return 0;
}

void collision_clear_stats(void) {
    n_ray_aabb_intersects = 0;
    n_ray_triangle_intersects = 0;
    n_vertical_cylinder_aabb_intersects = 0;
    n_vertical_cylinder_triangle_intersects = 0;
}

vec3_t closest_point_on_line_segment(const vec3_t a, const vec3_t b, const vec3_t point) {
    const vec3_t ab = vec3_sub(b, a);
    const vec3_t ap = vec3_sub(point, a);
    const scalar_t ap_projected_onto_ab = scalar_div(vec3_dot(ab, ap), vec3_dot(ab, ab));
    const scalar_t t = scalar_clamp(ap_projected_onto_ab, 0, ONE);
    return vec3_add(a, vec3_muls(ab, t));
}

int sphere_triangle_intersect(collision_triangle_3d_t* triangle, sphere_t sphere, rayhit_t* hit) {
#ifdef _DEBUG
    if (!triangle) return 0;
    if (!hit) return 0;
#endif
    const vec3_t p0 = vec3_shift_right(triangle->v0, 4);
    const vec3_t p1 = vec3_shift_right(triangle->v1, 4);
    const vec3_t p2 = vec3_shift_right(triangle->v2, 4);
    const vec3_t center = vec3_shift_right(sphere.center, 4);
    const scalar_t radius = sphere.radius >> 4;
    const scalar_t radius_squared = scalar_mul(radius, radius);

    const vec3_t N = vec3_normalize(vec3_cross(vec3_sub(p1, p0), vec3_sub(p2, p0)));
    const scalar_t dist = vec3_dot(vec3_sub(center, p0), N);

    if (dist > 0) return 0;
    if (dist < -radius || dist > radius) return 0;
    
    const vec3_t point0 = vec3_sub(center, vec3_muls(N, dist));
    const vec3_t c0 = vec3_cross(vec3_sub(point0, p0), vec3_sub(p1, p0));
    const vec3_t c1 = vec3_cross(vec3_sub(point0, p1), vec3_sub(p2, p1));
    const vec3_t c2 = vec3_cross(vec3_sub(point0, p2), vec3_sub(p0, p2));
    int inside = (vec3_dot(c0, N) <= 0) && (vec3_dot(c1, N) <= 0) && (vec3_dot(c2, N) <= 0);

    // edge 1
    const vec3_t point1 = closest_point_on_line_segment(p0, p1, center);
    const vec3_t v1 = vec3_sub(center, point1);
    const scalar_t distsq1 = vec3_dot(v1, v1);

    // edge 2
    const vec3_t point2 = closest_point_on_line_segment(p1, p2, sphere.center);
    const vec3_t v2 = vec3_sub(center, point2);
    const scalar_t distsq2 = vec3_dot(v2, v2);

    // edge 3
    const vec3_t point3 = closest_point_on_line_segment(p2, p0, sphere.center);
    const vec3_t v3 = vec3_sub(center, point3);
    const scalar_t distsq3 = vec3_dot(v3, v3);
    int intersects = (distsq1 < radius_squared) || (distsq2 < radius_squared) || (distsq3 < radius_squared);

    if (inside || intersects) {
        vec3_t best_point = point0;
        vec3_t intersection_vec;

        if (inside) {
            intersection_vec = vec3_sub(center, point0);
        }
        else {
            vec3_t d = vec3_sub(center, point1);
            scalar_t best_dist_squared = vec3_dot(d, d);
            best_point = point1;
            intersection_vec = d;

            d = vec3_sub(center, point2);
            scalar_t dist_squared = vec3_dot(d, d);
            if (dist_squared < best_dist_squared) {
                dist_squared = vec3_dot(d, d);
                best_point = point2;
                intersection_vec = d;
            }

            d = vec3_sub(center, point3);
            dist_squared = vec3_dot(d, d);
            if (dist_squared < best_dist_squared) {
                dist_squared = vec3_dot(d, d);
                best_point = point3;
                intersection_vec = d;
            }
        }

        const scalar_t len = scalar_sqrt(vec3_magnitude_squared(intersection_vec));
        hit->normal = vec3_divs(intersection_vec, len);
        hit->distance = (radius - len) << 4;
        hit->distance_along_normal = (radius - len) << 4;
        hit->position = vec3_shift_left(best_point, 4);
        return 1;
    }

    return 0;
}

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
        printf("[ERROR] Error loading collision mesh '%s', file header is invalid!\n", path);
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
