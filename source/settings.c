#include "main.h"

#include "input.h"
#include "text.h"
#include "ui.h"

#ifdef _PSX
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxpad.h>
#endif

#ifdef _PC
#include "pc/psx.h"
#include "pc/debug_layer.h"
#endif

#ifdef _NDS
#include "nds/psx.h"
#include <filesystem.h>
#endif

void state_enter_settings(void) {
	state.global.state_to_return_to = get_prev_state();
	state.title_screen.button_pressed = 0;
	renderer_start_fade_in(FADE_SPEED);
	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		ui_render_background();
		renderer_end_frame();
	}
}

void state_update_settings(int dt) {
	(void)dt;
	renderer_begin_frame(&id_transform);
	input_update();
    
    ui_render_background();
	
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_settings[0], 1, 1, white);

	// Draw settings text and box
	for (int i = 0; i < 5; ++i) {
		pixel32_t color = (pixel32_t){128, 128, 128, 255};
		if (i == state.settings.button_selected) {
			if (state.settings.button_pressed) {
				color.r = 64;
				color.g = 64;
				color.b = 64;
			}
			else {
				color.r = 255;
				color.g = 255;
				color.b = 255;
			}
		}
		renderer_draw_text((vec2_t){64*ONE, (96 + (24 * i))*ONE}, text_settings[i+1], 1, 0, white);
		renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (96 + (24 * i))*ONE}, (vec2_t){448*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, color, 2, 5, 1);
	}

	// Draw values
	char num_display[6];
	snprintf(num_display, 6, "%i", renderer_width());
	char* fps_text = "UNLOCKED";
	if (vsync_enable == 1) {
		if (is_pal) fps_text = "50 FPS";
		else fps_text = "60 FPS";
	} 
	else if (vsync_enable == 2) {
		if (is_pal) fps_text = "25 FPS";
		else fps_text = "30 FPS";
	} 
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 0))*ONE}, fps_text, 1, 0, white);
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 1))*ONE}, is_pal ? "PAL" : "NTSC", 1, 0, white);
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 2))*ONE}, widescreen ? "16:9" : "4:3", 1, 0, white);
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 3))*ONE}, num_display, 1, 0, white);

	// Handle button navigation
	if (input_pressed(PAD_UP, 0) && state.settings.button_selected > 0) {
		state.settings.button_selected--;
		state.settings.button_pressed = 0;
	}
	if (input_pressed(PAD_DOWN, 0) && state.settings.button_selected < 4) {
		state.settings.button_selected++;
		state.settings.button_pressed = 0;
	}
	if (input_pressed(PAD_CROSS, 0)) {
		state.settings.button_pressed = 1;
	}

	// Handle button presses
	if (input_released(PAD_CROSS, 0)) {
		state.settings.button_pressed = 0;
		switch (state.settings.button_selected) {
			case 0: // frame rate limit
				vsync_enable--;
				if (vsync_enable < 0) {
					vsync_enable = 2;
				}
				break;
			case 1: // video mode pal or ntsc
				is_pal = !is_pal;
				renderer_set_video_mode(is_pal);
				break;
			case 2: // aspect ratio
				widescreen = !widescreen;
				break;
			case 3: // controller sensitivity
				break;
			case 4: // back
				set_current_state(state.global.state_to_return_to);
				break;
		}
	}
	renderer_end_frame();
	return;
}

void state_exit_settings(void) {
	renderer_start_fade_out(FADE_SPEED);

	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		input_update();
		ui_render_background();
		
		renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_settings[0], 1, 1, white);

		// Draw settings text and box
		for (int i = 0; i < 5; ++i) {
			pixel32_t color = (pixel32_t){128, 128, 128, 255};
			if (i == state.settings.button_selected) {
				if (state.settings.button_pressed) {
					color.r = 64;
					color.g = 64;
					color.b = 64;
				}
				else {
					color.r = 255;
					color.g = 255;
					color.b = 255;
				}
			}
			renderer_draw_text((vec2_t){64*ONE, (96 + (24 * i))*ONE}, text_settings[i+1], 1, 0, white);
			renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (96 + (24 * i))*ONE}, (vec2_t){448*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, color, 2, 5, 1);
		}
		renderer_end_frame();
	}
}
