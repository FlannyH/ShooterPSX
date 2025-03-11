#ifndef DEBUG_LAYER_H
#define DEBUG_LAYER_H
#define GLFW_INCLUDE_NONE
#include "../level.h"
#include "../player.h"

#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sadly, these functions have to be C++, because I can't for the life of me get cimgui to work.
void debug_layer_init(GLFWwindow* window);
void debug_layer_begin(void);
void debug_layer_end(void);
void debug_layer_update_gameplay(void);
void debug_layer_close(void);
void debug_layer_manipulate_entity(transform_t* camera, int* selected_entity_slot, int* selected_light_slot, int* mouse_over_viewport, level_t* curr_level, player_t* player);

#ifdef __cplusplus
}
#endif
#endif
