#include <string.h>
#include "main.h"
#include "input.h"
#include "music.h"
#include "renderer.h"

#ifdef _PSX
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxpad.h>

#else
#include "win/psx.h"
#include "win/debug_layer.h"
#endif

#include "camera.h"
#include "fixed_point.h"
#include "player.h"

#include "texture.h"
#include "memory.h"
#include "text.h"

int widescreen = 0;
state_t current_state = STATE_NONE;
state_t prev_state = STATE_NONE;
int should_transition_state = 0;
#define FADE_SPEED 8
#define FADE_SPEED_SLOW 4

struct {
	struct {
		int frame_counter;
		int show_debug;
		int fade_level; // 255 means black, 0 means no fade
	} global;
	struct {
		int button_selected;
		int button_pressed;
	} title_screen;
	struct {
		scalar_t scroll;
	} credits;
	struct {
		transform_t t_level;
		model_t* m_level;
		model_t* m_chaser;
		model_t* m_level_col_dbg;
		collision_mesh_t* m_level_col;
		vislist_t* v_level;
    	bvh_t bvh_level_model;
		player_t player;
	} in_game;
} state;


int main(void) {
	mem_debug();
	// Init systems
	renderer_init();
	input_init();
	init();

	// Init state variables
	memset(&state, 0, sizeof(state));
	state.in_game.t_level = (transform_t){{0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096}};
	state.global.frame_counter = 0;
	state.global.show_debug = 0;

	// Stick deadzone
	input_set_stick_deadzone(36);

	// Let's start here
	current_state = STATE_TITLE_SCREEN;

    while (!renderer_should_close()) {
#ifndef _WIN32
        int delta_time_raw = renderer_get_delta_time_raw();
        if (delta_time_raw > 520) {
            delta_time_raw = 520;
        }
        int delta_time = renderer_convert_dt_raw_to_ms(delta_time_raw);
#else
        int delta_time = renderer_get_delta_time_ms();
        delta_time = scalar_min(delta_time, 40);
#endif
		// If a state change happened, transition between them
		if (current_state != prev_state) {
			// Exit the previous state
			switch(prev_state) {
				case STATE_NONE:
					break;
				case STATE_TITLE_SCREEN:
					state_exit_title_screen();
					break;
				case STATE_CREDITS:
					state_exit_credits();
					break;
				case STATE_SETTINGS:
					state_exit_settings();
					break;
				case STATE_IN_GAME:
					state_exit_in_game();
					break;
			}
			// Enter the current state
			switch(current_state) {
				case STATE_NONE:
					break;
				case STATE_TITLE_SCREEN:
					state_enter_title_screen();
					break;
				case STATE_CREDITS:
					state_enter_credits();
					break;
				case STATE_SETTINGS:
					state_enter_settings();
					break;
				case STATE_IN_GAME:
					state_enter_in_game();
					break;
			}
		}

		// Update the current state
		prev_state = current_state;
		switch(current_state) {
			case STATE_NONE:
				break;
			case STATE_TITLE_SCREEN:
				state_update_title_screen(delta_time);
				break;
			case STATE_CREDITS:
				state_update_credits(delta_time);
				break;
			case STATE_SETTINGS:
				state_update_settings(delta_time);
				break;
			case STATE_IN_GAME:
				state_update_in_game(delta_time);
				break;
		}
	}
#ifndef _PSX
	debug_layer_close();
#endif
    return 0;
}

void init(void) {
#ifdef _PSX
	// Init CD
	SpuInit();
	CdInit();


#endif
}

void state_enter_title_screen(void) {
	// Load graphics data
	texture_cpu_t *tex_menu1;
	texture_cpu_t *tex_menu2;
	texture_cpu_t *tex_ui;
	texture_collection_load("\\ASSETS\\MODELS\\UI_TEX\\MENU1.TXC;1", &tex_menu1, 1);
	texture_collection_load("\\ASSETS\\MODELS\\UI_TEX\\MENU2.TXC;1", &tex_menu2, 1);
	texture_collection_load("\\ASSETS\\MODELS\\UI_TEX\\UI.TXC;1", &tex_ui, 1);
	render_upload_8bit_texture_page(tex_menu1, 3);
	render_upload_8bit_texture_page(tex_menu2, 4);
	render_upload_8bit_texture_page(tex_ui, 5);
	mem_free_scheduled_frees();
	music_load_soundbank("\\ASSETS\\MUSIC\\INSTR.SBK;1");
	music_load_sequence("\\ASSETS\\MUSIC\\SEQUENCE\\LEVEL3.DSS;1");
	music_play_sequence(0);
	state.global.fade_level = 255;
}

