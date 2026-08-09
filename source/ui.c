#include "ui.h"
#include "common.h"

#include "renderer.h"
#include "math/vec2.h"

void ui_render_background() {
#if defined(_PSX) || defined(_PC)
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 3, TEX_CAT_MISC);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 4, TEX_CAT_MISC);
#elif defined(_NDS)
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 136*ONE}, (vec2_t){512*ONE, 240*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 191*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 4, TEX_CAT_MISC);
#endif
}

void ui_render_logo() {
    renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 85*ONE}, (vec2_t){128*ONE, 72*ONE}, (vec2_t){0*ONE, 184*ONE}, (vec2_t){128*ONE, 255*ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, TEX_CAT_MISC);
}

void ui_render_button() {

}
