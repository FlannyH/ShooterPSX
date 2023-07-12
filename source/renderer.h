#ifndef RENDERER_H
#define RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

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
#define RES_X 512
#ifdef PAL
#define RES_Y 256
#else
#define RES_Y 240
#endif
#define CENTER_X (RES_X / 2)
#define CENTER_Y (RES_Y / 2)

typedef struct {
    VECTOR position; // Position (4096 is 1.0 meter)
    VECTOR rotation; // Angles in fixed-point format (4194304 = 360 degrees)
    VECTOR scale; // Scale (4096 = 1.0)
} transform_t;
const static transform_t id_transform = { {0,0,0},{0,0,0}, {-4096, -4096, -4096} };
extern int widescreen;

// Functions
void renderer_init(void); // Initializes the renderer by configuring the GPU, setting the video mode, and preparing the drawing environment
void renderer_begin_frame(transform_t* camera_transform); // Applies the camera transform to the renderer, preparing it for a new frame
void renderer_end_frame(void); // Draws the render queue, swaps the drawbuffer, clears the render queue, and applies the display environments
void renderer_draw_model_shaded(const model_t* model, transform_t* model_transform); // Draws a 3D model at a given transform using shaded triangle primitives
void renderer_draw_mesh_shaded(const mesh_t* mesh, transform_t* model_transform); // Draws a 3D mesh at a given transform using shaded triangle primitives
void renderer_draw_triangles_shaded_2d(const vertex_2d_t* vertex_buffer, uint16_t n_verts, int16_t x, int16_t y);
void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, transform_t* model_transform);
void renderer_debug_draw_aabb(const aabb_t* box, pixel32_t color, transform_t* model_transform);
void renderer_debug_draw_sphere(sphere_t sphere);
void renderer_upload_texture(const texture_cpu_t* texture, uint8_t index);
int renderer_get_delta_time_raw(void);
int renderer_get_delta_time_ms(void);
int renderer_convert_dt_raw_to_ms(int dt_raw);
int renderer_should_close(void);
vec3_t renderer_get_forward_vector(void);
extern int n_rendered_triangles;
extern int n_rendered_quads;
extern int n_rendered_untex_triangles;
extern int n_rendered_untex_quads;
extern int n_calls_to_draw_triangle;
extern int n_calls_to_draw_quad;
extern int n_meshes_drawn;
extern int n_meshes_total;
#ifdef __cplusplus
}
#endif
#endif