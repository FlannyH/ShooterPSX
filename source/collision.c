#include "collision.h"

#include <stdlib.h>
#include <string.h>

#include "mesh.h"
#include "renderer.h"

void bvh_construct(bvh_t* self, const triangle_3d_t* primitives, const uint16_t n_primitives) {
    // Convert triangles from the model into collision triangles
    // todo: maybe store this mesh in the file? not sure if it's worth implementing but could be nice
    self->primitives = malloc(sizeof(collision_triangle_3d_t) * n_primitives);
    self->n_primitives = n_primitives;

    for (size_t i = 0; i < n_primitives; ++i) {
        // Set position
        self->primitives[i].v0 = vec3_from_int32s(-(int32_t)primitives[i].v0.x << 12, -(int32_t)primitives[i].v0.y << 12, -(int32_t)primitives[i].v0.z << 12);
        self->primitives[i].v1 = vec3_from_int32s(-(int32_t)primitives[i].v1.x << 12, -(int32_t)primitives[i].v1.y << 12, -(int32_t)primitives[i].v1.z << 12);
        self->primitives[i].v2 = vec3_from_int32s(-(int32_t)primitives[i].v2.x << 12, -(int32_t)primitives[i].v2.y << 12, -(int32_t)primitives[i].v2.z << 12);

        // Calculate normal
        // todo: maybe check whether this normal is pointing the right way
        const vec3_t ac = vec3_sub(vec3_shift_right(self->primitives[i].v1, 4), vec3_shift_right(self->primitives[i].v0, 4));
        const vec3_t ab = vec3_sub(vec3_shift_right(self->primitives[i].v2, 4), vec3_shift_right(self->primitives[i].v0, 4));
        self->primitives[i].normal = vec3_cross(ac, ab);
        self->primitives[i].normal = vec3_normalize(self->primitives[i].normal);

        // Calculate center
        self->primitives[i].center = self->primitives[i].v0;
        self->primitives[i].center = vec3_add(self->primitives[i].center, self->primitives[i].v1);
        self->primitives[i].center = vec3_add(self->primitives[i].center, self->primitives[i].v2);
        self->primitives[i].center = vec3_mul(self->primitives[i].center, vec3_from_scalar(scalar_from_int32(1365)));
    }


    // Create index array
    self->indices = malloc(sizeof(uint16_t) * n_primitives);
    for (int i = 0; i < n_primitives; i++)
    {
        self->indices[i] = i;
    }

    // Create root node
    self->nodes = malloc(sizeof(bvh_node_t) * n_primitives * 2);
    self->node_pointer = 2;
    self->root = &self->nodes[0]; // todo: maybe we can just hard code the root to be index 0 and skip this indirection?
    self->root->left_first = 0;
    self->root->primitive_count = n_primitives;
    bvh_subdivide(self, self->root, 0);
}

aabb_t bvh_get_bounds(const bvh_t* self, const uint16_t first, const uint16_t count)
{
    aabb_t result;
    result.max = vec3_from_int32s(INT32_MIN, INT32_MIN, INT32_MIN);
    result.min = vec3_from_int32s(INT32_MAX, INT32_MAX, INT32_MAX);
    for (int i = 0; i < count; i++)
    {
        const aabb_t curr_primitive_bounds = collision_triangle_get_bounds(&self->primitives[self->indices[first + i]]);

        result.min = vec3_min(result.min, curr_primitive_bounds.min);
        result.max = vec3_max(result.max, curr_primitive_bounds.max);
    }
    return result;
}

void bvh_subdivide(bvh_t* self, bvh_node_t* node, const int recursion_depth) {
    //Determine AABB for primitives in array
    node->bounds = bvh_get_bounds(self, node->left_first, node->primitive_count);

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
    bvh_partition(self, split_axis, split_pos, node->left_first, node->primitive_count, &split_index);

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
    node->left_first = self->node_pointer;
    self->node_pointer += 2;

    //Start
    self->nodes[node->left_first + 0].left_first = start_index;
    self->nodes[node->left_first + 1].left_first = split_index;

    //Count
    self->nodes[node->left_first + 0].primitive_count = split_index - start_index;
    self->nodes[node->left_first + 1].primitive_count = start_index + node->primitive_count - split_index;

    bvh_subdivide(self, &self->nodes[node->left_first + 0], recursion_depth + 1);
    bvh_subdivide(self, &self->nodes[node->left_first + 1], recursion_depth + 1);

    node->is_leaf = 0;
}

