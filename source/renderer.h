#ifndef RENDERER_H
#define RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PSX
#include <psxgte.h>
#elif defined(_NDS)
#include "nds/psx.h"
#else
#include "pc/psx.h"
#endif

#include "structs.h"
#include "vislist.h"
#include "texture.h"
#include "common.h"
#include "mesh.h"
#include "vec2.h"

#include <stdint.h>
    
#define ORD_TBL_LENGTH 4096
#define RES_X 384
#define RES_Y_PAL 256
#define RES_Y_NTSC 240
#define N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE 4
#define NO_TEXTURE 255

const static transform_t id_transform = { {0,0,0},{0,0,0}, {-4096, -4096, -4096} };
extern int widescreen;

// Functions
void renderer_init(void); // Initializes the renderer by configuring the GPU, setting the video mode, and preparing the drawing environment
void renderer_begin_frame(const transform_t* camera_transform); // Applies the camera transform to the renderer, preparing it for a new frame
void renderer_end_frame(void); // Draws the render queue, swaps the drawbuffer, clears the render queue, and applies the display environments
void renderer_draw_model_shaded(const model_t* model, const transform_t* model_transform, visfield_t* vislist, int tex_id_offset); // Draws a 3D model at a given transform using shaded triangle primitives
void renderer_draw_mesh_shaded(const mesh_t* mesh, const transform_t* model_transform, int local, int facing_camera, int tex_id_offset); // Draws a 3D mesh at a given transform using shaded triangle primitives. Setting local to 1 draws it relative to the camera view.
void renderer_draw_2d_quad_axis_aligned(vec2_t center, vec2_t size, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page);
void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page);
void renderer_draw_text(vec2_t pos, const char* text, const int text_type, const int centered, const pixel32_t color);
void renderer_apply_fade(int fade_level);
void renderer_tick_fade(void);
void renderer_start_fade_in(int speed);
void renderer_start_fade_out(int speed);
int renderer_is_fading(void);
int renderer_get_fade_level(void);
void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform);
void renderer_debug_draw_aabb(const aabb_t* box, pixel32_t color, const transform_t* model_transform);
void renderer_debug_draw_sphere(sphere_t sphere);
void renderer_upload_texture(const texture_cpu_t* texture, uint8_t index);
void renderer_upload_8bit_texture_page(const texture_cpu_t* texture, const uint8_t index);
void renderer_set_video_mode(int is_pal);
int renderer_get_delta_time_raw(void);
int renderer_get_delta_time_ms(void);
int renderer_convert_dt_raw_to_ms(int dt_raw);
int renderer_should_close(void);
void renderer_set_depth_bias(int bias);
int renderer_n_meshes_drawn(void);
int renderer_get_camera_level_section(vec3_t pos, const vislist_t vis);
int renderer_width(void);
int renderer_height(void);

#ifdef _LEVEL_EDITOR
float* renderer_debug_perspective_matrix(void);
float* renderer_debug_view_matrix(void);
void renderer_set_drawing_id(int id, int what); // what: 0 = none, 1 = entity, 2 = light
void renderer_update_window_res(int width, int height);
#endif

inline uint8_t mul_8x8(const uint8_t a, const uint8_t b) {
    uint16_t c = ((uint16_t)a * (uint16_t)b) >> 8;
    if (c > 255) c = 255;
    return (uint8_t)c;
}

#ifdef _LEVEL_EDITOR
vec3_t renderer_get_forward_vector(void); // Used in the level editor to determine where to spawn new entities
#endif 

extern int tex_level_start;
extern int tex_entity_start;
extern int tex_weapon_start;
extern int tex_alloc_cursor;
extern int vsync_enable;
extern int is_pal;
#ifdef __cplusplus
}
#endif
#endif
