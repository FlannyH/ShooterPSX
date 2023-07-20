#include "collision.h"

#include <stdlib.h>
#include <string.h>

#include "mesh.h"
#include "renderer.h"
#include "vec3.h"
#include "vec2.h"

int n_ray_aabb_intersects = 0;
int n_ray_triangle_intersects = 0;
int n_sphere_aabb_intersects = 0;
int n_sphere_triangle_intersects = 0;
int n_vertical_cylinder_aabb_intersects = 0;
int n_vertical_cylinder_triangle_intersects = 0;

void bvh_construct(bvh_t* bvh, const col_mesh_file_tri_t* primitives, const uint16_t n_primitives) {
    // Convert triangles from the model into collision triangles
    // todo: maybe store this mesh in the file? not sure if it's worth implementing but could be nice
    bvh->primitives = malloc(sizeof(collision_triangle_3d_t) * n_primitives);
    bvh->n_primitives = n_primitives;
    PANIC_IF("failed to allocate memory for collision model!", bvh->primitives == 0);

    for (size_t i = 0; i < n_primitives; ++i) {
        // Set position
        bvh->primitives[i].v0 = vec3_from_int32s(-(int32_t)primitives[i].v0.x * COL_SCALE, -(int32_t)primitives[i].v0.y * COL_SCALE, -(int32_t)primitives[i].v0.z * COL_SCALE);
        bvh->primitives[i].v1 = vec3_from_int32s(-(int32_t)primitives[i].v1.x * COL_SCALE, -(int32_t)primitives[i].v1.y * COL_SCALE, -(int32_t)primitives[i].v1.z * COL_SCALE);
        bvh->primitives[i].v2 = vec3_from_int32s(-(int32_t)primitives[i].v2.x * COL_SCALE, -(int32_t)primitives[i].v2.y * COL_SCALE, -(int32_t)primitives[i].v2.z * COL_SCALE);

        // Calculate normal
        const vec3_t ac = vec3_sub(vec3_shift_right(bvh->primitives[i].v1, 4), vec3_shift_right(bvh->primitives[i].v0, 4));
        const vec3_t ab = vec3_sub(vec3_shift_right(bvh->primitives[i].v2, 4), vec3_shift_right(bvh->primitives[i].v0, 4));
        bvh->primitives[i].normal = vec3_cross(ac, ab);
        WARN_IF("bvh normal calculation return infinity", (bvh->primitives[i].normal.x | bvh->primitives[i].normal.y | bvh->primitives[i].normal.z) == INT32_MAX);
        bvh->primitives[i].normal = vec3_normalize(bvh->primitives[i].normal);

        // Calculate center
        bvh->primitives[i].center = bvh->primitives[i].v0;
        bvh->primitives[i].center = vec3_add(bvh->primitives[i].center, bvh->primitives[i].v1);
        bvh->primitives[i].center = vec3_add(bvh->primitives[i].center, bvh->primitives[i].v2);
        bvh->primitives[i].center = vec3_muls(bvh->primitives[i].center, 1365);

        // Precalculate triangle collision variables
        vec3_t c = vec3_sub(bvh->primitives[i].v2, bvh->primitives[i].v0);
        vec3_t b = vec3_sub(bvh->primitives[i].v1, bvh->primitives[i].v0);
        int n_shifts_edge = 0;

        // Make sure we don't overflow - we can do expensive stuff here so we don't have to do that later :)
        while (vec3_dot(c, c) == INT32_MAX || vec3_dot(b, c) == INT32_MAX || vec3_dot(b, b) == INT32_MAX) {
            c = vec3_shift_right(c, 1);
            b = vec3_shift_right(b, 1);
            ++n_shifts_edge;
        }
        // Shift some more for good measure - this is scuffed but will have to do
        c = vec3_shift_right(c, 4);
        b = vec3_shift_right(b, 4);
        n_shifts_edge += 4;

        scalar_t cc = vec3_dot(c, c);
        scalar_t bc = vec3_dot(b, c);
        scalar_t bb = vec3_dot(b, b);
        int n_shifts_dot = 0;

        // Make sure we don't overflow - we can do expensive stuff here so we don't have to do that later :)
        while (scalar_mul(cc, bb) == INT32_MAX || scalar_mul(bc, bc) == INT32_MAX) {
            cc = cc >> 1;
            bc = bc >> 1;
            bb = bb >> 1;
            ++n_shifts_dot;
        }

        // Shift some more for good measure
        cc = cc >> 4;
        bc = bc >> 4;
        bb = bb >> 4;
        n_shifts_dot += 4;

        //Get barycentric coordinates
        const scalar_t cc_bb = scalar_mul(cc, bb); 
        const scalar_t bc_bc = scalar_mul(bc, bc); 
        const scalar_t d = cc_bb - bc_bc;

        // Put it in the triangle
        bvh->primitives[i].edge_c = c;
        bvh->primitives[i].edge_b = b;
        bvh->primitives[i].cc = cc; 
        bvh->primitives[i].bc = bc;
        bvh->primitives[i].bb = bb;
        bvh->primitives[i].d = d;
        bvh->primitives[i].n_shifts_edge = n_shifts_edge;
        bvh->primitives[i].n_shifts_dot = n_shifts_dot;
    }


    // Create index array
    bvh->indices = malloc(sizeof(uint16_t) * n_primitives);
    for (int i = 0; i < n_primitives; i++)
    {
        bvh->indices[i] = i;
    }

    // Create root node
    bvh->nodes = malloc(sizeof(bvh_node_t) * n_primitives * 2);
    bvh->node_pointer = 2;
    bvh->root = &bvh->nodes[0]; // todo: maybe we can just hard code the root to be index 0 and skip this indirection?
    bvh->root->left_first = 0;
    bvh->root->primitive_count = n_primitives;
    bvh_subdivide(bvh, bvh->root, 0);
}

