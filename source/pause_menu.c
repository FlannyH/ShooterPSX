#include "main.h"

#include <string.h>

#include "renderer.h"
#include "memory.h"
#include "cheats.h"
#include "input.h"
#include "music.h"
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
#include "windows/psx.h"
#include "windows/debug_layer.h"
#endif

#ifdef _NDS
#include "nds/psx.h"
#include <filesystem.h>
#endif

void state_enter_pause_menu(void) {}

void state_update_pause_menu(int dt) {
	(void)dt;
	renderer_begin_frame(&id_transform);
	input_update();
	if (input_pressed(PAD_START, 0)) {
		set_current_state(STATE_IN_GAME);
	}
	
	// Check cheats
	if (input_check_cheat_buffer(sizeof(cheat_doom_mode) / sizeof(uint16_t), cheat_doom_mode)) {
		state.cheats.doom_mode = 1;
		music_stop();
		mem_stack_release(STACK_MUSIC);
		music_load_sequence("audio/music/justice.dss");
		audio_load_soundbank("audio/instr.sbk", SOUNDBANK_TYPE_MUSIC);
		music_play_sequence(0);
	}

	// Paused text
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_pause_menu[0], 1, 1, white);
	ui_render_background();

	// Draw buttons
	for (int y = 0; y < 3; ++y) {
		// Left handle, right handle, middle bar, text
		pixel32_t color = (pixel32_t){128, 128, 128, 255};
		if (y == state.pause_menu.button_selected) {
			if (state.pause_menu.button_pressed) {
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
		renderer_draw_2d_quad_axis_aligned((vec2_t){145*ONE, (148 + (24 * y))*ONE}, (vec2_t){26*ONE, 20*ONE}, (vec2_t){129*ONE, 40*ONE}, (vec2_t){155*ONE, 60*ONE}, color, 2, 5, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){367*ONE, (148 + (24 * y))*ONE}, (vec2_t){26*ONE, 20*ONE}, (vec2_t){155*ONE, 40*ONE}, (vec2_t){129*ONE, 60*ONE}, color, 2, 5, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (148 + (24 * y))*ONE}, (vec2_t){192*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, color, 2, 5, 1);

		renderer_draw_text((vec2_t){256*ONE, (148 + (24 * y))*ONE}, text_pause_menu[y + 1], 1, 1, white);
	}

	// Handle button navigation
	if (input_pressed(PAD_UP, 0) && state.pause_menu.button_selected > 0) {
		state.pause_menu.button_selected--;
		state.pause_menu.button_pressed = 0;
	}
	if (input_pressed(PAD_DOWN, 0) && state.pause_menu.button_selected < 2) {
		state.pause_menu.button_selected++;
		state.pause_menu.button_pressed = 0;
	}
	if (input_pressed(PAD_CROSS, 0)) {
		state.pause_menu.button_pressed = 1;
	}

	// Handle button presses
	if (input_released(PAD_CROSS, 0)) {
		state.pause_menu.button_pressed = 0;
		switch (state.pause_menu.button_selected) {
			case 0: // continue
				set_current_state(STATE_IN_GAME);
				break;
			case 1: // settings
				set_current_state(STATE_SETTINGS);
				break;
			case 2: // exit game
				music_stop();
				set_current_state(STATE_TITLE_SCREEN);
				break;
		}
	}

	renderer_end_frame();
}

void state_exit_pause_menu(void) {}
