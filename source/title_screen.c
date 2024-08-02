#include "main.h"

#include <string.h>

#include "renderer.h"
#include "memory.h"
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

void state_enter_title_screen(void) {
	if (!state.title_screen.assets_in_memory) {
		// Load graphics data
#if defined(_PSX) || defined(_WINDOWS)
		texture_cpu_t *tex_menu1;
		texture_cpu_t *tex_menu2;
		texture_cpu_t *tex_ui;
		mem_stack_release(STACK_TEMP);
		texture_collection_load("\\assets\\models\\ui_tex\\menu1.txc", &tex_menu1, 1, STACK_TEMP);
		renderer_upload_8bit_texture_page(tex_menu1, 3);
		mem_stack_release(STACK_TEMP);
		texture_collection_load("\\assets\\models\\ui_tex\\menu2.txc", &tex_menu2, 1, STACK_TEMP);
		renderer_upload_8bit_texture_page(tex_menu2, 4);
		mem_stack_release(STACK_TEMP);
		texture_collection_load("\\assets\\models\\ui_tex\\ui.txc", &tex_ui, 1, STACK_TEMP);
		renderer_upload_8bit_texture_page(tex_ui, 5);
#elif defined(_NDS)
		texture_cpu_t *tex_menu;
		texture_cpu_t *tex_ui;
		mem_stack_release(STACK_TEMP);
		texture_collection_load("\\assets\\models\\ui_tex\\menu_ds.txc", &tex_menu, 1, STACK_TEMP);
		renderer_upload_8bit_texture_page(tex_menu, 4);
		mem_stack_release(STACK_TEMP);
		texture_collection_load("\\assets\\models\\ui_tex\\ui.txc", &tex_ui, 1, STACK_TEMP);
		renderer_upload_8bit_texture_page(tex_ui, 5);
#endif
		mem_stack_release(STACK_TEMP);
		state.title_screen.assets_in_memory = 1;
		mem_stack_release(STACK_MUSIC);
		music_load_soundbank("\\assets\\music\\instr.sbk");
		music_load_sequence("\\assets\\music\\sequence\\level3.dss");
		music_play_sequence(0);
	}
	state.global.fade_level = 255;
}

void state_update_title_screen(int dt) {
    (void)dt;
	renderer_begin_frame(&id_transform);
	input_update();
	ui_render_background();
    ui_render_logo();

	// Draw buttons
	for (int y = 0; y < 3; ++y) {
		// Left handle, right handle, middle bar, text
		pixel32_t color = (pixel32_t){128, 128, 128, 255};
		if (y == state.title_screen.button_selected) {
			if (state.title_screen.button_pressed) {
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

		renderer_draw_text((vec2_t){256*ONE, (148 + (24 * y))*ONE}, text_main_menu[y], 1, 1, white);
	}

	// Handle button navigation
	if (input_pressed(PAD_UP, 0) && state.title_screen.button_selected > 0) {
		state.title_screen.button_selected--;
		state.title_screen.button_pressed = 0;
	}
	if (input_pressed(PAD_DOWN, 0) && state.title_screen.button_selected < 2) {
		state.title_screen.button_selected++;
		state.title_screen.button_pressed = 0;
	}
	if (input_pressed(PAD_CROSS, 0)) {
		state.title_screen.button_pressed = 1;
	}

	// Handle button presses
	if (input_released(PAD_CROSS, 0)) {
		state.title_screen.button_pressed = 0;
		switch (state.title_screen.button_selected) {
			case 0: // start game
				current_state = STATE_IN_GAME;
				break;
			case 1: // settings
				current_state = STATE_SETTINGS;
				break;
			case 2: // credits
				current_state = STATE_CREDITS;
				break;
		}
	}

#ifdef _DEBUG
	char debug_text[64];
	char* curr_pointer = debug_text;
	sprintf(curr_pointer, "DEBUG\n"); curr_pointer += strlen("DEBUG ");
	if (is_pal) {
		sprintf(curr_pointer, "PAL\n");  curr_pointer += strlen("PAL ");
	}
	else {
		sprintf(curr_pointer, "NTSC\n");  curr_pointer += strlen("NTSC ");
	}
#ifdef DEBUG_CAMERA
	sprintf(curr_pointer, "FREECAM\n");  curr_pointer += strlen("FREECAM ");
#endif

	renderer_draw_text((vec2_t){32*ONE, 192*ONE}, debug_text, 0, 0, white);
#endif

	if (input_pressed(PAD_START, 0)) {
		current_state = STATE_IN_GAME;
	} 
	if (state.global.fade_level > 0) {
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
	} 
	renderer_end_frame();
}
void state_exit_title_screen(void) {
	state.global.fade_level = 0;
	while (state.global.fade_level < 255) {
		renderer_begin_frame(&id_transform);
		input_update();
        ui_render_background();
        ui_render_logo();
		renderer_apply_fade(state.global.fade_level);
		renderer_end_frame();
		state.global.fade_level += FADE_SPEED;
		if (current_state == STATE_IN_GAME) music_set_volume(255 - state.global.fade_level);
	}

    // One last frame with the screen blank
	state.global.fade_level = 255;
    renderer_begin_frame(&id_transform);
    renderer_apply_fade(state.global.fade_level);
    renderer_end_frame();
}
