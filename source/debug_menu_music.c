#include "main.h"

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

#ifdef _WINDOWS
#include "windows/psx.h"
#include "windows/debug_layer.h"
#endif

#ifdef _NDS
#include "nds/psx.h"
#include <filesystem.h>
#endif

void state_enter_debug_menu_music(void) {
    state_enter_title_screen();
	state.global.state_to_return_to = prev_state;
	state.global.fade_level = 255;
	state.title_screen.button_pressed = 0;
	while (state.global.fade_level > 0) {
		renderer_begin_frame(&id_transform);
		ui_render_background();
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 0;
}

void state_update_debug_menu_music(int dt) {
	(void)dt;
	renderer_begin_frame(&id_transform);
	input_update();
    
    ui_render_background();
	
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_debug_menu_music[0], 1, 1, white);

	char* songs[] = {
		"\\assets\\audio\\music\\level1.dss",
		"\\assets\\audio\\music\\level2.dss",
		"\\assets\\audio\\music\\level3.dss",
		"\\assets\\audio\\music\\subnivis.dss",
	};

	// Draw settings text and box
	for (int i = 0; i < 5; ++i) {
		pixel32_t color = (pixel32_t){128, 128, 128, 255};
		if (i == state.debug_menu_music.button_selected) {
			if (state.debug_menu_music.button_pressed) {
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
		renderer_draw_text((vec2_t){64*ONE, (96 + (24 * i))*ONE}, text_debug_menu_music[i+1], 1, 0, white);
		renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (96 + (24 * i))*ONE}, (vec2_t){448*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, color, 2, 5, 1);
	}

	// Handle button navigation
	if (input_pressed(PAD_UP, 0) && state.debug_menu_music.button_selected > 0) {
		state.debug_menu_music.button_selected--;
		state.debug_menu_music.button_pressed = 0;
	}
	if (input_pressed(PAD_DOWN, 0) && state.debug_menu_music.button_selected < 4) {
		state.debug_menu_music.button_selected++;
		state.debug_menu_music.button_pressed = 0;
	}
	if (input_pressed(PAD_CROSS, 0)) {
		state.debug_menu_music.button_pressed = 1;
	}

	// Handle button presses
	if (input_released(PAD_CROSS, 0)) {
		state.debug_menu_music.button_pressed = 0;
		switch (state.debug_menu_music.button_selected) {
			case 0:
			case 1: 
			case 2: 
			case 3: 
				music_stop();
				mem_stack_release(STACK_TEMP);
				mem_stack_release(STACK_MUSIC);
				audio_load_soundbank("\\assets\\audio\\instr.sbk", SOUNDBANK_TYPE_MUSIC);
				music_load_sequence(songs[state.debug_menu_music.button_selected]);
				music_set_volume(255);
				music_play_sequence(0);
				break;
			case 4:
                current_state = STATE_DEBUG_MENU_MAIN;
				break;
		}
	}
	renderer_end_frame();
	return;
}

void state_exit_debug_menu_music(void) {
	state.global.fade_level = 0;

	while (state.global.fade_level < 255) {
		renderer_begin_frame(&id_transform);
		input_update();
		ui_render_background();
		state.global.fade_level += FADE_SPEED;
		renderer_apply_fade(state.global.fade_level);
		renderer_end_frame();
	}
	state.global.fade_level = 255;
}