void handle_node_intersection_ray(bvh_t* self, bvh_node_t* current_node, ray_t ray, rayhit_t* hit, int rec_depth) {
    // Intersect current node
    if (ray_aabb_intersect(&current_node->bounds, ray))
    {
        pixel32_t red = { 255,0,0,255 };
        transform_t id_transform = { {0,0,0},{0,0,0}, {-4096, -4096, -4096} };
        // If it's a leaf
        if (current_node->is_leaf)
        {
            // Intersect all triangles attached to it
            rayhit_t sub_hit = {0};
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

void handle_node_intersection_sphere(bvh_t* self, bvh_node_t* current_node, sphere_t sphere, rayhit_t* hit, int rec_depth) {
    // Intersect current node
    if (sphere_aabb_intersect(&current_node->bounds, sphere))
    {
        pixel32_t c = {
            255,
            rec_depth * 32,
            rec_depth * 32,
            255
        };
        transform_t id_transform = { {0,0,0},{0,0,0}, {4096, 4096, 4096} };
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
        handle_node_intersection_sphere(self, &self->nodes[current_node->left_first + 0], sphere, hit, rec_depth + 1);
        handle_node_intersection_sphere(self, &self->nodes[current_node->left_first + 1], sphere, hit, rec_depth + 1);
    }
}

int curr_hit_index = 0;
void handle_node_intersection_spheres(bvh_t* self, bvh_node_t* current_node, sphere_t sphere, rayhit_t* hit, int rec_depth) {
    // Intersect current node
    if (sphere_aabb_intersect(&current_node->bounds, sphere))
    {
        pixel32_t c = {
            255,
            rec_depth * 32,
            rec_depth * 32,
            255
        };
        transform_t id_transform = { {0,0,0},{0,0,0}, {4096, 4096, 4096} };
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
                    // If lowest distance
                    if (sub_hit.distance.raw >= 0)
                    {
                        // Copy the hit info into the output hit for the BVH traversal
                        memcpy(&hit[curr_hit_index], &sub_hit, sizeof(rayhit_t));
                        hit[curr_hit_index].triangle = &self->primitives[self->indices[i]];
                        curr_hit_index++;
                        printf("hit!\n");
                    }
                }
            }
            return;
        }

        //Otherwise, intersect child nodes
        handle_node_intersection_spheres(self, &self->nodes[current_node->left_first + 0], sphere, hit, rec_depth + 1);
        handle_node_intersection_spheres(self, &self->nodes[current_node->left_first + 1], sphere, hit, rec_depth + 1);
    }
}

void bvh_intersect_ray(bvh_t* self, const ray_t ray, rayhit_t* hit) {
    hit->distance = scalar_from_int32(INT32_MAX);
    handle_node_intersection_ray(self, self->root, ray, hit, 0);
}

void bvh_intersect_sphere(bvh_t* self, sphere_t ray, rayhit_t* hit) {
    hit->distance = scalar_from_int32(INT32_MAX);
    handle_node_intersection_sphere(self, self->root, ray, hit, 0);
}

int bvh_intersect_spheres(bvh_t* self, sphere_t ray, rayhit_t* hit, int n_max_hits) {
    curr_hit_index = 0;
    for (int i = 0; i < n_max_hits; i++) {
        hit[i].distance = scalar_from_int32(INT32_MAX);
    }
    handle_node_intersection_spheres(self, self->root, ray, hit, 0);
    return curr_hit_index;
}

void bvh_swap_primitives(uint16_t* a, uint16_t* b) {
    const int tmp = *a;
    *a = *b;
    *b = tmp;
}

void bvh_partition(const bvh_t* self, const axis_t axis, scalar_t pivot, const uint16_t start, const uint16_t count, uint16_t* split_index) {
    int i = start;
    for (int j = start; j < start + count; j++)
    {
        // Get min and max of the axis we want
        const aabb_t bounds = collision_triangle_get_bounds(&self->primitives[self->indices[j]]);

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
            bvh_swap_primitives(&self->indices[i], &self->indices[j]);
            i++;
        }
    }
    *split_index = i;
}

void bvh_from_mesh(bvh_t* self, const mesh_t* mesh) {
    bvh_construct(self, (triangle_3d_t*)mesh->vertices, (uint16_t)mesh->n_vertices / 3);
}

