#include "renderer.h"
#include "lut.h"

#include <string.h>

// This file contains code that's either the exact same across platforms,
// or so similar that it makes sense to put it in the same file with ifdefs.

// Shared rendering parameters
uint8_t tex_id_start = 0;

// Debug parameters
#ifdef _LEVEL_EDITOR
extern int drawing_entity_id;
#endif

void renderer_draw_2d_quad_axis_aligned(vec2_t center, vec2_t size, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
    const vec2_t tl = {center.x - size.x/2, center.y - size.y/2};
    const vec2_t tr = {center.x + size.x/2, center.y - size.y/2};
    const vec2_t bl = {center.x - size.x/2, center.y + size.y/2};
    const vec2_t br = {center.x + size.x/2, center.y + size.y/2};
    if ((tl.y > 256 * ONE) || (bl.y < 0) || (tl.x > 512 * ONE) || (tr.x < 0)) return;
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
            renderer_draw_mesh_shaded(&model->meshes[i], model_transform, 0, 0, tex_level_start);
        }
    }
    else {
        // Determine which meshes to render
        visfield_t combined = { 0, 0, 0, 0 };

        // Get all the vislist bitfields and combine them together
        for (int i = 0; i < n_sections; ++i) {
            combined.sections_0_31 |= vislist[sections[i]].sections_0_31;
            combined.sections_32_63 |= vislist[sections[i]].sections_32_63;
            combined.sections_64_95 |= vislist[sections[i]].sections_64_95;
            combined.sections_96_127 |= vislist[sections[i]].sections_96_127;
        }

        // Render only the meshes that are visible
        for (size_t i = 0; i < model->n_meshes; ++i) {
            if ((i < 32) && (combined.sections_0_31 & (1 << i))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform, 0, 0, tex_level_start);
            else if ((i >= 32) && (i < 64) && (combined.sections_32_63 & (1 << (i - 32)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform, 0, 0, tex_level_start);
            else if ((i >= 64) && (i < 96) && (combined.sections_64_95 & (1 << (i - 64)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform, 0, 0, tex_level_start);
            else if ((i >= 96) && (i < 128) && (combined.sections_96_127 & (1 << (i - 96)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform, 0, 0, tex_level_start);
        }
    }

	tex_id_start = 0;
}

int renderer_get_level_section_from_position(const model_t* model, vec3_t position) {
    position.x = -position.x / COL_SCALE;
    position.y = -position.y / COL_SCALE;
    position.z = -position.z / COL_SCALE;
    n_sections = 0;
    for (size_t i = 0; i < model->n_meshes; ++i) {
        if (n_sections == N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE) break;
        if (point_aabb_intersect(&model->meshes[i].bounds, position)) {
            renderer_debug_draw_aabb(&model->meshes[i].bounds, green, &id_transform);
            sections[n_sections] = i;
            n_sections += 1;
        }
        else {
            renderer_debug_draw_aabb(&model->meshes[i].bounds, red, &id_transform);
        }
    }
    return n_sections; // -1 means no section
}

void renderer_draw_text(vec2_t pos, const char* text, const int text_type, const int centered, const pixel32_t color) {
    // Set to text_type == 0 by default. This way the variables are always initialized
    int font_x = 0;
    int font_y = 60;
    int font_src_width = 7;
    int font_src_height = 9;
    int font_dst_width = 7;
    int font_dst_height = 9;
    int chars_per_row = 36;

    if (text_type == 1) {
        font_x = 0;
        font_y = 80;
        font_src_width = 16;
        font_src_height = 16;
        font_dst_width = 16;
        font_dst_height = 16;
        chars_per_row = 16;
    }
    else if (text_type == 2) {
        font_x = 0;
        font_y = 60;
        font_src_width = 7;
        font_src_height = 9;
        font_dst_width = 14;
        font_dst_height = 18;
        chars_per_row = 16;
    }

    if (centered) {
        pos.x -= ((strlen(text)) * font_dst_width - (font_dst_width / 2)) * ONE / 2;
    }

    vec2_t start_pos = pos;

    // Draw each character
    while (*text) {
        // Handle special cases
        if (*text == '\n') {
            pos.x = start_pos.x;
            pos.y += font_dst_height * ONE;
            goto end;
        }
        
        if (*text == '\r') {
            pos.x = start_pos.x;
            goto end;
        }

        if (*text == '\t') {
            // Get X coordinate relative to start, and round the position up to the nearest multiple of 4
            scalar_t rel_x = pos.x - start_pos.x;
            int n_spaces = 4 - ((rel_x / font_dst_width) % 4);
            pos.x += n_spaces * font_dst_width * ONE;
            goto end;
        }

        // Get index in bitmap
        int index_to_draw = (int)lut_font_letters[(size_t)*text];
        
        // Get UV coordinates
        vec2_t top_left;
        top_left.x = (font_x + (font_src_width * (index_to_draw % chars_per_row))) * ONE;
        top_left.y = (font_y + ((index_to_draw / chars_per_row) * font_src_height)) * ONE;
        vec2_t bottom_right = vec2_add(top_left, (vec2_t){(font_src_width-1)*ONE, (font_src_height-1)*ONE});

        renderer_draw_2d_quad_axis_aligned(pos, (vec2_t){(font_dst_width-1)*ONE, (font_dst_height-1)*ONE}, top_left, bottom_right, (pixel32_t){color.r/2, color.g/2, color.b/2, 255}, 1, 5, 1);
        pos.x += font_dst_width * ONE;
        end:
        ++text;
    }
}