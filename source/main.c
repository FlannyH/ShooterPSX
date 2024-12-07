#include "main.h"

#include "entities/pickup.h"
#include "entities/chaser.h"
#include "entities/crate.h"
#include "entities/door.h"
#include "fixed_point.h"
#include "renderer.h"
#include "texture.h"
#include "cheats.h"
#include "player.h"
#include "memory.h"
#include "entity.h"
#include "random.h"
#include "timer.h"
#include "input.h"
#include "music.h"
#include "file.h"
#include "text.h"

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
#include <nds.h>
#endif

#include <string.h>

int widescreen = 0;
state_t current_state = STATE_NONE;
state_t prev_state = STATE_NONE;
state_vars_t state;

int main(void) {
	// Init systems
	mem_init();

#ifdef _DEBUG
	mem_debug();
#endif

	file_init("\\assets.sfa");
	renderer_init();
	input_init();
	input_set_stick_deadzone(36);
	audio_init();
	init();
	setup_timers();

	// Init state variables
	memset(&state, 0, sizeof(state));
	state.in_game.level = (level_t){0};
	state.global.frame_counter = 0;
	state.global.show_debug = 0;

	// Let's start here
	current_state = STATE_DEBUG_MENU_MAIN;

    while (!renderer_should_close()) {
#ifndef _WINDOWS
        int delta_time_raw = renderer_get_delta_time_raw();
        int delta_time = renderer_convert_dt_raw_to_ms(delta_time_raw);
#else
        int delta_time = renderer_get_delta_time_ms();
#endif
#ifndef BENCHMARK_MODE
        delta_time = scalar_min(delta_time, 40);
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
#ifdef _WINDOWS
	debug_layer_close();
#endif
    return 0;
}

void init(void) {
#ifdef _PSX
	CdInit();
#endif
#ifdef _NDS
	defaultExceptionHandler();
	nitroFSInit(NULL);
#endif
}