void bvh_from_model(bvh_t* self, const model_t* mesh) {
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
    bvh_construct(self, triangles, n_verts / 3);
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

void bvh_debug_draw(const bvh_t* self, const int min_depth, const int max_depth, const pixel32_t color) {
    debug_draw(self, self->root, min_depth, max_depth, 0, color);
}

int ray_aabb_intersect(const aabb_t* self, ray_t ray) {
    scalar_t tx1 = scalar_mul(scalar_sub(self->min.x, ray.position.x), ray.inv_direction.x);
    scalar_t tx2 = scalar_mul(scalar_sub(self->max.x, ray.position.x), ray.inv_direction.x);

    scalar_t tmin = scalar_min(tx1, tx2);
    scalar_t tmax = scalar_max(tx1, tx2);

    scalar_t ty1 = scalar_mul(scalar_sub(self->min.y, ray.position.y), ray.inv_direction.y);
    scalar_t ty2 = scalar_mul(scalar_sub(self->max.y, ray.position.y), ray.inv_direction.y);

    tmin = scalar_max(scalar_min(ty1, ty2), tmin);
    tmax = scalar_min(scalar_max(ty1, ty2), tmax);

    scalar_t tz1 = scalar_mul(scalar_sub(self->min.z, ray.position.z), ray.inv_direction.z);
    scalar_t tz2 = scalar_mul(scalar_sub(self->max.z, ray.position.z), ray.inv_direction.z);

    tmin = scalar_max(scalar_min(tz1, tz2), tmin);
    tmax = scalar_min(scalar_max(tz1, tz2), tmax);
    
    return tmax.raw >= tmin.raw;
}

int ray_triangle_intersect(const collision_triangle_3d_t* self, ray_t ray, rayhit_t* hit) {
    //Get vectors
    vec3_t v0 = self->v0;
    vec3_t v1 = self->v1;
    vec3_t v2 = self->v2;

    // Shift it to the right by 4 - to avoid overflow with bigger triangles at the cost of some precision
    v0.x.raw >>= 4;
    v0.y.raw >>= 4;
    v0.z.raw >>= 4;
    v1.x.raw >>= 4;
    v1.y.raw >>= 4;
    v1.z.raw >>= 4;
    v2.x.raw >>= 4;
    v2.y.raw >>= 4;
    v2.z.raw >>= 4;

    // Get edges
    vec3_t c = vec3_sub(v2, v0);
    vec3_t b = vec3_sub(v1, v0);

    // calculate normal
    const vec3_t normal_normalized = vec3_neg(self->normal);
    const scalar_t dot_dir_nrm = vec3_dot(ray.direction, normal_normalized);

    // If the ray is perfectly parallel to the triangle, we did not hit it
    if (dot_dir_nrm.raw == 0)
    {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    //Get distance to intersection point
    vec3_t temp = vec3_sub(self->center, ray.position);
    scalar_t distance = vec3_dot(temp, normal_normalized);
    distance = scalar_div(distance, dot_dir_nrm);

    // If the distance is negative, the triangle is behind the ray origin, so we did not hit it
    if (distance.raw < 0) {
        hit->distance.raw = INT32_MAX;
        return 0;
    }

    //Get position
    vec3_t position = vec3_add(ray.position, vec3_mul(ray.direction, vec3_from_scalar(distance)));

    // Get more vectors
    vec3_t p = vec3_sub(position, self->v0);

    //Get dots
    const scalar_t cc = vec3_dot(c, c);
    const scalar_t bc = vec3_dot(b, c);
    const scalar_t pc = vec3_dot(c, p);
    const scalar_t bb = vec3_dot(b, b);
    const scalar_t pb = vec3_dot(b, p);

    //Get barycentric coordinates
    const scalar_t cc_bb = scalar_mul(cc, bb);
    const scalar_t bc_bc = scalar_mul(bc, bc);
    const scalar_t bb_pc = scalar_mul(bb, pc);
    const scalar_t bc_pb = scalar_mul(bc, pb);
    const scalar_t cc_pb = scalar_mul(cc, pb);
    const scalar_t bc_pc = scalar_mul(bc, pc);
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
        hit->normal = normal_normalized;
        hit->triangle = self;
        return 1;
    }
    return 0;
}

int sphere_aabb_intersect(const aabb_t* self, sphere_t sphere) {
    // First check if the center is inside the AABB
    if (
        sphere.center.x.raw >= self->min.x.raw && 
        sphere.center.x.raw <= self->max.x.raw && 
        sphere.center.y.raw >= self->min.y.raw && 
        sphere.center.y.raw <= self->max.y.raw && 
        sphere.center.z.raw >= self->min.z.raw && 
        sphere.center.z.raw <= self->max.z.raw
    ) {
        return 1;
    }

    // Find the point on the AABB that's closest to the sphere's center
    const vec3_t closest_point = {
        scalar_max(self->min.x, scalar_min(sphere.center.x, self->max.x)),
        scalar_max(self->min.y, scalar_min(sphere.center.y, self->max.y)),
        scalar_max(self->min.z, scalar_min(sphere.center.z, self->max.z))
    };

    // Get the distance from that point to the sphere
    const vec3_t sphere_center_to_closest_point = vec3_sub(sphere.center, closest_point);
    const scalar_t distance_squared = vec3_magnitude_squared(sphere_center_to_closest_point);
    return distance_squared.raw <= sphere.radius_squared.raw;
}

int sphere_triangle_intersect(const collision_triangle_3d_t* self, sphere_t sphere, rayhit_t* hit) {
    // Get the vector from triangle to sphere center, and project it onto the normal vector to get the distance
    const vec3_t triangle_to_center = vec3_sub(sphere.center, self->v0);
    const scalar_t distance = vec3_dot(triangle_to_center, self->normal);

    // Check if the distance is lower than the sphere radius, if so, we are colliding
    if (distance.raw < sphere.radius.raw) {
        hit->distance = distance;
        hit->position = vec3_sub(sphere.center, vec3_mul(vec3_from_scalar(distance), self->normal));
        hit->triangle = self;
        hit->normal = self->normal;
        return 1;
    }
    return 0;
}
