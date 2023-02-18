#ifndef RENDERER_H
#define RENDERER_H
#ifdef _PSX
#include <psxgte.h>
#else
#include "win/psx.h"
#endif
#include <stdint.h>
#include "mesh.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "common.h"
#include "texture.h"

#define ORD_TBL_LENGTH 4096
#define RES_X 320
#ifdef PAL
#define RES_Y 256
#else
#define RES_Y 240
#endif
#define CENTER_X (RES_X / 2)
#define CENTER_Y (RES_Y / 2)

typedef struct {
    VECTOR position; // Position (todo: settle on a fixed point unit, maybe 128 is one meter?)
    VECTOR rotation; // Angles in fixed-point format (4194304 = 360 degrees)
    VECTOR scale; // Scale (todo: settle on a fixed point unit)
} Transform;

// Functions
void renderer_init(void); // Initializes the renderer by configuring the GPU, setting the video mode, and preparing the drawing environment
void renderer_begin_frame(Transform* camera_transform); // Applies the camera transform to the renderer, preparing it for a new frame
void renderer_end_frame(void); // Draws the render queue, swaps the drawbuffer, clears the render queue, and applies the display environments
void renderer_draw_model_shaded(const Model* model, Transform* model_transform); // Draws a 3D model at a given transform using shaded triangle primitives
void renderer_draw_mesh_shaded(const Mesh* mesh, Transform* model_transform); // Draws a 3D mesh at a given transform using shaded triangle primitives
void renderer_draw_triangles_shaded_2d(const Vertex2D* vertex_buffer, uint16_t n_verts, int16_t x, int16_t y);
void renderer_upload_texture(const TextureCPU* texture, uint8_t index);
int renderer_get_delta_time_raw(void);
int renderer_get_delta_time_ms(void);
uint32_t renderer_get_n_rendered_triangles(void);
uint32_t renderer_get_n_total_triangles(void);
#endif