aabb_t bvh_get_bounds(const bvh_t* bvh, const uint16_t first, const uint16_t count)
{
    aabb_t result;
    result.max = vec3_from_int32s(INT32_MIN, INT32_MIN, INT32_MIN);
    result.min = vec3_from_int32s(INT32_MAX, INT32_MAX, INT32_MAX);
    for (int i = 0; i < count; i++)
    {
        const aabb_t curr_primitive_bounds = collision_triangle_get_bounds(&bvh->primitives[bvh->indices[first + i]]);

        result.min = vec3_min(result.min, curr_primitive_bounds.min);
        result.max = vec3_max(result.max, curr_primitive_bounds.max);
    }
    return result;
}

void bvh_subdivide(bvh_t* bvh, bvh_node_t* node, const int recursion_depth) {
    //Determine AABB for primitives in array
    node->bounds = bvh_get_bounds(bvh, node->left_first, node->primitive_count);

    if (node->primitive_count < 3)
    {
        node->is_leaf = 1;
        return;
    }

    //Determine split axis - choose biggest axis
    axis_t split_axis = axis_x;
    scalar_t split_pos = 0;

    const vec3_t size = vec3_sub(node->bounds.max, node->bounds.min);

    if (size.x > size.y 
        && size.x > size.z)
    {
        split_axis = axis_x;
        split_pos = (node->bounds.max.x + node->bounds.min.x);
        split_pos = split_pos >> 1;
    }

    if (size.y > size.x
        && size.y > size.z)
    {
        split_axis = axis_y;
        split_pos = (node->bounds.max.y + node->bounds.min.y);
        split_pos = split_pos >> 1;
    }

    if (size.z > size.x
        && size.z > size.y)
    {
        split_axis = axis_z;
        split_pos = (node->bounds.max.z + node->bounds.min.z);
        split_pos = split_pos >> 1;
    }

    //Partition the index array, and get the split position
    uint16_t split_index = 0;
    bvh_partition(bvh, split_axis, split_pos, node->left_first, node->primitive_count, &split_index);

    //If splitIndex and node count are the same, we've reached a dead end, so stop here
    if (split_index == node->primitive_count)
    {
        node->is_leaf = 1;
        return;
    }

    //If splitIndex and the start are the same, we've reached a dead end, so stop here
    if (split_index == node->left_first)
    {
        node->is_leaf = 1;
        return;
    }

    //Save the start index of this node
    const int start_index = node->left_first;

    //Create child nodes
    node->left_first = bvh->node_pointer;
    bvh->node_pointer += 2;

    //Start
    bvh->nodes[node->left_first + 0].left_first = start_index;
    bvh->nodes[node->left_first + 1].left_first = split_index;

    //Count
    bvh->nodes[node->left_first + 0].primitive_count = split_index - start_index;
    bvh->nodes[node->left_first + 1].primitive_count = start_index + node->primitive_count - split_index;

    bvh_subdivide(bvh, &bvh->nodes[node->left_first + 0], recursion_depth + 1);
    bvh_subdivide(bvh, &bvh->nodes[node->left_first + 1], recursion_depth + 1);

    node->is_leaf = 0;
}

