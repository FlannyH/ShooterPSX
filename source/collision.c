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

void bvh_construct(bvh_t* bvh, const triangle_3d_t* primitives, const uint16_t n_primitives) {
    // Convert triangles from the model into collision triangles
    // todo: maybe store this mesh in the file? not sure if it's worth implementing but could be nice
    bvh->primitives = malloc(sizeof(collision_triangle_3d_t) * n_primitives);
    bvh->n_primitives = n_primitives;
    PANIC_IF("failed to allocate memory for collision model!", bvh->primitives == 0);

    for (size_t i = 0; i < n_primitives; ++i) {
        // Set position
        bvh->primitives[i].v0 = vec3_from_int32s(-(int32_t)primitives[i].v0.x << 12, -(int32_t)primitives[i].v0.y << 12, -(int32_t)primitives[i].v0.z << 12);
        bvh->primitives[i].v1 = vec3_from_int32s(-(int32_t)primitives[i].v1.x << 12, -(int32_t)primitives[i].v1.y << 12, -(int32_t)primitives[i].v1.z << 12);
        bvh->primitives[i].v2 = vec3_from_int32s(-(int32_t)primitives[i].v2.x << 12, -(int32_t)primitives[i].v2.y << 12, -(int32_t)primitives[i].v2.z << 12);

        // Calculate normal
        const vec3_t ac = vec3_sub(vec3_shift_right(bvh->primitives[i].v1, 4), vec3_shift_right(bvh->primitives[i].v0, 4));
        const vec3_t ab = vec3_sub(vec3_shift_right(bvh->primitives[i].v2, 4), vec3_shift_right(bvh->primitives[i].v0, 4));
        bvh->primitives[i].normal = vec3_cross(ac, ab);
        WARN_IF("bvh normal calculation return infinity", (bvh->primitives[i].normal.x.raw | bvh->primitives[i].normal.y.raw | bvh->primitives[i].normal.z.raw) == INT32_MAX);
        bvh->primitives[i].normal = vec3_normalize(bvh->primitives[i].normal);

        // Calculate center
        bvh->primitives[i].center = bvh->primitives[i].v0;
        bvh->primitives[i].center = vec3_add(bvh->primitives[i].center, bvh->primitives[i].v1);
        bvh->primitives[i].center = vec3_add(bvh->primitives[i].center, bvh->primitives[i].v2);
        bvh->primitives[i].center = vec3_mul(bvh->primitives[i].center, vec3_from_scalar(scalar_from_int32(1365)));

        // Precalculate triangle collision variables
        vec3_t c = vec3_sub(bvh->primitives[i].v2, bvh->primitives[i].v0);
        vec3_t b = vec3_sub(bvh->primitives[i].v1, bvh->primitives[i].v0);
        int n_shifts_edge = 0;

        // Make sure we don't overflow - we can do expensive stuff here so we don't have to do that later :)
        while (vec3_dot(c, c).raw == INT32_MAX || vec3_dot(b, c).raw == INT32_MAX || vec3_dot(b, b).raw == INT32_MAX) {
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
        while (scalar_mul(cc, bb).raw == INT32_MAX || scalar_mul(bc, bc).raw == INT32_MAX) {
            cc = scalar_shift_right(cc, 1);
            bc = scalar_shift_right(bc, 1);
            bb = scalar_shift_right(bb, 1);
            ++n_shifts_dot;
        }

        // Shift some more for good measure
        cc = scalar_shift_right(cc, 4);
        bc = scalar_shift_right(bc, 4);
        bb = scalar_shift_right(bb, 4);
        n_shifts_dot += 4;

        //Get barycentric coordinates
        const scalar_t cc_bb = scalar_mul(cc, bb); 
        const scalar_t bc_bc = scalar_mul(bc, bc); 
        const scalar_t d = scalar_sub(cc_bb, bc_bc);

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
    scalar_t split_pos = scalar_from_int32(0);

    const vec3_t size = vec3_sub(node->bounds.max, node->bounds.min);

    if (size.x.raw > size.y.raw 
        && size.x.raw > size.z.raw)
    {
        split_axis = axis_x;
        split_pos = scalar_add(node->bounds.max.x, node->bounds.min.x);
        split_pos.raw = split_pos.raw >> 1;
    }

    if (size.y.raw > size.x.raw
        && size.y.raw > size.z.raw)
    {
        split_axis = axis_y;
        split_pos = scalar_add(node->bounds.max.y, node->bounds.min.y);
        split_pos.raw = split_pos.raw >> 1;
    }

    if (size.z.raw > size.x.raw
        && size.z.raw > size.y.raw)
    {
        split_axis = axis_z;
        split_pos = scalar_add(node->bounds.max.z, node->bounds.min.z);
        split_pos.raw = split_pos.raw >> 1;
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

void handle_node_intersection_ray(bvh_t* self, bvh_node_t* current_node, ray_t ray, rayhit_t* hit, int rec_depth) {
    // Intersect current node
    if (ray_aabb_intersect(&current_node->bounds, ray))
    {
        // If it's a leaf
        if (current_node->is_leaf)
        {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = { 0 };
            sub_hit.distance.raw = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (ray_triangle_intersect(&self->primitives[self->indices[i]], ray, &sub_hit)) {
                    // If lowest distance
                    if (sub_hit.distance.raw < hit->distance.raw && sub_hit.distance.raw >= 0)
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
            sub_hit.distance.raw = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (sphere_triangle_intersect(&self->primitives[self->indices[i]], sphere, &sub_hit)) {
                    // When colliding with a surface, you would want 
                    const vec3_t player_to_surface = vec3_sub(sphere.center, sub_hit.position);
                    sub_hit.distance_along_normal = vec3_dot(sub_hit.normal, player_to_surface);

                    if (sub_hit.distance_along_normal.raw <= sphere.radius.raw)
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
            sub_hit.distance.raw = 0;
            for (int i = current_node->left_first; i < current_node->left_first + current_node->primitive_count; i++)
            {
                // If hit
                if (vertical_cylinder_triangle_intersect(&self->primitives[self->indices[i]], vertical_cylinder, &sub_hit)) {
                    // If lowest distance
                    if (sub_hit.distance.raw < hit->distance.raw && sub_hit.distance.raw >= 0)
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


int bvh_intersect_ray(bvh_t* self, const ray_t ray, rayhit_t* hit) {
    hit->distance = scalar_from_int32(INT32_MAX);
    handle_node_intersection_ray(self, self->root, ray, hit, 0);
}

void bvh_intersect_sphere(bvh_t* bvh, const sphere_t ray, rayhit_t* hit) {
    hit->distance = scalar_from_int32(INT32_MAX);
    handle_node_intersection_sphere(bvh, bvh->root, ray, hit, 0);
}

void bvh_intersect_vertical_cylinder(bvh_t* bvh, vertical_cylinder_t ray, rayhit_t* hit) {
    hit->distance = scalar_from_int32(INT32_MAX);
    handle_node_intersection_vertical_cylinder(bvh, bvh->root, ray, hit, 0);
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
        center.x.raw = center.x.raw >> 1;
        center.y.raw = center.y.raw >> 1;
        center.z.raw = center.z.raw >> 1;
        const scalar_t* center_points = (scalar_t*)&center;

        // If the current primitive's center's <axis>-component is greated than the pivot's <axis>-component
        if (center_points[(size_t)axis].raw > pivot.raw && (j != i))
        {
            // Move the primitive's index to the first partition of this node
            bvh_swap_primitives(&bvh->indices[i], &bvh->indices[j]);
            i++;
        }
    }
    *split_index = i;
}

void bvh_from_mesh(bvh_t* bvh, const mesh_t* mesh) {
    bvh_construct(bvh, (triangle_3d_t*)mesh->vertices, (uint16_t)mesh->n_vertices / 3);
}

void bvh_from_model(bvh_t* bvh, const model_t* mesh) {
    // Get total number of vertices
    uint32_t n_verts = 0;
    for (uint32_t i = 0; i < mesh->n_meshes; ++i) {
        n_verts += mesh->meshes[i].n_vertices;
    }

    // Allocate a buffer for the vertices
    triangle_3d_t* triangles = malloc(n_verts / 3 * sizeof(triangle_3d_t));

    // Copy all the meshes into it sequentially
    unsigned long long offset = 0;
    for (uint32_t i = 0; i < mesh->n_meshes; ++i) {
        const size_t bytes_to_copy = mesh->meshes[i].n_vertices * sizeof(vertex_3d_t);
        const size_t verts_to_copy = mesh->meshes[i].n_vertices / 3;
        memcpy(triangles + offset, mesh->meshes[i].vertices, bytes_to_copy);
        offset += verts_to_copy;
    }

    // Construct the BVH
    bvh_construct(bvh, triangles, n_verts / 3);
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

int ray_aabb_intersect(const aabb_t* aabb, ray_t ray) {
    n_ray_aabb_intersects++;
    scalar_t tx1 = scalar_mul(scalar_sub(aabb->min.x, ray.position.x), ray.inv_direction.x);
    scalar_t tx2 = scalar_mul(scalar_sub(aabb->max.x, ray.position.x), ray.inv_direction.x);

    scalar_t tmin = scalar_min(tx1, tx2);
    scalar_t tmax = scalar_max(tx1, tx2);

    scalar_t ty1 = scalar_mul(scalar_sub(aabb->min.y, ray.position.y), ray.inv_direction.y);
    scalar_t ty2 = scalar_mul(scalar_sub(aabb->max.y, ray.position.y), ray.inv_direction.y);

    tmin = scalar_max(scalar_min(ty1, ty2), tmin);
    tmax = scalar_min(scalar_max(ty1, ty2), tmax);

    scalar_t tz1 = scalar_mul(scalar_sub(aabb->min.z, ray.position.z), ray.inv_direction.z);
    scalar_t tz2 = scalar_mul(scalar_sub(aabb->max.z, ray.position.z), ray.inv_direction.z);

    tmin = scalar_max(scalar_min(tz1, tz2), tmin);
    tmax = scalar_min(scalar_max(tz1, tz2), tmax);
    
    return tmax.raw >= tmin.raw && tmax.raw >= 0;
}

int ray_triangle_intersect(const collision_triangle_3d_t* triangle, ray_t ray, rayhit_t* hit) {
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
    if (distance_min.raw < scalar_mul(ray.length, ray.length).raw) {

        hit->distance.raw = INT32_MAX;
        return 0;
    }

    // Calculate normal
    const vec3_t normal_normalized = vec3_neg(triangle->normal);
    const scalar_t dot_dir_nrm = vec3_dot(ray.direction, normal_normalized);

    // If the ray is perfectly parallel to the plane, we did not hit it
    if (dot_dir_nrm.raw == 0)
    {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    //Get distance to intersection point
    vec3_t temp = vec3_sub(triangle->center, ray.position);
    scalar_t distance = vec3_dot(temp, normal_normalized);
    distance = scalar_div(distance, dot_dir_nrm);

    // If the distance is negative, the plane is behind the ray origin, so we did not hit it
    if (distance.raw < 0) {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    //Get position
    vec3_t position = vec3_add(ray.position, vec3_mul(ray.direction, vec3_from_scalar(distance)));

    // Shift it to the right by 4 - to avoid overflow with bigger triangles at the cost of some precision
    v0 = vec3_shift_right(v0, 4);
    v1 = vec3_shift_right(v1, 4);
    v2 = vec3_shift_right(v2, 4);

    // Get edges
    vec3_t c = vec3_sub(v2, v0);
    vec3_t b = vec3_sub(v1, v0);

    // Get more vectors
    vec3_t p = vec3_sub(position, triangle->v0);
    p = vec3_shift_right(p, 4);

    //Get dots
    const scalar_t cc = scalar_shift_right(vec3_dot(c, c), 6); WARN_IF("vec3_dot(c, c) overflowed", is_infinity(vec3_dot(c, c)));
    const scalar_t bc = scalar_shift_right(vec3_dot(b, c), 6); WARN_IF("vec3_dot(b, c) overflowed", is_infinity(vec3_dot(b, c)));
    const scalar_t pc = scalar_shift_right(vec3_dot(c, p), 6); WARN_IF("vec3_dot(c, p) overflowed", is_infinity(vec3_dot(c, p)));
    const scalar_t bb = scalar_shift_right(vec3_dot(b, b), 6); WARN_IF("vec3_dot(b, b) overflowed", is_infinity(vec3_dot(b, b)));
    const scalar_t pb = scalar_shift_right(vec3_dot(b, p), 6); WARN_IF("vec3_dot(b, p) overflowed", is_infinity(vec3_dot(b, p)));

    //Get barycentric coordinates
    const scalar_t cc_bb = scalar_mul(cc, bb); WARN_IF("cc_bb overflowed", cc_bb.raw == INT32_MAX || cc_bb.raw == -INT32_MAX);
    const scalar_t bc_bc = scalar_mul(bc, bc); WARN_IF("bc_bc overflowed", bc_bc.raw == INT32_MAX || bc_bc.raw == -INT32_MAX);
    const scalar_t bb_pc = scalar_mul(bb, pc); WARN_IF("bb_pc overflowed", bb_pc.raw == INT32_MAX || bb_pc.raw == -INT32_MAX);
    const scalar_t bc_pb = scalar_mul(bc, pb); WARN_IF("bc_pb overflowed", bc_pb.raw == INT32_MAX || bc_pb.raw == -INT32_MAX);
    const scalar_t cc_pb = scalar_mul(cc, pb); WARN_IF("cc_pb overflowed", cc_pb.raw == INT32_MAX || cc_pb.raw == -INT32_MAX);
    const scalar_t bc_pc = scalar_mul(bc, pc); WARN_IF("bc_pc overflowed", bc_pc.raw == INT32_MAX || bc_pc.raw == -INT32_MAX);
    const scalar_t d = scalar_sub(cc_bb, bc_bc);
    scalar_t u = scalar_sub(bb_pc, bc_pb);
    scalar_t v = scalar_sub(cc_pb, bc_pc);
    u = scalar_div(u, d);
    v = scalar_div(v, d);

    // If the point is inside the triangle, store the this result - we shift the 4 bits back out here too
    if ((u.raw >= 0) && (v.raw >= 0) && ((u.raw + v.raw) <= 4096 << 4))
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
        sphere.center.x.raw >= aabb->min.x.raw && 
        sphere.center.x.raw <= aabb->max.x.raw && 
        sphere.center.y.raw >= aabb->min.y.raw && 
        sphere.center.y.raw <= aabb->max.y.raw && 
        sphere.center.z.raw >= aabb->min.z.raw && 
        sphere.center.z.raw <= aabb->max.z.raw
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
    return distance_squared.raw <= sphere.radius_squared.raw;
}

// Approximation!
int vertical_cylinder_aabb_intersect(const aabb_t* aabb, const vertical_cylinder_t vertical_cylinder) {
    n_vertical_cylinder_aabb_intersects++;
    // Exit early if the AABB and vertical cylinder do not overlap on the Y-axis
    if (aabb->max.y.raw < vertical_cylinder.bottom.y.raw - vertical_cylinder.height.raw || aabb->min.y.raw > vertical_cylinder.bottom.y.raw) {
        //return 0;
    }

    // The rest can be done in 2D
    // Create a new AABB that's extended by the cylinder's radius. This is an approximation, but we use AABB's for BVH traversal only so it's fine
    const scalar_t min_x = scalar_sub(aabb->min.x, vertical_cylinder.radius);
    const scalar_t max_x = scalar_add(aabb->max.x, vertical_cylinder.radius);
    const scalar_t min_z = scalar_sub(aabb->min.z, vertical_cylinder.radius);
    const scalar_t max_z = scalar_add(aabb->max.z, vertical_cylinder.radius);
    const scalar_t point_x = vertical_cylinder.bottom.x;
    const scalar_t point_z = vertical_cylinder.bottom.z;

    // Check if it's inside
    return(
        point_x.raw >= min_x.raw &&
        point_x.raw <= max_x.raw &&
        point_z.raw >= min_z.raw &&
        point_z.raw <= max_z.raw
    );
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
    progress_along_edge.raw = int32_clamp(progress_along_edge.raw, 0, 4096);
    return progress_along_edge;
}

// for some reason the function used for the 3d triangles doesn't work in 2d? so i made my own instead
// u_out is 1.0 p lies on v0, and v_out is 1.0 if p lies on v1
vec2_t find_closest_point_on_triangle_2d(vec2_t v0, vec2_t v1, vec2_t v2, vec2_t p, scalar_t* u_out, scalar_t* v_out) {
    // This is what we hope to return after this
    vec2_t closest_point;

    // Calculate edge0
    vec2_t v1_p = vec2_sub(p, v1);
    vec2_t v1_v2 = vec2_sub(v2, v1);
    scalar_t edge0 = vec2_cross(v1_p, v1_v2);

    // Calculate edge1
    vec2_t v2_p = vec2_sub(p, v2);
    vec2_t v2_v0 = vec2_sub(v0, v2);
    scalar_t edge1 = vec2_cross(v2_p, v2_v0);

    // Calculate edge2
    vec2_t v0_p = vec2_sub(p, v0);
    vec2_t v0_v1 = vec2_sub(v1, v0);
    scalar_t edge2 = vec2_cross(v0_p, v0_v1);

    // Are we inside triangle?
    if (edge0.raw >= 0 && edge1.raw >= 0 && edge2.raw >= 0) {
        // Normalize barycoords
        vec2_t v1_v0 = vec2_sub(v0, v1);
        vec2_t v1_v2 = vec2_sub(v2, v1);
        scalar_t area = vec2_cross(v1_v0, v1_v2);
        *u_out = scalar_div(edge0, area);
        *v_out = scalar_div(edge1, area);

        // Return P as is
        return p;
    }

    //It's impossible for all the numbers to be negative
    WARN_IF("all barycentric coordinates ended up negative! this should be impossible", (edge0.raw < 0 && edge1.raw < 0 && edge2.raw < 0));
    if ((edge0.raw < 0 && edge1.raw < 0 && edge2.raw < 0)) {
        vec2_t error;
        error.x.raw = INT32_MAX;
        error.y.raw = 1;
    }

    // Otherwise, project onto the first negative edge we find
    scalar_t progress;

    // A = v1, B = v2
    if (edge0.raw < 0) {
        progress = get_progress_of_p_on_ab(v1, v2, p);
        closest_point = vec2_add(v1, vec2_mul(vec2_sub(v2, v1), vec2_from_scalar(progress)));
        u_out->raw = 0;
        v_out->raw = 4096 - progress.raw;

    }
    // A = v2, B = v0
    else if (edge1.raw < 0) {
        progress = get_progress_of_p_on_ab(v2, v0, p);
        closest_point = vec2_add(v2, vec2_mul(vec2_sub(v0, v2), vec2_from_scalar(progress)));
        u_out->raw = progress.raw;
        v_out->raw = 0;
    }
    // A = v0, B = v1
    else /*if (edge2.raw < 0)*/ {
        progress = get_progress_of_p_on_ab(v0, v1, p);
        closest_point = vec2_add(v0, vec2_mul(vec2_sub(v1, v0), vec2_from_scalar(progress)));
        u_out->raw = 4096 - progress.raw;
        v_out->raw = progress.raw;
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
    if (d1.raw <= 0 && d2.raw <= 0) return a;

    // B's dorito zone - closest point is B
    const vec3_t bp = vec3_sub(p, b);
    const scalar_t d3 = vec3_dot(ab, bp);
    const scalar_t d4 = vec3_dot(ac, bp);
    if (d3.raw > 0 && d4.raw <= d3.raw) return b;

    // C's dorito zone - closest point is C
    const vec3_t cp = vec3_sub(p, c);
    const scalar_t d5 = vec3_dot(ab, cp);
    const scalar_t d6 = vec3_dot(ac, cp);
    if (d6.raw > 0 && d5.raw <= d6.raw) return b;

    // AB's chonko zone - closest point is on edge AB
    const scalar_t vc = scalar_sub(scalar_mul(d1, d4), scalar_mul(d3, d2));
    if (vc.raw <= 0 && d1.raw >= 0 && d3.raw <= 0) {
        const scalar_t v = scalar_div(d1, scalar_sub(d1, d3));
        return vec3_add(a, vec3_mul(vec3_from_scalar(v), ab));
    }

    // AC's chonko zone - closest point is on edge AC
    const scalar_t vb = scalar_sub(scalar_mul(d5, d2), scalar_mul(d1, d6));
    if (vb.raw <= 0 && d2.raw >= 0 && d6.raw <= 0) {
        const scalar_t v = scalar_div(d2, scalar_sub(d2, d6));
        return vec3_add(a, vec3_mul(vec3_from_scalar(v), ac));
    }

    // BC's chonko zone - closest point is on edge BC
    const scalar_t va = scalar_sub(scalar_mul(d3, d6), scalar_mul(d5, d4));
    if (va.raw <= 0 && scalar_sub(d4, d3).raw >= 0 && scalar_sub(d5, d6).raw >= 0) {
        const scalar_t v = scalar_div(scalar_sub(d4, d3), scalar_add(scalar_sub(d4, d3), scalar_sub(d5, d6)));
        return vec3_add(b, vec3_mul(vec3_from_scalar(v), vec3_sub(c, b)));
    }

    // Otherwise the point is inside the triangle
    const scalar_t va_vb_vc = { .raw = va.raw + vb.raw + vc.raw };
    const scalar_t v = scalar_div(vb, va_vb_vc);
    const scalar_t w = scalar_div(vc, va_vb_vc);
    if (v_out) *v_out = v;
    if (w_out) *w_out = w;
    return vec3_add(vec3_add(a, vec3_mul(vec3_from_scalar(v), ab)), vec3_mul(vec3_from_scalar(w), ac));
}

int vertical_cylinder_triangle_intersect(collision_triangle_3d_t* triangle, vertical_cylinder_t vertical_cylinder, rayhit_t* hit) {
    n_vertical_cylinder_triangle_intersects++;
    // Since this is only used for ground collisions, we can ignore surfaces that don't face up
    if (triangle->normal.y.raw <= 0) {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    // Project everything into 2D top-down
    vec2_t v0 = { triangle->v0.x, triangle->v0.z };
    vec2_t v1 = { triangle->v1.x, triangle->v1.z };
    vec2_t v2 = { triangle->v2.x, triangle->v2.z };
    vec2_t position = { vertical_cylinder.bottom.x, vertical_cylinder.bottom.z };

    // Find closest point
    scalar_t u, v, w;
    const vec2_t closest_pos_on_triangle = find_closest_point_on_triangle_2d(v0, v1, v2, position, &u, &v);

    // If closest point returned a faulty value, ignore it
    if (closest_pos_on_triangle.x.raw == INT32_MAX) {
        hit->distance.raw = INT32_MAX;
        return 0;
    }
    w.raw = 4096 - u.raw - v.raw;

    // We found the closest point! Does the circle intersect it?
    scalar_t distance_to_closest_point = vec2_magnitude_squared(vec2_sub(position, closest_pos_on_triangle));
    if (distance_to_closest_point.raw >= vertical_cylinder.radius_squared.raw) {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    // It does! calculate the Y coordinate
    vec3_t closest_pos_3d;
    closest_pos_3d.x = closest_pos_on_triangle.x;
    closest_pos_3d.z = closest_pos_on_triangle.y;
    closest_pos_3d.y = triangle->v0.y;
    closest_pos_3d.y = scalar_add(closest_pos_3d.y, scalar_mul(v, scalar_sub(triangle->v1.y, triangle->v0.y)));
    closest_pos_3d.y = scalar_add(closest_pos_3d.y, scalar_mul(w, scalar_sub(triangle->v2.y, triangle->v0.y)));

    // Is this Y coordinate within the cylinder's range?
    const scalar_t min = vertical_cylinder.bottom.y;
    const scalar_t max = scalar_add(min, vertical_cylinder.height);
    if (closest_pos_3d.y.raw >= min.raw && closest_pos_3d.y.raw <= max.raw) {
        // Return this point
        hit->position = closest_pos_3d;
        hit->normal = triangle->normal;
        hit->triangle = triangle;
        hit->distance = scalar_sqrt(distance_to_closest_point);
        return 1;
    }

    hit->distance.raw = INT32_MAX;
    return 0;
}

void collision_clear_stats() {
    n_ray_aabb_intersects = 0;
    n_ray_triangle_intersects = 0;
    n_sphere_aabb_intersects = 0;
    n_sphere_triangle_intersects = 0;
    n_vertical_cylinder_aabb_intersects = 0;
    n_vertical_cylinder_triangle_intersects = 0;
}

int sphere_triangle_intersect(const collision_triangle_3d_t* triangle, sphere_t sphere, rayhit_t* hit) {
    n_sphere_triangle_intersects++;
    // Find the closest point from the sphere to the triangle
    const vec3_t triangle_to_center = vec3_sub(sphere.center, triangle->v0);
    const scalar_t distance = vec3_dot(triangle_to_center, triangle->normal); WARN_IF("distance overflowed", is_infinity(distance));
    vec3_t position = vec3_sub(sphere.center, vec3_mul(triangle->normal, vec3_from_scalar(distance)));

    // If the closest point from the sphere on the plane is too far, don't bother with all the expensive math, we will never intersect
    if (distance.raw > sphere.radius.raw && distance.raw < -sphere.radius.raw) {
        hit->distance.raw = INT32_MAX;
        return 0;
    }
;
    // Shift it to the right by 3 - to avoid overflow with bigger triangles at the cost of some precision
    vec3_t v0 = vec3_shift_right(triangle->v0, 3);
    vec3_t v1 = vec3_shift_right(triangle->v1, 3);
    vec3_t v2 = vec3_shift_right(triangle->v2, 3);
    position = vec3_shift_right(position, 3);

    vec3_t closest_pos_on_triangle = find_closest_point_on_triangle_3d(v0, v1, v2, position, 0, 0);

    // Shift it back
    closest_pos_on_triangle = vec3_shift_left(closest_pos_on_triangle, 3);
    position = vec3_shift_left(position, 3);

    // Is the hit position close enough to the plane hit? (is it within the sphere?)
    const scalar_t distance_from_hit_squared = vec3_magnitude_squared(vec3_sub(sphere.center, closest_pos_on_triangle)); //WARN_IF("distance_from_hit_squared overflowed", is_infinity(distance_from_hit_squared));

    if (distance_from_hit_squared.raw >= sphere.radius_squared.raw)
    {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    hit->position = closest_pos_on_triangle;
    hit->distance = scalar_sqrt(distance_from_hit_squared);
    hit->normal = triangle->normal;
    hit->triangle = triangle;
    return 1;

}