void state_update_title_screen(int dt) {
	renderer_begin_frame(&id_transform);
	input_update();
	// Draw background
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);

	// Draw Sub Nivis logo
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 85*ONE}, (vec2_t){128*ONE, 72*ONE}, (vec2_t){0*ONE, 184*ONE}, (vec2_t){128*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);

	// Draw buttons
	for (int y = 0; y < 3; ++y) {
		// Left handle, right handle, middle bar, text
		pixel32_t color = (pixel32_t){128, 128, 128};
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

		renderer_draw_text((vec2_t){256*ONE, (148 + (24 * y))*ONE}, text_main_menu[y], 1, 1);
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
	if (input_pressed(PAD_CIRCLE, 0) || input_pressed(PAD_CROSS, 0)) {
		state.title_screen.button_pressed = 1;
	}

	// Handle button presses
	if (input_released(PAD_CROSS, 0) || input_released(PAD_CIRCLE, 0)) {
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
#ifdef PAL
	sprintf(curr_pointer, "PAL\n");  curr_pointer += strlen("PAL ");
#else
	sprintf(curr_pointer, "NTSC\n");  curr_pointer += strlen("NTSC ");
#endif
#ifdef DEBUG_CAMERA
	sprintf(curr_pointer, "FREECAM\n");  curr_pointer += strlen("FREECAM ");
#endif

	renderer_draw_text((vec2_t){32*ONE, 192*ONE}, debug_text, 0, 0);
#endif

	if (input_pressed(PAD_START, 0)) {
		current_state = STATE_IN_GAME;
	} 
	if (state.global.fade_level > 0) {
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
	} 
	music_tick(16);
	renderer_end_frame();
}
void state_exit_title_screen(void) {
	state.global.fade_level = 0;
	while (state.global.fade_level < 255) {
		renderer_begin_frame(&id_transform);
		input_update();

		// Draw background
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);

		// Draw Sub Nivis logo
		renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 85*ONE}, (vec2_t){128*ONE, 72*ONE}, (vec2_t){0*ONE, 184*ONE}, (vec2_t){128*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);

		renderer_apply_fade(state.global.fade_level);

		renderer_end_frame();
		state.global.fade_level += FADE_SPEED_SLOW;
		if (current_state == STATE_IN_GAME) music_set_volume(255 - state.global.fade_level);
		music_tick(16);
	}
	state.global.fade_level = 255;
}

void state_enter_in_game(void) {

    // Init player
    state.in_game.player.position.x = 11705653 / 2;
   	state.in_game.player.position.y = 11413985 / 2;
    state.in_game.player.position.z = 2112866  / 2;
	state.in_game.player.rotation.y = 4096 * 16;

	// Load models
    state.in_game.m_level = model_load("\\ASSETS\\MODELS\\LEVEL.MSH;1");
    state.in_game.m_chaser = model_load("\\ASSETS\\MODELS\\ENTITY.MSH;1");
    state.in_game.m_level_col_dbg = model_load_collision_debug("\\ASSETS\\MODELS\\LEVEL.COL;1");
    state.in_game.m_level_col = model_load_collision("\\ASSETS\\MODELS\\LEVEL.COL;1");
	state.in_game.v_level = model_load_vislist("\\ASSETS\\MODELS\\LEVEL.VIS;1");

	// Generate collision BVH
    bvh_from_model(&state.in_game.bvh_level_model, state.in_game.m_level_col);

	// Load textures
	texture_cpu_t *tex_level;
	texture_cpu_t *entity_textures;
	const uint32_t n_level_textures = texture_collection_load("\\ASSETS\\MODELS\\LEVEL.TXC;1", &tex_level, 1);
	const uint32_t n_entity_textures = texture_collection_load("\\ASSETS\\MODELS\\ENTITY.TXC;1", &entity_textures, 1);
	for (uint8_t i = 0; i < n_level_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i);
	}
	mem_free_scheduled_frees();
	

	// Start music
	music_stop();
	music_load_soundbank("\\ASSETS\\MUSIC\\INSTR.SBK;1");
	music_load_sequence("\\ASSETS\\MUSIC\\SEQUENCE\\SUBNIVIS.DSS;1");
	music_play_sequence(0);
	
	music_set_volume(255);
	
	mem_debug();
}