void handle_node_intersection_ray(bvh_t* self, const bvh_node_t* current_node, const ray_t ray, rayhit_t* hit, const int rec_depth) {
    // Intersect current node
    if (ray_aabb_intersect(&current_node->bounds, ray))
    {
        // If it's a leaf
        if (current_node->is_leaf)
        {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (ray_triangle_intersect(&self->primitives[self->indices[i]], ray, &sub_hit)) {
                    // If lowest distance
                    if (sub_hit.distance < hit->distance && sub_hit.distance >= 0)
                    {
                        // Copy the hit info into the output hit for the BVH traversal
                        memcpy(hit, &sub_hit, sizeof(rayhit_t));
                        hit->triangle = &self->primitives[self->indices[i]];
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

void handle_node_intersection_sphere(bvh_t* self, const bvh_node_t* current_node, const sphere_t sphere, rayhit_t* hit, const int rec_depth) {
    // Intersect current node
    if (sphere_aabb_intersect(&current_node->bounds, sphere))
    {
        // If it's a leaf
        if (current_node->is_leaf)
        {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (sphere_triangle_intersect_old(&self->primitives[self->indices[i]], sphere, &sub_hit)) {
                    const collision_triangle_3d_t* tri = &self->primitives[self->indices[i]];

                    // When colliding with a surface, you would want 
                    const vec3_t player_to_surface = vec3_sub(sphere.center, sub_hit.position);
                    sub_hit.distance_along_normal = vec3_dot(sub_hit.normal, player_to_surface);

                    if (sub_hit.distance_along_normal >= 0 && sub_hit.distance_along_normal <= sphere.radius)
                    {
                        // Copy the hit info into the output hit for the BVH traversal
                        memcpy(hit, &sub_hit, sizeof(rayhit_t));
                        hit->triangle = &self->primitives[self->indices[i]];
                    }
                }
            }
            return;
        }

        //Otherwise, intersect child nodes
        handle_node_intersection_sphere(self, &self->nodes[current_node->left_first + 0], sphere, hit, rec_depth + 1);
        handle_node_intersection_sphere(self, &self->nodes[current_node->left_first + 1], sphere, hit, rec_depth + 1);
    }
}

void handle_node_intersection_vertical_cylinder(bvh_t* self, const bvh_node_t* current_node, const vertical_cylinder_t vertical_cylinder, rayhit_t* hit, const int rec_depth) {
    // Intersect current node
    if (vertical_cylinder_aabb_intersect(&current_node->bounds, vertical_cylinder))
    {
        // If it's a leaf
        if (current_node->is_leaf)
        {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (vertical_cylinder_triangle_intersect(&self->primitives[self->indices[i]], vertical_cylinder, &sub_hit)) {
                    // If lowest distance
                    if (sub_hit.distance < hit->distance && sub_hit.distance >= 0)
                    {
                        // Copy the hit info into the output hit for the BVH traversal
                        memcpy(hit, &sub_hit, sizeof(rayhit_t));
                        hit->triangle = &self->primitives[self->indices[i]];
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

void handle_node_intersection_capsule(bvh_t* self, const bvh_node_t* current_node, const capsule_t capsule, rayhit_t* hit, const int rec_depth) {
    // Intersect current node
    if (capsule_aabb_intersect(&current_node->bounds, capsule))
    {
        // If it's a leaf
        if (current_node->is_leaf)
        {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (capsule_triangle_intersect(&self->primitives[self->indices[i]], capsule, &sub_hit)) {
                    const collision_triangle_3d_t* tri = &self->primitives[self->indices[i]];
                    // If lowest distance
                    if (sub_hit.distance < hit->distance && sub_hit.distance >= 0)
                    {
                        // Copy the hit info into the output hit for the BVH traversal
                        memcpy(hit, &sub_hit, sizeof(rayhit_t));
                        hit->triangle = &self->primitives[self->indices[i]];
                    }
                }
            }
            return;
        }

        //Otherwise, intersect child nodes
        handle_node_intersection_capsule(self, &self->nodes[current_node->left_first + 0], capsule, hit, rec_depth + 1);
        handle_node_intersection_capsule(self, &self->nodes[current_node->left_first + 1], capsule, hit, rec_depth + 1);
    }
}


void bvh_intersect_ray(bvh_t* self, ray_t ray, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    handle_node_intersection_ray(self, self->root, ray, hit, 0);
}

void bvh_intersect_sphere(bvh_t* bvh, sphere_t sphere, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    handle_node_intersection_sphere(bvh, bvh->root, sphere, hit, 0);
}

void bvh_intersect_vertical_cylinder(bvh_t* bvh, vertical_cylinder_t cyl, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    handle_node_intersection_vertical_cylinder(bvh, bvh->root, cyl, hit, 0);
}

void bvh_intersect_capsule(bvh_t* bvh, capsule_t capsule, rayhit_t* hit) {
    hit->distance = INT32_MAX;
    handle_node_intersection_capsule(bvh, bvh->root, capsule, hit, 0);
}

void bvh_swap_primitives(uint16_t* a, uint16_t* b) {
    const int tmp = *a;
    *a = *b;
    *b = tmp;
}

void bvh_partition(const bvh_t* bvh, const axis_t axis, const scalar_t pivot, const uint16_t start, const uint16_t count, uint16_t* split_index) {
    int i = start;
    for (int j = start; j < start + count; j++)
    {
        // Get min and max of the axis we want
        const aabb_t bounds = collision_triangle_get_bounds(&bvh->primitives[bvh->indices[j]]);

        // Get center
        vec3_t center = vec3_add(bounds.min, bounds.max);
        center.x = center.x >> 1;
        center.y = center.y >> 1;
        center.z = center.z >> 1;
        const scalar_t* center_points = (scalar_t*)&center;

        // If the current primitive's center's <axis>-component is greated than the pivot's <axis>-component
        if (center_points[(size_t)axis] > pivot && (j != i))
        {
            // Move the primitive's index to the first partition of this node
            bvh_swap_primitives(&bvh->indices[i], &bvh->indices[j]);
            i++;
        }
    }
    *split_index = i;
}

void bvh_from_mesh(bvh_t* bvh, const mesh_t* mesh) {
    bvh_construct(bvh, (triangle_3d_t*)mesh->vertices, (uint16_t)mesh->n_triangles);
}

void bvh_from_model(bvh_t* bvh, const collision_mesh_t* mesh) {
    // Construct the BVH
    bvh_construct(bvh, mesh->verts, mesh->n_verts / 3);
}

void debug_draw(const bvh_t* self, const bvh_node_t* node, const int min_depth, const int max_depth, const int curr_depth, const pixel32_t color) {
    transform_t trans = { {0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096} };

    // Draw box of this node - only if within the depth bounds
    if (curr_depth > max_depth) {
        return;
    }

    if (curr_depth >= min_depth) {
        renderer_debug_draw_aabb(&node->bounds, color, &trans);
    }

    // If child nodes exist
    if (node->is_leaf) {
        return;
    }

    // Draw child nodes
    debug_draw(self, &self->nodes[node->left_first + 0], min_depth, max_depth, curr_depth + 1, color);
    debug_draw(self, &self->nodes[node->left_first + 1], min_depth, max_depth, curr_depth + 1, color);
}

void bvh_debug_draw(const bvh_t* bvh, const int min_depth, const int max_depth, const pixel32_t color) {
    debug_draw(bvh, bvh->root, min_depth, max_depth, 0, color);
}

int point_aabb_intersect(const aabb_t* aabb, vec3_t point) {
    return (point.x >= aabb->min.x)
    &&     (point.y >= aabb->min.y)
    &&     (point.z >= aabb->min.z)
    &&     (point.x <= aabb->max.x)
    &&     (point.y <= aabb->max.y)
    &&     (point.z <= aabb->max.z);
}

int ray_aabb_intersect(const aabb_t* aabb, ray_t ray) {
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
    n_ray_triangle_intersects++;
    //Get vectors
    vec3_t v0 = triangle->v0;
    vec3_t v1 = triangle->v1;
    vec3_t v2 = triangle->v2;

    // Get distance to each point
    const scalar_t distance_p_v0 = vec3_magnitude_squared(vec3_sub(v0, ray.position));
    const scalar_t distance_p_v1 = vec3_magnitude_squared(vec3_sub(v1, ray.position));
    const scalar_t distance_p_v2 = vec3_magnitude_squared(vec3_sub(v2, ray.position));

    // If all of them are too far away, we didn't hit it, skip
    const scalar_t distance_min = scalar_min(scalar_min(distance_p_v0, distance_p_v1), distance_p_v2);
    if (distance_min < scalar_mul(ray.length, ray.length)) {

        hit->distance = INT32_MAX;
        return 0;
    }

    // Calculate normal
    const vec3_t normal_normalized = vec3_neg(triangle->normal);
    const scalar_t dot_dir_nrm = vec3_dot(ray.direction, normal_normalized);

    // If the ray is perfectly parallel to the plane, we did not hit it
    if (dot_dir_nrm == 0)
    {
        hit->distance = INT32_MAX;
        return 0;
    }

    //Get distance to intersection point
    const vec3_t temp = vec3_sub(triangle->center, ray.position);
    scalar_t distance = vec3_dot(temp, normal_normalized);
    distance = scalar_div(distance, dot_dir_nrm);

    // If the distance is negative, the plane is behind the ray origin, so we did not hit it
    if (distance < 0) {
        hit->distance = INT32_MAX;
        return 0;
    }

    //Get position
    const vec3_t position = vec3_add(ray.position, vec3_muls(ray.direction, distance));

    // Shift it to the right by 4 - to avoid overflow with bigger triangles at the cost of some precision
    v0 = vec3_shift_right(v0, 4);
    v1 = vec3_shift_right(v1, 4);
    v2 = vec3_shift_right(v2, 4);

    // Get edges
    const vec3_t c = vec3_sub(v2, v0);
    const vec3_t b = vec3_sub(v1, v0);

    // Get more vectors
    vec3_t p = vec3_sub(position, triangle->v0);
    p = vec3_shift_right(p, 4);

    //Get dots
    const scalar_t cc = vec3_dot(c, c) >> 6; WARN_IF("vec3_dot(c, c) overflowed", is_infinity(vec3_dot(c, c)));
    const scalar_t bc = vec3_dot(b, c) >> 6; WARN_IF("vec3_dot(b, c) overflowed", is_infinity(vec3_dot(b, c)));
    const scalar_t pc = vec3_dot(c, p) >> 6; WARN_IF("vec3_dot(c, p) overflowed", is_infinity(vec3_dot(c, p)));
    const scalar_t bb = vec3_dot(b, b) >> 6; WARN_IF("vec3_dot(b, b) overflowed", is_infinity(vec3_dot(b, b)));
    const scalar_t pb = vec3_dot(b, p) >> 6; WARN_IF("vec3_dot(b, p) overflowed", is_infinity(vec3_dot(b, p)));

    //Get barycentric coordinates
    const scalar_t cc_bb = scalar_mul(cc, bb); WARN_IF("cc_bb overflowed", cc_bb == INT32_MAX || cc_bb == -INT32_MAX);
    const scalar_t bc_bc = scalar_mul(bc, bc); WARN_IF("bc_bc overflowed", bc_bc == INT32_MAX || bc_bc == -INT32_MAX);
    const scalar_t bb_pc = scalar_mul(bb, pc); WARN_IF("bb_pc overflowed", bb_pc == INT32_MAX || bb_pc == -INT32_MAX);
    const scalar_t bc_pb = scalar_mul(bc, pb); WARN_IF("bc_pb overflowed", bc_pb == INT32_MAX || bc_pb == -INT32_MAX);
    const scalar_t cc_pb = scalar_mul(cc, pb); WARN_IF("cc_pb overflowed", cc_pb == INT32_MAX || cc_pb == -INT32_MAX);
    const scalar_t bc_pc = scalar_mul(bc, pc); WARN_IF("bc_pc overflowed", bc_pc == INT32_MAX || bc_pc == -INT32_MAX);
    const scalar_t d = cc_bb - bc_bc;
    scalar_t u = bb_pc - bc_pb;
    scalar_t v = cc_pb - bc_pc;
    u = scalar_div(u, d);
    v = scalar_div(v, d);

    // If the point is inside the triangle, store the this result - we shift the 4 bits back out here too
    if ((u >= 0) && (v >= 0) && ((u + v) <= 4096 << 4))
    {
        hit->distance = distance;
        hit->position = position;
        hit->normal = triangle->normal;
        hit->triangle = triangle;
        return 1;
    }
    return 0;
}

int sphere_aabb_intersect(const aabb_t* aabb, const sphere_t sphere) {
    n_sphere_aabb_intersects++;
    // First check if the center is inside the AABB
    if (
        sphere.center.x >= aabb->min.x && 
        sphere.center.x <= aabb->max.x && 
        sphere.center.y >= aabb->min.y && 
        sphere.center.y <= aabb->max.y && 
        sphere.center.z >= aabb->min.z && 
        sphere.center.z <= aabb->max.z
    ) {
        return 1;
    }

    // Find the point on the AABB that's closest to the sphere's center
    const vec3_t closest_point = {
        scalar_max(aabb->min.x, scalar_min(sphere.center.x, aabb->max.x)),
        scalar_max(aabb->min.y, scalar_min(sphere.center.y, aabb->max.y)),
        scalar_max(aabb->min.z, scalar_min(sphere.center.z, aabb->max.z))
    };

    // Get the distance from that point to the sphere
    const vec3_t sphere_center_to_closest_point = vec3_sub(sphere.center, closest_point);
    const scalar_t distance_squared = vec3_magnitude_squared(sphere_center_to_closest_point);
    return distance_squared <= sphere.radius_squared;
}

// Approximation!
int vertical_cylinder_aabb_intersect(const aabb_t* aabb, const vertical_cylinder_t vertical_cylinder) {
    n_vertical_cylinder_aabb_intersects++;
    // Exit early if the AABB and vertical cylinder do not overlap on the Y-axis
    if (aabb->max.y < vertical_cylinder.bottom.y - vertical_cylinder.height || aabb->min.y > vertical_cylinder.bottom.y) {
        //return 0;
    }

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

// We can just make this an AABB-AABB collision lmao it's broad phase anyway
int capsule_aabb_intersect(const aabb_t* aabb, capsule_t capsule) {
    // Create AABB from capsule
    const vec3_t corner1 = {
        capsule.tip.x - capsule.radius,
        capsule.tip.y,
        capsule.tip.z - capsule.radius
    };
    const vec3_t corner2 = {
        capsule.base.x + capsule.radius,
        capsule.base.y,
        capsule.base.z + capsule.radius
    };
    const aabb_t capsule_box = {
        .max = {
            scalar_max(corner1.x, corner2.x),
            scalar_max(corner1.y, corner2.y),
            scalar_max(corner1.z, corner2.z),
        },
        .min = {
            scalar_min(corner1.x, corner2.x),
            scalar_min(corner1.y, corner2.y),
            scalar_min(corner1.z, corner2.z),
        }
    };

    // Do they intersect?
    return  (capsule_box.min.x <= aabb->max.x) && (capsule_box.max.x >= aabb->min.x)
    &&      (capsule_box.min.y <= aabb->max.y) && (capsule_box.max.y >= aabb->min.y)
    &&      (capsule_box.min.z <= aabb->max.z) && (capsule_box.max.z >= aabb->min.z);
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
    WARN_IF("all barycentric coordinates ended up negative! this should be impossible", (edge0 < 0 && edge1 < 0 && edge2 < 0));
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

vec3_t find_closest_point_on_triangle_3d(vec3_t a, vec3_t b, vec3_t c, vec3_t p, scalar_t* v_out, scalar_t* w_out) {
    // Calculate vectors
    const vec3_t ab = vec3_sub(b, a);
    const vec3_t ac = vec3_sub(c, a);

    // A's dorito zone - closest point is A
    const vec3_t ap = vec3_sub(p, a);
    const scalar_t d1 = vec3_dot(ab, ap);
    const scalar_t d2 = vec3_dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return a;

    // B's dorito zone - closest point is B
    const vec3_t bp = vec3_sub(p, b);
    const scalar_t d3 = vec3_dot(ab, bp);
    const scalar_t d4 = vec3_dot(ac, bp);
    if (d3 > 0 && d4 <= d3) return b;

    // C's dorito zone - closest point is C
    const vec3_t cp = vec3_sub(p, c);
    const scalar_t d5 = vec3_dot(ab, cp);
    const scalar_t d6 = vec3_dot(ac, cp);
    if (d6 > 0 && d5 <= d6) return b;

    // AB's chonko zone - closest point is on edge AB
    const scalar_t vc = scalar_mul(d1, d4) - scalar_mul(d3, d2);
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        const scalar_t v = scalar_div(d1, (d1 - d3));
        return vec3_add(a, vec3_muls(ab, v));
    }

    // AC's chonko zone - closest point is on edge AC
    const scalar_t vb = (scalar_mul(d5, d2) - scalar_mul(d1, d6));
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        const scalar_t v = scalar_div(d2, (d2 - d6));
        return vec3_add(a, vec3_muls(ac, v));
    }

    // BC's chonko zone - closest point is on edge BC
    const scalar_t va = (scalar_mul(d3, d6) - scalar_mul(d5, d4));
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        const scalar_t v = scalar_div((d4 - d3), ((d4 - d3) + (d5 - d6)));
        return vec3_add(b, vec3_muls(vec3_sub(c, b), v));
    }

    // Otherwise the point is inside the triangle
    const scalar_t va_vb_vc = va + vb + vc;
    const scalar_t v = scalar_div(vb, va_vb_vc);
    const scalar_t w = scalar_div(vc, va_vb_vc);
    if (v_out) *v_out = v;
    if (w_out) *w_out = w;
    return vec3_add(vec3_add(a, vec3_muls(ab, v)), vec3_muls(ac, w));
}

int vertical_cylinder_triangle_intersect(collision_triangle_3d_t* triangle, vertical_cylinder_t vertical_cylinder, rayhit_t* hit) {
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
    if (triangle->normal.y == 0) {
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
        hit->triangle = triangle;
        return 1;
    }

    hit->distance = INT32_MAX;
    return 0;
}

void collision_clear_stats(void) {
    n_ray_aabb_intersects = 0;
    n_ray_triangle_intersects = 0;
    n_sphere_aabb_intersects = 0;
    n_sphere_triangle_intersects = 0;
    n_vertical_cylinder_aabb_intersects = 0;
    n_vertical_cylinder_triangle_intersects = 0;
}

int sphere_triangle_intersect_old(collision_triangle_3d_t* triangle, sphere_t sphere, rayhit_t* hit) {
    n_sphere_triangle_intersects++;
    // Find the closest point from the sphere to the triangle
    const vec3_t triangle_to_center = vec3_sub(sphere.center, triangle->v0);
    const scalar_t distance = vec3_dot(triangle_to_center, triangle->normal); WARN_IF("distance overflowed", is_infinity(distance));
    vec3_t position = vec3_sub(sphere.center, vec3_muls(triangle->normal, distance));

    // If the closest point from the sphere on the plane is too far, don't bother with all the expensive math, we will never intersect
    if (distance > sphere.radius || distance < 0) {
        hit->distance = INT32_MAX;
        return 0;
    }

    // Shift it to the right by 3 - to avoid overflow with bigger triangles at the cost of some precision
    const vec3_t v0 = vec3_shift_right(triangle->v0, 3);
    const vec3_t v1 = vec3_shift_right(triangle->v1, 3);
    const vec3_t v2 = vec3_shift_right(triangle->v2, 3);
    position = vec3_shift_right(position, 3);

    vec3_t closest_pos_on_triangle = find_closest_point_on_triangle_3d(v0, v1, v2, position, 0, 0);

    // Shift it back
    closest_pos_on_triangle = vec3_shift_left(closest_pos_on_triangle, 3);

    // Is the hit position close enough to the plane hit? (is it within the sphere?)
    vec3_t penetration_normal = vec3_sub(sphere.center, closest_pos_on_triangle);
    scalar_t distance_from_hit_squared = vec3_magnitude_squared(penetration_normal); //WARN_IF("distance_from_hit_squared overflowed", is_infinity(distance_from_hit_squared));

    if (distance_from_hit_squared >= sphere.radius_squared)
    {
        hit->distance = INT32_MAX;
        return 0;
    }
    distance_from_hit_squared = scalar_sqrt(distance_from_hit_squared);
    penetration_normal = vec3_divs(penetration_normal, distance_from_hit_squared);

    hit->position = closest_pos_on_triangle;
    hit->distance = distance_from_hit_squared;
    hit->normal = penetration_normal;
    hit->triangle = triangle;
    return 1;
}

vec3_t closest_point_on_line_segment(vec3_t a, vec3_t b, vec3_t point) {
    vec3_t ab = vec3_sub(a, b);
    scalar_t t = scalar_min(scalar_max(scalar_div(vec3_dot(vec3_sub(point, a), ab), vec3_dot(ab, ab)), 0), 1);
    return vec3_add(a, vec3_muls(ab, t));
}

int sphere_triangle_intersect(collision_triangle_3d_t* triangle, sphere_t sphere, rayhit_t* hit) {
    const vec3_t p0 = vec3_shift_right(triangle->v0, 4);
    const vec3_t p1 = vec3_shift_right(triangle->v1, 4);
    const vec3_t p2 = vec3_shift_right(triangle->v2, 4);
    const vec3_t center = vec3_shift_right(sphere.center, 4);
    const scalar_t radius = sphere.radius >> 4;
    const scalar_t radius_squared = scalar_mul(radius, radius);

    const vec3_t N = vec3_normalize(vec3_cross(vec3_sub(p1, p0), vec3_sub(p2, p0)));
    //vec3_debug(N);
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

int capsule_triangle_intersect(collision_triangle_3d_t* triangle, capsule_t capsule, rayhit_t* hit) {
    // A capsule is just a thick line, compute the normalized vector from v0 to v1
    const vec3_t line_dir = vec3_normalize(vec3_sub(capsule.tip, capsule.base));

    // Calculate distance 
    const vec3_t v0_to_base = vec3_sub(capsule.base, triangle->v0);
    scalar_t distance = vec3_dot(triangle->normal, v0_to_base);

    return 0;
}