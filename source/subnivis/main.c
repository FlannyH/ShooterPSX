#include "main.h"

#include "input.h"
#include "input_mapping.h"
#include "input_map.h"
#include "music.h"
#include "file.h"
#include "player.h"
#include "renderer.h"

#ifdef _DEBUG
#include "test/test.h"
#endif

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
#include <nds.h>
#endif

#include <string.h>

int widescreen = 0;
state_t current_state = STATE_NONE;
state_t prev_state = STATE_NONE;
state_vars_t state;

void set_current_state(state_t state) { current_state = state; }
state_t get_current_state(void) { return current_state; }
state_t get_prev_state(void) { return prev_state; }

void platform_init(void) {
#ifdef _PSX
    // Reset GPU and enable interrupts
    ResetGraph(0);
#endif
#ifdef _NDS
	defaultExceptionHandler();
#endif
}

int main(void) {
	// Init systems
	mem_init();

#ifdef _DEBUG
	mem_debug();
#endif

	platform_init();
	file_init("\\assets.sfa");
	renderer_init();
	audio_init();

	input_init();
	input_set_gamepad_stick_deadzone(SCALAR(36.0/128));

	input_mapping_init();
	input_mapping_register_keyboard(IM_JUMP, INPUT_KEY_SPACE, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MOVE_X, INPUT_KEY_A, SCALAR(-1.0));
    input_mapping_register_keyboard(IM_MOVE_X, INPUT_KEY_D, SCALAR(+1.0));
    input_mapping_register_keyboard(IM_MOVE_Y, INPUT_KEY_W, SCALAR(+1.0));
    input_mapping_register_keyboard(IM_MOVE_Y, INPUT_KEY_S, SCALAR(-1.0));
    input_mapping_register_mouse(IM_LOOK_MOUSE_X, INPUT_MOUSE_DELTA_X, mouse_sensitivity);
    input_mapping_register_mouse(IM_LOOK_MOUSE_Y, INPUT_MOUSE_DELTA_Y, mouse_sensitivity);
    input_mapping_register_keyboard(IM_MENU_UP, INPUT_KEY_UP, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_UP, INPUT_KEY_W, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_DOWN, INPUT_KEY_DOWN, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_DOWN, INPUT_KEY_S, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_LEFT, INPUT_KEY_LEFT, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_LEFT, INPUT_KEY_A, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_RIGHT, INPUT_KEY_RIGHT, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_RIGHT, INPUT_KEY_D, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_GO, INPUT_KEY_SPACE, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_PREVIEW, INPUT_KEY_ENTER, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MENU_TAB, INPUT_KEY_TAB, SCALAR(1.0));
    input_mapping_register_keyboard(IM_START_PAUSE, INPUT_KEY_ESC, SCALAR(1.0));
    input_mapping_register_mouse(IM_SHOOT, INPUT_MOUSE_BUTTON_RIGHT, SCALAR(1.0));
    input_mapping_register_keyboard(IM_DEBUG_DOWN, INPUT_KEY_DOWN, SCALAR(1.0));
    input_mapping_register_keyboard(IM_DEBUG_UP, INPUT_KEY_UP, SCALAR(1.0));

#ifdef _DEBUG
    const int n_failed_tests = test();
    printf("Unit tests finished with %i errors\n", n_failed_tests);
#endif

	// Init state variables
	memset(&state, 0, sizeof(state));
	state.in_game.level = (level_t){0};
	state.global.frame_counter = 0;
	state.global.show_debug = 0;

	// Let's start here
	current_state = STATE_DEBUG_MENU_MAIN;

    while (!renderer_should_close()) {
        scalar_t delta_time = renderer_delta_time(DT_TICK);
#ifndef BENCHMARK_MODE
        delta_time = scalar_min(delta_time, SCALAR(1.0 / 25.0));
#endif
		state.global.time_counter += delta_time;

		// If a state change happened, transition between them
		if (current_state != prev_state) {
			// Exit the previous state
			switch(prev_state) {
				case STATE_NONE:				/* nothing */ 						break;
				case STATE_TITLE_SCREEN: 		state_exit_title_screen(); 			break;
				case STATE_CREDITS: 			state_exit_credits(); 				break;
				case STATE_SETTINGS: 			state_exit_settings(); 				break;
				case STATE_IN_GAME: 			state_exit_in_game(); 				break;
				case STATE_PAUSE_MENU: 			state_exit_pause_menu(); 			break;
				case STATE_DEBUG_MENU_MAIN:		state_exit_debug_menu_main(); 		break;
				case STATE_DEBUG_MENU_LEVEL:	state_exit_debug_menu_level(); 		break;
				case STATE_DEBUG_MENU_MUSIC:	state_exit_debug_menu_music(); 		break;
			}
			// Enter the current state
			switch(current_state) {
				case STATE_NONE:				/* nothing */ 						break;
				case STATE_TITLE_SCREEN:		state_enter_title_screen();			break;
				case STATE_CREDITS:				state_enter_credits();				break;
				case STATE_SETTINGS:			state_enter_settings();				break;
				case STATE_IN_GAME:				state_enter_in_game();				break;
				case STATE_PAUSE_MENU:			state_enter_pause_menu();			break;
				case STATE_DEBUG_MENU_MAIN:		state_enter_debug_menu_main();		break;
				case STATE_DEBUG_MENU_LEVEL:	state_enter_debug_menu_level();		break;
				case STATE_DEBUG_MENU_MUSIC:	state_enter_debug_menu_music();		break;
			}
		}

		// Update the current state
		prev_state = current_state;
		switch(current_state) {
			case STATE_NONE:				/* nothing */ 								break;
			case STATE_TITLE_SCREEN:		state_update_title_screen(delta_time);		break;
			case STATE_CREDITS:				state_update_credits(delta_time);			break;
			case STATE_SETTINGS:			state_update_settings(delta_time);			break;
			case STATE_IN_GAME:				state_update_in_game(delta_time);			break;
			case STATE_PAUSE_MENU:			state_update_pause_menu(delta_time);		break;
			case STATE_DEBUG_MENU_MAIN:		state_update_debug_menu_main(delta_time);	break;
			case STATE_DEBUG_MENU_LEVEL:	state_update_debug_menu_level(delta_time);	break;
			case STATE_DEBUG_MENU_MUSIC:	state_update_debug_menu_music(delta_time);	break;
		}
	}
#ifdef _PC
	debug_layer_close();
#endif
    return 0;
}
