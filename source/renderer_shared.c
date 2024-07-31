#include "renderer.h"

// This file contains code that's either the exact same across platforms,
// or so similar that it makes sense to put it in the same file with ifdefs.

// Shared rendering parameters
uint8_t tex_id_start = 0;

// Debug parameters
#ifdef _LEVEL_EDITOR
extern int drawing_entity_id;
#endif

void renderer_draw_mesh_shaded_offset(const mesh_t* mesh, const transform_t* model_transform, int tex_id_offset) {
    tex_id_start = tex_id_offset;
    renderer_draw_mesh_shaded(mesh, model_transform);
}

void renderer_draw_2d_quad_axis_aligned(vec2_t center, vec2_t size, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
    const vec2_t tl = {center.x - size.x/2, center.y - size.y/2};
    const vec2_t tr = {center.x + size.x/2, center.y - size.y/2};
    const vec2_t bl = {center.x - size.x/2, center.y + size.y/2};
    const vec2_t br = {center.x + size.x/2, center.y + size.y/2};
    renderer_draw_2d_quad(tl, tr, bl, br, uv_tl, uv_br, color, depth, texture_id, is_page);
}

void renderer_debug_draw_aabb(const aabb_t* box, const pixel32_t color, const transform_t* model_transform) {
    // Create 8 vertices
    const vec3_t vertex000 = {box->min.x, box->min.y, box->min.z};
    const vec3_t vertex001 = {box->min.x, box->min.y, box->max.z};
    const vec3_t vertex010 = {box->min.x, box->max.y, box->min.z};
    const vec3_t vertex011 = {box->min.x, box->max.y, box->max.z};
    const vec3_t vertex100 = {box->max.x, box->min.y, box->min.z};
    const vec3_t vertex101 = {box->max.x, box->min.y, box->max.z};
    const vec3_t vertex110 = {box->max.x, box->max.y, box->min.z};
    const vec3_t vertex111 = {box->max.x, box->max.y, box->max.z};

    // Draw the lines
    renderer_debug_draw_line(vertex000, vertex100, color, model_transform);
    renderer_debug_draw_line(vertex100, vertex101, color, model_transform);
    renderer_debug_draw_line(vertex101, vertex001, color, model_transform);
    renderer_debug_draw_line(vertex001, vertex000, color, model_transform);
    renderer_debug_draw_line(vertex010, vertex110, color, model_transform);
    renderer_debug_draw_line(vertex110, vertex111, color, model_transform);
    renderer_debug_draw_line(vertex111, vertex011, color, model_transform);
    renderer_debug_draw_line(vertex011, vertex010, color, model_transform);
    renderer_debug_draw_line(vertex000, vertex010, color, model_transform);
    renderer_debug_draw_line(vertex100, vertex110, color, model_transform);
    renderer_debug_draw_line(vertex101, vertex111, color, model_transform);
    renderer_debug_draw_line(vertex001, vertex011, color, model_transform);
}

void renderer_debug_draw_sphere(const sphere_t sphere) {
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(sphere.radius, 0, 0)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(-sphere.radius, 0, 0)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(0, 0, sphere.radius)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(0, 0, -sphere.radius)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(+sphere.radius, 0, +sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(+sphere.radius, 0, -sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(-sphere.radius, 0, +sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(-sphere.radius, 0, -sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
}

void renderer_draw_model_shaded(const model_t* model, const transform_t* model_transform, visfield_t* vislist, int tex_id_offset) {
	if (!model) return;
    tex_id_start = tex_id_offset;

#ifdef _LEVEL_EDITOR
	drawing_entity_id = 255;
#endif

    if (vislist == NULL || n_sections == 0) {
        for (size_t i = 0; i < model->n_meshes; ++i) {
            //renderer_debug_draw_aabb(&model->meshes[i].bounds, red, &id_transform);
            renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
        }
    }
    else {
        // Determine which meshes to render
        visfield_t combined = { 0, 0, 0, 0 };

        // Get all the vislist bitfields and combine them together
        for (size_t i = 0; i < n_sections; ++i) {
            combined.sections_0_31 |= vislist[sections[i]].sections_0_31;
            combined.sections_32_63 |= vislist[sections[i]].sections_32_63;
            combined.sections_64_95 |= vislist[sections[i]].sections_64_95;
            combined.sections_96_127 |= vislist[sections[i]].sections_96_127;
        }

        // Render only the meshes that are visible
        for (size_t i = 0; i < model->n_meshes; ++i) {
            if ((i < 32) && (combined.sections_0_31 & (1 << i))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i >= 32) && (i < 64) && (combined.sections_32_63 & (1 << (i - 32)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i >= 64) && (i < 96) && (combined.sections_64_95 & (1 << (i - 64)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i >= 96) && (i < 128) && (combined.sections_96_127 & (1 << (i - 96)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
        }
    }

	tex_id_start = 0;
}