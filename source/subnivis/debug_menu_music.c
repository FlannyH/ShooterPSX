#include "main.h"

#include "engine/input_mapping.h"
#include "engine/renderer.h"
#include "engine/music.h"
#include "subnivis/input_map.h"
#include "subnivis/text.h"
#include "misc.h"
#include "ui.h"

void state_enter_debug_menu_music(void) {
    state_enter_title_screen();
    renderer_stop_fade();
	state.global.state_to_return_to = get_prev_state();
	state.title_screen.button_pressed = 0;
    for (int i = 0; i < 2; ++i) {
        renderer_begin_frame(&id_transform);
        ui_render_background();
    	renderer_end_frame();
    	input_update();
    }
}

extern volume_env_t vol_envs[N_SPU_CHANNELS];

void state_update_debug_menu_music(scalar_t dt) {
	(void)dt;

	char* songs[] = {
	    "audio/music/black.dss",
	    "audio/music/combust.dss",
	    "audio/music/e1m1.dss",
	    "audio/music/e3m3.dss",
	    "audio/music/energia.dss",
	    "audio/music/jungle.dss",
	    "audio/music/justice.dss",
	    "audio/music/level1.dss",
	    "audio/music/level2.dss",
	    "audio/music/level3.dss",
	    "audio/music/pitchtst.dss",
	    "audio/music/subnivis.dss",
	    "audio/music/newsong.dss",
		0
	};
	char* sbk[] = {
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/jungle.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/instr.sbk",
	    "audio/newsong.sbk",
		0
	};

	static int music_selected = 0;

	if (input_mapping_pressed(IM_MENU_UP, 0)) music_selected++;
	if (input_mapping_pressed(IM_MENU_DOWN, 0)) music_selected--;

	if (music_selected < 0) music_selected = 0;
	if (songs[music_selected] == 0) --music_selected;

	// SFX debug
	if (input_mapping_pressed(IM_MENU_LEFT, 0)) {
		state.debug_menu_music.sfx_id--;
		if (state.debug_menu_music.sfx_id < 0) state.debug_menu_music.sfx_id = 0;
	}
	if (input_mapping_pressed(IM_MENU_RIGHT, 0)) {
		state.debug_menu_music.sfx_id++;
	}
	if (input_mapping_pressed(IM_MENU_TAB, 0)) {
		audio_play_sound(state.debug_menu_music.sfx_id, 0, 1, (vec3_t){0, 0, 0}, ONE * 16);
	}
	if (input_mapping_pressed(IM_MENU_PREVIEW, 0)) {
		music_stop();
		mem_stack_release(STACK_TEMP);
		mem_stack_release(STACK_MUSIC);
		audio_load_soundbank(sbk[music_selected], SOUNDBANK_TYPE_MUSIC);
		mem_stack_release(STACK_TEMP);
		audio_load_soundbank("audio/sfx.sbk", SOUNDBANK_TYPE_SFX);
		mem_stack_release(STACK_TEMP);
		music_load_sequence(songs[music_selected]);
		music_set_volume(255);
		music_play_sequence(0);
	}

	renderer_begin_frame(&id_transform);

    ui_render_background();

	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, "MUSIC DEBUG", 1, 1, white);

	char text[32];
	sprintf(text, "sfx: %i\nmusic: %s", state.debug_menu_music.sfx_id, &songs[music_selected][12]);
	renderer_draw_text((vec2_t){32*ONE, 32*ONE}, text, 1, 0, white);

	// exiting
	renderer_draw_text((vec2_t){120*ONE, 216*ONE}, "EXIT", 1, 1, white);
	renderer_draw_2d_quad_axis_aligned((vec2_t){120*ONE, 216*ONE}, (vec2_t){200*ONE, 20*ONE}, (vec2_t){0*ONE, 144*ONE}, (vec2_t){192*ONE, 164*ONE}, white, 2, 5, TEX_CAT_MISC);
	if (input_mapping_pressed(IM_MENU_GO, 0)) {
	    set_current_state(STATE_DEBUG_MENU_MAIN);
	}

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
    for (int i = 0; i < 2; ++i) {
        renderer_begin_frame(&id_transform);
        ui_render_background();
    	renderer_end_frame();
    	input_update();
    }
}
