#include "main.h"

#include "memory.h"
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

void state_enter_debug_menu_level(void) {
    state_enter_title_screen();
	state.global.state_to_return_to = get_prev_state();
	state.title_screen.button_pressed = 0;
	renderer_start_fade_in(FADE_SPEED);
	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		ui_render_background();
		renderer_end_frame();
	}
}

void state_update_debug_menu_level(int dt) {
	(void)dt;
	renderer_begin_frame(&id_transform);
	input_update();
    
    ui_render_background();
	
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_debug_menu_level[0], 1, 1, white);

	char* levels[] = {
		"levels/test.lvl",
		"levels/test2.lvl",
		"levels/test3.lvl",
		"levels/level1.lvl",
		"levels/level2.lvl",
	};

	// Draw settings text and box
	for (int i = 0; i < 4; ++i) {
		pixel32_t color = (pixel32_t){128, 128, 128, 255};
		if (i == state.debug_menu_level.button_selected) {
			if (state.debug_menu_level.button_pressed) {
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
		renderer_draw_text((vec2_t){64*ONE, (96 + (24 * i))*ONE}, text_debug_menu_level[i+1], 1, 0, white);
		renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (96 + (24 * i))*ONE}, (vec2_t){448*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, color, 2, 5, 1);
	}

	// Handle button navigation
	if (input_pressed(PAD_UP, 0) && state.debug_menu_level.button_selected > 0) {
		state.debug_menu_level.button_selected--;
		state.debug_menu_level.button_pressed = 0;
	}
	if (input_pressed(PAD_DOWN, 0) && state.debug_menu_level.button_selected < 4) {
		state.debug_menu_level.button_selected++;
		state.debug_menu_level.button_pressed = 0;
	}
	if (input_pressed(PAD_CROSS, 0)) {
		state.debug_menu_level.button_pressed = 1;
	}

	// Handle button presses
	if (input_released(PAD_CROSS, 0)) {
		state.debug_menu_level.button_pressed = 0;
		switch (state.debug_menu_level.button_selected) {
			case 0:
			case 1: 
			case 2: 
			case 3: 
			case 4: 
				mem_stack_release(STACK_TEMP);
				state.in_game.level_load_path = levels[state.debug_menu_level.button_selected];
				set_current_state(STATE_IN_GAME);
				break;
			case 5:
				set_current_state(STATE_DEBUG_MENU_MAIN);
				break;
		}
	}
	renderer_end_frame();
	return;
}

void state_exit_debug_menu_level(void) {
	renderer_start_fade_out(FADE_SPEED);

	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		input_update();
		ui_render_background();
		renderer_end_frame();
	}
}
