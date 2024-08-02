#include "ui.h"

#include "renderer.h"
#include "vec2.h"

void ui_render_background() {
#if defined(_PSX) || defined(_WINDOWS)
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 3, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 4, 1);
#elif defined(_NDS)
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 136*ONE}, (vec2_t){512*ONE, 240*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 191*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 4, 1);
#endif
}

void ui_render_logo() {

}

void ui_render_button() {

}
