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

#ifdef _PC
#include "pc/psx.h"
#include "pc/debug_layer.h"
#endif

#ifdef _NDS
#include "nds/psx.h"
#include <filesystem.h>
#endif

void state_enter_debug_menu_music(void) {
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

extern volume_env_t vol_envs[N_SPU_CHANNELS];

void state_update_debug_menu_music(int dt) {
	(void)dt;
	input_update();

	char* songs[] = {
		"audio/music/level1.dss",
		"audio/music/level2.dss",
		"audio/music/level3.dss",
		"audio/music/subnivis.dss",
	};

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
				audio_load_soundbank("audio/instr.sbk", SOUNDBANK_TYPE_MUSIC);
				mem_stack_release(STACK_TEMP);
				audio_load_soundbank("audio/sfx.sbk", SOUNDBANK_TYPE_SFX);
				mem_stack_release(STACK_TEMP);
				music_load_sequence(songs[state.debug_menu_music.button_selected]);
				music_set_volume(255);
				music_play_sequence(0);
				break;
			case 4:
				set_current_state(STATE_DEBUG_MENU_MAIN);
				break;
		}
	}

	// SFX debug
	if (input_pressed(PAD_LEFT, 0)) {
		state.debug_menu_music.sfx_id--;
		if (state.debug_menu_music.sfx_id < 0) state.debug_menu_music.sfx_id = 0;
	}
	if (input_pressed(PAD_RIGHT, 0)) {
		state.debug_menu_music.sfx_id++;
	}
	if (input_pressed(PAD_SQUARE, 0)) {
		audio_play_sound(state.debug_menu_music.sfx_id, 0, 1, (vec3_t){0, 0, 0}, ONE * 16);
	}

	renderer_begin_frame(&id_transform);
    
    ui_render_background();
	
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_debug_menu_music[0], 1, 1, white);

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
		renderer_draw_text((vec2_t){40*ONE, (96 + (24 * i))*ONE}, text_debug_menu_music[i+1], 1, 0, white);
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, (96 + (24 * i))*ONE}, (vec2_t){224*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, color, 2, 5, 1);
	}

	char text[32];
	sprintf(text, "sfx: %i", state.debug_menu_music.sfx_id);
	renderer_draw_text((vec2_t){32*ONE, 32*ONE}, text, 1, 0, white);

	// Draw black background behind debug for readability sake
	renderer_draw_2d_quad_axis_aligned((vec2_t){376*ONE, 156 * ONE}, (vec2_t){258*ONE, 160*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, black, 2, NO_TEXTURE, 0);
	renderer_draw_text((vec2_t){256 * ONE, 80 * ONE}, "ADSR debug", 0, 0, white);

	for (int i = 0; i < N_SPU_CHANNELS; ++i) {
		const char* stage_names[] = {
			"------",
			"D-----",
			"-A----",
			"--H---",
			"---D--",
			"----S-",
			"-----R",
		};
		// base text
		sprintf(text, "%02i:", i);
		renderer_draw_text((vec2_t){(256 + (128 * (i / (N_SPU_CHANNELS / 2))))*ONE, (96 + 9*(i % (N_SPU_CHANNELS / 2)))*ONE}, text, 0, 0, white);

		// volume with color fade
		scalar_t adsr_volume_visual = vol_envs[i].adsr_volume / (ADSR_VOLUME_ONE / ONE);
		adsr_volume_visual = scalar_mul(adsr_volume_visual, adsr_volume_visual);

		const pixel32_t color = {
			.r = (uint8_t)scalar_clamp(adsr_volume_visual / (ONE / 255), 128, 255),
			.g = (uint8_t)scalar_clamp(adsr_volume_visual / (ONE / 255), 128, 255),
			.b = (uint8_t)scalar_clamp(255 - adsr_volume_visual / (ONE / 255), 128, 255),
			.a = 255,
		};
		sprintf(text, "    %3i%%", vol_envs[i].adsr_volume / (ADSR_VOLUME_ONE / 100));
		renderer_draw_text((vec2_t){(256 + (128 * (i / (N_SPU_CHANNELS / 2))))*ONE, (96 + 9*(i % (N_SPU_CHANNELS / 2)))*ONE}, text, 0, 0, color);

		// volume with color fade
		sprintf(text, "         (%s)", stage_names[vol_envs[i].stage]);
		renderer_draw_text((vec2_t){(256 + (128 * (i / (N_SPU_CHANNELS / 2))))*ONE, (96 + 9*(i % (N_SPU_CHANNELS / 2)))*ONE}, text, 0, 0, white);
	}

	renderer_end_frame();
	return;
}

void state_exit_debug_menu_music(void) {
	renderer_start_fade_out(FADE_SPEED);

	while (renderer_is_fading()) {
		renderer_begin_frame(&id_transform);
		input_update();
		ui_render_background();
		renderer_end_frame();
	}
}
