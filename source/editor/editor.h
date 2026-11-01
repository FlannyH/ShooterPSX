#ifndef DEBUG_LAYER_H
#define DEBUG_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#define GLFW_INCLUDE_NONE
#include "engine/level.h"
#include "engine/texture.h"
#include "subnivis/player.h"

#include <GLFW/glfw3.h>

// Sadly, these functions have to be C++, because I can't for the life of me get cimgui to work.
void debug_layer_init(void);
void debug_layer_begin(void);
void debug_layer_end(void);
void debug_layer_update_gameplay(void);
void debug_layer_close(void);
void debug_layer_manipulate_entity(transform_t* camera, int* selected_entity_slot, int* selected_light_slot, int* selected_shape_slot, int* mouse_over_viewport, level_t* curr_level, player_t* player);

vec3_t editor_selected_vertex_position(void);
void editor_draw_sphere(transform_t transform);

int inspect_int(int* value, const char* label, int min, int max, int draggable);
int inspect_bool(int* value, const char* label);
int inspect_pixel32(pixel32_t* vec, const char* label);
int inspect_vec3(vec3_t* vec, const char* label);
int inspect_svec3_4_12(svec3_t* vec, const char* label);
int inspect_scalar(scalar_t* scalar, const char* label);
size_t inspect_enum(size_t value, const char** names, const char* label);
int inspect_entity(size_t entity_id);
int inspect_light(size_t light_id);
int inspect_shape(size_t shape_id);
int inspect_button(const char* label);
int inspect_tree_push(const char* label);
void inspect_tree_pop(void);
void inspect_index_push(int i);
void inspect_index_pop(void);
void editor_set_viewport_position(vec3_t pos);
void editor_set_viewport_rotation(vec3_t rot);
vec3_t editor_get_viewport_position(void);
vec3_t editor_get_viewport_rotation(void);

#ifdef __cplusplus
}
#endif

#endif
