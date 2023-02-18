#ifndef DEBUG_LAYER_H
#define DEBUG_LAYER_H
#include "glfw/glfw3.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sadly, these functions have to be C++, because I can't for the life of me get cimgui to work.
void debug_layer_init(GLFWwindow* window);
void debug_layer_update();
void debug_layer_close();

#ifdef __cplusplus
}
#endif

#endif