void state_update_in_game(int dt) {
	renderer_begin_frame(&state.in_game.player.transform);

	// Draw crosshair
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 128*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){96*ONE, 40*ONE}, (vec2_t){127*ONE, 59*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);

	// Draw HUD - background
	renderer_draw_2d_quad_axis_aligned((vec2_t){(121 - 16)*ONE, 236*ONE}, (vec2_t){50*ONE, 40*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){50*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(260 - 16)*ONE, 236*ONE}, (vec2_t){228*ONE, 40*ONE}, (vec2_t){51*ONE, 0*ONE}, (vec2_t){51*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(413 - 16)*ONE, 236*ONE}, (vec2_t){80*ONE, 40*ONE}, (vec2_t){51*ONE, 0*ONE}, (vec2_t){129*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	
	// Draw HUD - gauges
	renderer_draw_2d_quad_axis_aligned((vec2_t){(138 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){64*ONE, 40*ONE}, (vec2_t){96*ONE, 60*ONE}, (pixel32_t){128, 128, 128}, 1, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(226 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){32*ONE, 40*ONE}, (vec2_t){64*ONE, 60*ONE}, (pixel32_t){128, 128, 128}, 1, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(314 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){0*ONE, 40*ONE}, (vec2_t){32*ONE, 60*ONE}, (pixel32_t){128, 128, 128}, 1, 5, 1);

	int n_sections = player_get_level_section(&state.in_game.player, state.in_game.m_level);
	state.global.frame_counter += dt;
	if (input_pressed(PAD_SELECT, 0)) state.global.show_debug = !state.global.show_debug;
	if (state.global.show_debug) {
		PROFILE("input", input_update(), 1);
		PROFILE("render", renderer_draw_model_shaded(state.in_game.m_level, &state.in_game.t_level, state.in_game.v_level, 0), 1);
		PROFILE("player", player_update(&state.in_game.player, &state.in_game.bvh_level_model, dt), 1);
		PROFILE("music", music_tick(16), 1);
		//FntPrint(-1, "sections: ");
		//for (int i = 0; i < n_sections; ++i) FntPrint(-1, "%i, ", sections[i]);
		//FntPrint(-1, "\n");
		//FntPrint(-1, "dt: %i\n", dt);
		//FntPrint(-1, "meshes drawn: %i / %i\n", n_meshes_drawn, n_meshes_total);
		collision_clear_stats();
		//FntFlush(-1);
	}
	else {
		input_update();
		renderer_draw_model_shaded(state.in_game.m_level, &state.in_game.t_level, state.in_game.v_level, 0);
		//renderer_draw_entity_shaded(&m_chaser->meshes[0], &t_level, 3);
		player_update(&state.in_game.player, &state.in_game.bvh_level_model, dt);
		music_tick(16);
	}
	if (state.global.fade_level > 0) {
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
	}
	renderer_end_frame();
}
void state_exit_in_game(void) {
	return;
}

void state_enter_settings(void) {
	state.global.fade_level = 255;
	state.title_screen.button_pressed = 0;
	while (state.global.fade_level > 0) {
		renderer_begin_frame(&id_transform);
		music_tick(16);
		// Draw background
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 0;
}
void state_update_settings(int dt) {
	renderer_begin_frame(&id_transform);
	music_tick(16);
	// Draw background
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);
	renderer_apply_fade(state.global.fade_level);
	state.global.fade_level -= FADE_SPEED;
	renderer_end_frame();
	return;
}
void state_exit_settings(void) {
	return;
}

void state_enter_credits(void) {
	state.global.fade_level = 255;
	while (state.global.fade_level > 0) {
		renderer_begin_frame(&id_transform);
		music_tick(16);
		// Draw background
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 0;
	state.credits.scroll = 0;
	return;
}
void state_update_credits(int dt) {
	renderer_begin_frame(&id_transform);

	// Draw background
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);

	// Draw text
	for (int i = 0; i < sizeof(text_credits) / sizeof(text_credits[0]); ++i) {
		renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1);
	}

	char buf[64];
	sprintf(buf, "scroll: %i", state.credits.scroll);
	renderer_draw_text((vec2_t){32*ONE, 64*ONE}, buf, 0, 0);

	// Scroll text
	state.credits.scroll -= 16 * 80;

	//music_tick(16);
	renderer_end_frame();
	input_update();
	if (input_pressed(0xFFFF, 0) || state.credits.scroll < -5800000) {
		current_state = STATE_TITLE_SCREEN;
	}
	return;
}
void state_exit_credits(void) {
	music_stop();

	// Fade
	state.global.fade_level = 0;
	while (state.global.fade_level < 255) {
		renderer_begin_frame(&id_transform);
		
		// Draw background
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);

		// Draw text
		renderer_begin_frame(&id_transform);
		for (int i = 0; i < sizeof(text_credits) / sizeof(text_credits[0]); ++i) {
			renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1);
		}
		input_update();
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level += FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 255;
}