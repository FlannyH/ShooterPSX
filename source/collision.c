#include "collision.h"

#include <stdlib.h>

#include "mesh.h"
#include "renderer.h"

void bvh_construct(bvh_t* self, triangle_3d_t* primitives, uint16_t n_primitives) {
    self->primitives = primitives;
    self->n_primitives = n_primitives;

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
    result.max = vec3_from_int16s(INT16_MIN, INT16_MIN, INT16_MIN);
    result.min = vec3_from_int16s(INT16_MAX, INT16_MAX, INT16_MAX);
    for (int i = 0; i < count; i++)
    {
        const aabb_t curr_primitive_bounds = triangle_get_bounds(&self->primitives[self->indices[first + i]]);

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
    scalar_t split_pos = scalar_from_int16(0);

    const vec3_t size = vec3_sub(node->bounds.max, node->bounds.min);

    if (size.x.raw > size.y.raw 
        && size.x.raw > size.z.raw)
    {
        split_axis = axis_x;
        split_pos = scalar_mul(scalar_from_float(0.5f), scalar_add(node->bounds.max.x, node->bounds.min.x));
    }

    if (size.y.raw > size.x.raw
        && size.y.raw > size.z.raw)
    {
        split_axis = axis_y;
        split_pos = scalar_mul(scalar_from_float(0.5f), scalar_add(node->bounds.max.y, node->bounds.min.y));
    }

    if (size.z.raw > size.x.raw
        && size.z.raw > size.y.raw)
    {
        split_axis = axis_z;
        split_pos = scalar_mul(scalar_from_float(0.5f), scalar_add(node->bounds.max.z, node->bounds.min.z));
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
        const aabb_t bounds = triangle_get_bounds(&self->primitives[self->indices[j]]);

        // Get center
        vec3_t center = vec3_mul(vec3_add(bounds.min, bounds.max), vec3_from_int16s(128, 128, 128)); // 128 = 0.5 (0x00.80)
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


void debug_draw(const bvh_t* self, bvh_node_t* node, int min_depth, int max_depth, int curr_depth, pixel32_t color) {
    const transform_t trans = { 0, 0, 0, 0, 0, 0 };

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

void bvh_debug_draw(const bvh_t* self, int min_depth, int max_depth, pixel32_t color) {
    debug_draw(self, self->root, min_depth, max_depth, 0, color);
}