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

#include "cheats.h"
#include "camera.h"
#include "fixed_point.h"
#include "player.h"

#include "texture.h"
#include "memory.h"
#include "text.h"
#include "timer.h"
#include "entity.h"
#include "random.h"
#include "entities/door.h"
#include "entities/pickup.h"
#include "entities/crate.h"

int widescreen = 0;
extern int vsync_enable;
extern int is_pal;
state_t current_state = STATE_NONE;
state_t prev_state = STATE_NONE;
int should_transition_state = 0;
#define FADE_SPEED 18
#define FADE_SPEED_SLOW 5

typedef struct {
	struct {
		int frame_counter;
		int time_counter;
		int show_debug;
		int fade_level; // 255 means black, 0 means no fade
		state_t state_to_return_to;
	} global;
	struct {
		int doom_mode : 1;
	} cheats;
	struct {
		int button_selected;
		int button_pressed;
		int assets_in_memory;
	} title_screen;
	struct {
		int button_selected;
		int button_pressed;
	} settings;
	struct {
		scalar_t scroll;
	} credits;
	struct {
		transform_t t_level;
		model_t* m_level;
		model_t* m_entity;
		model_t* m_level_col_dbg;
		model_t* m_weapons;
		collision_mesh_t* m_level_col;
		vislist_t* v_level;
    	bvh_t bvh_level_model;
		player_t player;
		scalar_t gun_animation_timer;
		scalar_t gun_animation_timer_sqrt;
		scalar_t screen_shake_intensity_rotation;
		scalar_t screen_shake_dampening_rotation;
		scalar_t screen_shake_intensity_position;
		scalar_t screen_shake_dampening_position;
	} in_game;
	struct {
		int button_selected;
		int button_pressed;
	} pause_menu;
	struct {
		vec3_t shoot_origin_position;
		vec3_t shoot_hit_position;
	} debug;
} state_vars_t;

state_vars_t state;


int main(void) {
	mem_debug();
	// Init systems
	renderer_init();
	input_init();
	init();
	setup_timers();
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
		state.global.time_counter += delta_time;
		// If a state change happened, transition between them
		if (current_state != prev_state) {
			// Exit the previous state
			switch(prev_state) {
				case STATE_NONE:			/* nothing */ 					break;
				case STATE_TITLE_SCREEN: 	state_exit_title_screen(); 		break;
				case STATE_CREDITS: 		state_exit_credits(); 			break;
				case STATE_SETTINGS: 		state_exit_settings(); 			break;
				case STATE_IN_GAME: 		state_exit_in_game(); 			break;
				case STATE_PAUSE_MENU: 		state_exit_pause_menu(); 		break;
			}
			// Enter the current state
			switch(current_state) {
				case STATE_NONE:			/* nothing */ 					break;
				case STATE_TITLE_SCREEN:	state_enter_title_screen();		break;
				case STATE_CREDITS:			state_enter_credits();			break;
				case STATE_SETTINGS:		state_enter_settings();			break;
				case STATE_IN_GAME:			state_enter_in_game();			break;
				case STATE_PAUSE_MENU:		state_enter_pause_menu();		break;
			}
		}

		// Update the current state
		prev_state = current_state;
		switch(current_state) {
			case STATE_NONE:			/* nothing */ 							break;
			case STATE_TITLE_SCREEN:	state_update_title_screen(delta_time);	break;
			case STATE_CREDITS:			state_update_credits(delta_time);		break;
			case STATE_SETTINGS:		state_update_settings(delta_time);		break;
			case STATE_IN_GAME:			state_update_in_game(delta_time);		break;
			case STATE_PAUSE_MENU:		state_update_pause_menu(delta_time);	break;
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
	if (!state.title_screen.assets_in_memory) {
		// Load graphics data
		texture_cpu_t *tex_menu1;
		texture_cpu_t *tex_menu2;
		texture_cpu_t *tex_ui;
		mem_stack_release(STACK_TEMP);
		texture_collection_load("\\ASSETS\\MODELS\\UI_TEX\\MENU1.TXC;1", &tex_menu1, 1, STACK_TEMP);
		texture_collection_load("\\ASSETS\\MODELS\\UI_TEX\\MENU2.TXC;1", &tex_menu2, 1, STACK_TEMP);
		texture_collection_load("\\ASSETS\\MODELS\\UI_TEX\\UI.TXC;1", &tex_ui, 1, STACK_TEMP);
		render_upload_8bit_texture_page(tex_menu1, 3);
		render_upload_8bit_texture_page(tex_menu2, 4);
		render_upload_8bit_texture_page(tex_ui, 5);
		mem_stack_release(STACK_TEMP);
		state.title_screen.assets_in_memory = 1;
		mem_stack_release(STACK_MUSIC);
		music_load_soundbank("\\ASSETS\\MUSIC\\INSTR.SBK;1");
		music_load_sequence("\\ASSETS\\MUSIC\\SEQUENCE\\LEVEL3.DSS;1");
		music_play_sequence(0);
	}
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

		// Draw background
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);

		// Draw Sub Nivis logo
		renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 85*ONE}, (vec2_t){128*ONE, 72*ONE}, (vec2_t){0*ONE, 184*ONE}, (vec2_t){128*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);

		renderer_apply_fade(state.global.fade_level);

		renderer_end_frame();
		state.global.fade_level += FADE_SPEED;
		if (current_state == STATE_IN_GAME) music_set_volume(255 - state.global.fade_level);
	}
	state.global.fade_level = 255;
}

void state_enter_in_game(void) {
	if (prev_state == STATE_PAUSE_MENU) return;

	mem_stack_release(STACK_LEVEL);

	state.title_screen.assets_in_memory = 0;

    // Init player
    state.in_game.player.position.x = 11705653 / 2;
   	state.in_game.player.position.y = 11413985 / 2;
    state.in_game.player.position.z = 2112866  / 2;
	state.in_game.player.rotation.y = 4096 * 16;
    //state.in_game.player.position.x = 0;
   	//state.in_game.player.position.y = 0;
    //state.in_game.player.position.z = 0;

	// Load models
    state.in_game.m_level = model_load("\\ASSETS\\MODELS\\LEVEL.MSH;1", 1, STACK_LEVEL);
    state.in_game.m_level_col_dbg = model_load_collision_debug("\\ASSETS\\MODELS\\LEVEL.COL;1", 1, STACK_LEVEL);
    state.in_game.m_level_col = model_load_collision("\\ASSETS\\MODELS\\LEVEL.COL;1", 1, STACK_LEVEL);
	state.in_game.v_level = model_load_vislist("\\ASSETS\\MODELS\\LEVEL.VIS;1", 1, STACK_LEVEL);
	state.in_game.m_weapons = model_load("\\ASSETS\\MODELS\\WEAPONS.MSH", 1, STACK_LEVEL);

	entity_init();

	// debug entity
	entity_door_t* door;
	door = entity_door_new(); door->is_locked = 0; door->entity_header.position = (vec3_t){1325 * ONE, 1372 * ONE, 1699 * ONE}; door->is_big_door = 0; door->is_rotated = 0;
	door = entity_door_new(); door->is_locked = 0; door->entity_header.position = (vec3_t){1209 * ONE, 1372 * ONE, 1699 * ONE}; door->is_big_door = 0; door->is_rotated = 0;
	door = entity_door_new(); door->is_locked = 1; door->entity_header.position = (vec3_t){1121 * ONE, 1372 * ONE, 1755 * ONE}; door->is_big_door = 0; door->is_rotated = 1;
	door = entity_door_new(); door->is_locked = 1; door->entity_header.position = (vec3_t){1053 * ONE, 1372 * ONE, 1682 * ONE}; door->is_big_door = 1; door->is_rotated = 0;
	door = entity_door_new(); door->is_locked = 0; door->entity_header.position = (vec3_t){926 * ONE,  1164 * ONE, 1675 * ONE}; door->is_big_door = 0; door->is_rotated = 0;
	door = entity_door_new(); door->is_locked = 1; door->entity_header.position = (vec3_t){501 * ONE,  1015 * ONE, 1944 * ONE}; door->is_big_door = 0; door->is_rotated = 1;

	entity_pickup_t* pickup;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*0)*ONE, 1294*ONE, (465+64*0)*ONE}; pickup->type = PICKUP_TYPE_AMMO_BIG;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*0)*ONE, 1294*ONE, (465+64*1)*ONE}; pickup->type = PICKUP_TYPE_ARMOR_BIG;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*0)*ONE, 1294*ONE, (465+64*2)*ONE}; pickup->type = PICKUP_TYPE_HEALTH_BIG;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*1)*ONE, 1294*ONE, (465+64*0)*ONE}; pickup->type = PICKUP_TYPE_AMMO_SMALL;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*1)*ONE, 1294*ONE, (465+64*1)*ONE}; pickup->type = PICKUP_TYPE_ARMOR_SMALL;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*1)*ONE, 1294*ONE, (465+64*2)*ONE}; pickup->type = PICKUP_TYPE_HEALTH_SMALL;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*2)*ONE, 1294*ONE, (465+64*0)*ONE}; pickup->type = PICKUP_TYPE_KEY_BLUE;
	pickup = entity_pickup_new(); pickup->entity_header.position = (vec3_t){(1338 + 64*2)*ONE, 1294*ONE, (465+64*1)*ONE}; pickup->type = PICKUP_TYPE_KEY_YELLOW;

	entity_crate_t* crate;
	crate = entity_crate_new(); crate->entity_header.position = (vec3_t){(1338 + 64*3)*ONE, 1294*ONE, (465+64*0)*ONE}; crate->pickup_to_spawn = PICKUP_TYPE_HEALTH_BIG;


	// Generate collision BVH
    bvh_from_model(&state.in_game.bvh_level_model, state.in_game.m_level_col);

	// Load textures
	texture_cpu_t *tex_level;
	texture_cpu_t *entity_textures;
	texture_cpu_t *weapon_textures;
	mem_stack_release(STACK_TEMP);
	printf("occupied STACK_TEMP: %i / %i\n", mem_stack_get_occupied(STACK_TEMP), mem_stack_get_size(STACK_TEMP));
	const uint32_t n_level_textures = texture_collection_load("\\ASSETS\\MODELS\\LEVEL.TXC;1", &tex_level, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_level_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i);
	}
	const uint32_t n_entity_textures = texture_collection_load("\\ASSETS\\MODELS\\ENTITY.TXC;1", &entity_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
	    renderer_upload_texture(&entity_textures[i], i + 64);
	}
	const uint32_t n_weapons_textures = texture_collection_load("\\ASSETS\\MODELS\\WEAPONS.TXC;1", &weapon_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_weapons_textures; ++i) {
	    renderer_upload_texture(&weapon_textures[i], i + 100);
	}
	mem_stack_release(STACK_TEMP);

	// Start music
	music_stop();
	music_load_soundbank("\\ASSETS\\MUSIC\\INSTR.SBK;1");
	music_load_sequence("\\ASSETS\\MUSIC\\SEQUENCE\\SUBNIVIS.DSS;1");
	music_play_sequence(0);
	
	music_set_volume(255);
#ifdef _DEBUG
#ifdef _PSX
	FntLoad(320,256);
	FntOpen(32, 32, 256, 192, 0, 512);
#endif
#endif
	
	mem_debug();

	state.in_game.player.has_key_blue = 0;
	state.in_game.player.has_key_yellow = 0;
	state.in_game.player.health = 40;
	state.in_game.player.armor = 0;
	state.in_game.player.ammo = 0;
}

void state_update_in_game(int dt) {
	if (state.in_game.screen_shake_intensity_position > 0) {
		state.in_game.screen_shake_intensity_position -= state.in_game.screen_shake_dampening_position * dt;
		if (state.in_game.screen_shake_intensity_position < 0) {
			state.in_game.screen_shake_intensity_position = 0;
		}
	}
	if (state.in_game.screen_shake_intensity_rotation > 0) {
		state.in_game.screen_shake_intensity_rotation -= state.in_game.screen_shake_dampening_rotation * dt;
		if (state.in_game.screen_shake_intensity_rotation < 0) {
			state.in_game.screen_shake_intensity_rotation = 0;
		}
	}
	transform_t camera_transform = state.in_game.player.transform;
	camera_transform.rotation.vx += scalar_mul((random() % 8192) - 4096, state.in_game.screen_shake_intensity_rotation);
	camera_transform.rotation.vy += scalar_mul((random() % 8192) - 4096, state.in_game.screen_shake_intensity_rotation);
	camera_transform.position.vx += scalar_mul((random() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	camera_transform.position.vy += scalar_mul((random() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	camera_transform.position.vz += scalar_mul((random() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	renderer_begin_frame(&camera_transform);

	// Draw crosshair
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, 128*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){96*ONE, 40*ONE}, (vec2_t){127*ONE, 59*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (128 + 8*(!is_pal))*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){96*ONE, 40*ONE}, (vec2_t){127*ONE, 59*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);

	// Draw HUD - background
	renderer_draw_2d_quad_axis_aligned((vec2_t){(121 - 16)*ONE, 236*ONE}, (vec2_t){50*ONE, 40*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){50*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(260 - 16)*ONE, 236*ONE}, (vec2_t){228*ONE, 40*ONE}, (vec2_t){51*ONE, 0*ONE}, (vec2_t){51*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(413 - 16)*ONE, 236*ONE}, (vec2_t){80*ONE, 40*ONE}, (vec2_t){51*ONE, 0*ONE}, (vec2_t){129*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 2, 5, 1);
	
	// Draw HUD - gauges
	renderer_draw_2d_quad_axis_aligned((vec2_t){(138 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){64*ONE, 40*ONE}, (vec2_t){96*ONE, 60*ONE}, (pixel32_t){128, 128, 128}, 1, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(226 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){32*ONE, 40*ONE}, (vec2_t){64*ONE, 60*ONE}, (pixel32_t){128, 128, 128}, 1, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(314 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){0*ONE, 40*ONE}, (vec2_t){32*ONE, 60*ONE}, (pixel32_t){128, 128, 128}, 1, 5, 1);

	// Draw HUD - gauge text
	char gauge_text_buffer[4];
	snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.health);
	renderer_draw_text((vec2_t){(138 + 24 - 16)*ONE, 236*ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0});
	snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.armor);
	renderer_draw_text((vec2_t){(226 + 24 - 16)*ONE, 236*ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0});
	snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.ammo);
	renderer_draw_text((vec2_t){(314 + 24 - 16)*ONE, 236*ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0});

	// Draw HUD - keycards
	if (state.in_game.player.has_key_blue) renderer_draw_2d_quad_axis_aligned((vec2_t){(256 - 80 - 40)*ONE, 210*ONE}, (vec2_t){31*ONE, 20*ONE}, (vec2_t){194*ONE, 0*ONE}, (vec2_t){255*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 3, 5, 1);
	if (state.in_game.player.has_key_yellow) renderer_draw_2d_quad_axis_aligned((vec2_t){(256 + 80)*ONE, 210*ONE}, (vec2_t){31*ONE, 20*ONE}, (vec2_t){130*ONE, 0*ONE}, (vec2_t){193*ONE, 40*ONE}, (pixel32_t){128, 128, 128}, 3, 5, 1);

	int n_sections = player_get_level_section(&state.in_game.player, state.in_game.m_level);
	state.global.frame_counter += dt;
#ifdef _DEBUG
	if (input_pressed(PAD_SELECT, 0)) state.global.show_debug = !state.global.show_debug;
	if (state.global.show_debug) {
		PROFILE("input", input_update(), 1);
		PROFILE("lvl_gfx", renderer_draw_model_shaded(state.in_game.m_level, &state.in_game.t_level, state.in_game.v_level, 0), 1);
		PROFILE("entity", entity_update_all(&state.in_game.player, dt), 1);
		PROFILE("player", player_update(&state.in_game.player, &state.in_game.bvh_level_model, dt, state.global.time_counter), 1);
		FntPrint(-1, "sections: ");
		for (int i = 0; i < n_sections; ++i) FntPrint(-1, "%i, ", sections[i]);
		FntPrint(-1, "\n");
		FntPrint(-1, "dt: %i\n", dt);
		FntPrint(-1, "meshes drawn: %i / %i\n", n_meshes_drawn, n_meshes_total);
		FntPrint(-1, "polygons drawn: %i\n", n_polygons_drawn);
		FntPrint(-1, "primitive occ.: %i / %i KiB\n", primitive_occupation / 1024, 128);
		FntPrint(-1, "frame: %i\n", state.global.frame_counter);
		FntPrint(-1, "time: %i.%03i\n", state.global.time_counter / 1000, state.global.time_counter % 1000);
		FntPrint(-1, "player pos: %i, %i, %i\n", 
			state.in_game.player.position.x / 4096, 
			state.in_game.player.position.y / 4096, 
			state.in_game.player.position.z / 4096
		);
		FntPrint(-1, "player velocity: %i, %i, %i\n", 
			state.in_game.player.velocity.x, 
			state.in_game.player.velocity.y, 
			state.in_game.player.velocity.z
		);
		FntPrint(-1, "origin: %i, %i, %i\n", 
			state.debug.shoot_origin_position.x / 4096, 
			state.debug.shoot_origin_position.y / 4096, 
			state.debug.shoot_origin_position.z / 4096
		);
		FntPrint(-1, "hit: %i, %i, %i\n", 
			state.debug.shoot_hit_position.x / 4096, 
			state.debug.shoot_hit_position.y / 4096, 
			state.debug.shoot_hit_position.z / 4096
		);
		FntPrint(-1, "gun anim timer: %i\n", state.in_game.gun_animation_timer);
		for (int i = 0; i < N_STACK_TYPES; ++i) {
			FntPrint(-1, "%s: %i / %i KiB (%i%%)\n", 
				stack_names[i],
				mem_stack_get_occupied(i) / KiB,
				mem_stack_get_size(i) / KiB, 
				(mem_stack_get_occupied(i) * 100) / mem_stack_get_size(i)
			);
		}
		collision_clear_stats();
		FntFlush(-1);
	}
	else {
#endif
		input_update();
		renderer_draw_model_shaded(state.in_game.m_level, &state.in_game.t_level, state.in_game.v_level, 0);
		entity_update_all(&state.in_game.player, dt);
		player_update(&state.in_game.player, &state.in_game.bvh_level_model, dt, state.global.time_counter);
#ifdef _DEBUG
	}
#endif
	if (state.global.fade_level > 0) {
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level -= FADE_SPEED;
	}
	if (input_pressed(PAD_START, 0)) {
		current_state = STATE_PAUSE_MENU;
	}

	// Play shoot animation
	if (state.in_game.gun_animation_timer > 0) {
		state.in_game.gun_animation_timer -= dt * 16; 
		if (state.in_game.gun_animation_timer < 0) {
			state.in_game.gun_animation_timer = 0;
		}
		state.in_game.gun_animation_timer_sqrt = scalar_mul(state.in_game.gun_animation_timer, state.in_game.gun_animation_timer);
	}
	else {
		if (input_held(PAD_R2, 0) && state.in_game.player.ammo > 0) {
			// Cast ray through scene and handle entity collisions
			scalar_t cosx = hicos(camera_transform.rotation.vx);
			scalar_t sinx = hisin(camera_transform.rotation.vx);
			scalar_t cosy = hicos(camera_transform.rotation.vy);
			scalar_t siny = hisin(camera_transform.rotation.vy);
			ray_t ray = {
				.length = INT32_MAX,
				.position = (vec3_t){camera_transform.position.vx, camera_transform.position.vy, camera_transform.position.vz},
				.direction = {scalar_mul(siny, cosx), -sinx, scalar_mul(-cosy, cosx)},
			};
			ray.position = vec3_muls(ray.position, -COL_SCALE);
			ray.inv_direction = vec3_div((vec3_t){ONE, ONE, ONE}, ray.direction);
			rayhit_t hit;
			bvh_intersect_ray(&state.in_game.bvh_level_model, ray, &hit);
			for (size_t i = 0; i < entity_n_active_aabb; ++i) {
				rayhit_t entity_hit;
				if (ray_aabb_intersect_fancy(&entity_aabb_queue[i].aabb, ray, &entity_hit)) {
					if (entity_hit.distance < hit.distance) {
						hit = entity_hit;
						hit.type = RAY_HIT_TYPE_ENTITY_HITBOX;
						hit.entity_hitbox.box_index = entity_aabb_queue[i].box_index;
						hit.entity_hitbox.entity_index = entity_aabb_queue[i].entity_index;
					}
				}
			}
			
			if (!is_infinity(hit.distance)) {
				state.debug.shoot_origin_position = ray.position;
				state.debug.shoot_hit_position = hit.position;
			}
			if (!is_infinity(hit.distance) && hit.type == RAY_HIT_TYPE_ENTITY_HITBOX) {
				switch (entity_list[hit.entity_hitbox.entity_index].type) {
					case ENTITY_DOOR: entity_door_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
					case ENTITY_PICKUP: entity_pickup_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
					case ENTITY_CRATE: entity_crate_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
				}
			}

			// Start shoot animation
			state.in_game.gun_animation_timer = 4096;
			state.in_game.screen_shake_intensity_position = 80000;
			state.in_game.screen_shake_dampening_position = 200;
			state.in_game.screen_shake_intensity_rotation = 240;
			state.in_game.screen_shake_dampening_rotation = 2;

			// Reduce ammo count
			state.in_game.player.ammo--;
		}
		if (state.global.show_debug) {
			for (size_t i = 0; i < entity_n_active_aabb; ++i) {
				renderer_debug_draw_aabb(&entity_aabb_queue[i].aabb, pink, &id_transform);
			}
		}
	}

	// We render the player's weapons last, because we need the view matrices to be reset
	if (state.in_game.player.has_gun || 1) {
		transform_t gun_transform;
		vec2_t vel_2d = {state.in_game.player.velocity.x, state.in_game.player.velocity.z};
		scalar_t speed_1d = vec2_magnitude(vel_2d);
		if (state.cheats.doom_mode) {
			gun_transform.position.vx = 0 + (isin(state.global.time_counter * 6) * speed_1d) / (40 * ONE); if (widescreen) gun_transform.position.vx += 30;
			gun_transform.position.vy = 165 + (icos(state.global.time_counter * 12) * speed_1d) / (80 * ONE);
			gun_transform.position.vz = 110;
		} else {
			gun_transform.position.vx = 145 + (isin(state.global.time_counter * 6) * speed_1d) / (40 * ONE); if (widescreen) gun_transform.position.vx += 30;
			gun_transform.position.vy = 135 + (icos(state.global.time_counter * 12) * speed_1d) / (80 * ONE) + scalar_mul(state.in_game.gun_animation_timer_sqrt, 50);
			gun_transform.position.vz = 125 - scalar_mul(state.in_game.gun_animation_timer_sqrt, 225);
		}
		gun_transform.rotation.vx = 0 + scalar_mul(state.in_game.gun_animation_timer_sqrt, 9500);
		gun_transform.rotation.vy = 4096 * 16;
		gun_transform.rotation.vz = 0;
		gun_transform.scale.vx = ONE;
		gun_transform.scale.vy = ONE;
		gun_transform.scale.vz = ONE;
		renderer_draw_mesh_shaded_offset_local(&state.in_game.m_weapons->meshes[1], &gun_transform, 100);
	} 
	// Sword
	{
		transform_t sword_transform;
		vec2_t vel_2d = {state.in_game.player.velocity.x, state.in_game.player.velocity.z};
		scalar_t speed_1d = vec2_magnitude(vel_2d);
		sword_transform.position.vx = -165 + (isin(state.global.time_counter * 6) * speed_1d) / (32 * ONE); if (widescreen) sword_transform.position.vx -= 52;
		sword_transform.position.vy = 135 + (icos(state.global.time_counter * 12) * speed_1d) / (64 * ONE);
		sword_transform.position.vz = 180;
		sword_transform.rotation.vx = 1800 * 16;
		sword_transform.rotation.vy = 4096 * 16;
		sword_transform.rotation.vz = -2048 * 16;
		sword_transform.scale.vx = ONE;
		sword_transform.scale.vy = ONE;
		sword_transform.scale.vz = ONE;
		renderer_draw_mesh_shaded_offset_local(&state.in_game.m_weapons->meshes[0], &sword_transform, 100);
	} 

	renderer_end_frame();

	// free temporary allocations
	mem_stack_release(STACK_TEMP);
}
void state_exit_in_game(void) {
	return;
}

void state_enter_settings(void) {
	state.global.state_to_return_to = prev_state;
	state.global.fade_level = 255;
	state.title_screen.button_pressed = 0;
	while (state.global.fade_level > 0) {
		renderer_begin_frame(&id_transform);
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
	input_update();
	// Draw background
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);
	
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_settings[0], 1, 1, white);

	// Draw settings text and box
	for (int i = 0; i < 5; ++i) {
		pixel32_t color = (pixel32_t){128, 128, 128};
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
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 0))*ONE}, (is_pal) ? ((vsync_enable == 2) ? "25 FPS" : "50 FPS") : ((vsync_enable == 2) ? "30 FPS" : "60 FPS"), 1, 0, white);
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 1))*ONE}, is_pal ? "PAL" : "NTSC", 1, 0, white);
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 2))*ONE}, widescreen ? "16:9" : "4:3", 1, 0, white);
	renderer_draw_text((vec2_t){320*ONE, (96 + (24 * 3))*ONE}, "not impl.", 1, 0, white);

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
			case 0: // frame rate limit 30 or 60
				if (vsync_enable == 1) vsync_enable = 2;
				else if (vsync_enable == 2) vsync_enable = 1;
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
				current_state = state.global.state_to_return_to;
				break;
		}
	}
	renderer_end_frame();
	return;
}
void state_exit_settings(void) {
	state.global.fade_level = 0;

	while (state.global.fade_level < 255) {
		renderer_begin_frame(&id_transform);
		input_update();
		// Draw background
		renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
		renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);
		
		renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_settings[0], 1, 1, white);

		// Draw settings text and box
		for (int i = 0; i < 5; ++i) {
			pixel32_t color = (pixel32_t){128, 128, 128};
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
		state.global.fade_level += FADE_SPEED;
		renderer_apply_fade(state.global.fade_level);
		renderer_end_frame();
	}
	state.global.fade_level = 255;
}

void state_enter_credits(void) {
	state.global.fade_level = 255;
	while (state.global.fade_level > 0) {
		renderer_begin_frame(&id_transform);
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
		renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1, white);
	}

	// Scroll text
	state.credits.scroll -= dt * 140;

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
			renderer_draw_text((vec2_t){256 * ONE, (state.credits.scroll + i * 16 * ONE) + (256 * ONE)}, text_credits[i], 1, 1, white);
		}
		input_update();
		renderer_apply_fade(state.global.fade_level);
		state.global.fade_level += FADE_SPEED;
		renderer_end_frame();
	}
	state.global.fade_level = 255;
	state.title_screen.assets_in_memory = 0;
}

void state_enter_pause_menu(void) {

}

void state_update_pause_menu(int dt) {	renderer_begin_frame(&id_transform);
	renderer_begin_frame(&id_transform);
	input_update();
	if (input_pressed(PAD_START, 0)) {
		current_state = STATE_IN_GAME;
	}
	
	// Check cheats
	if (input_check_cheat_buffer(sizeof(cheat_doom_mode) / sizeof(uint16_t), cheat_doom_mode)) {
		state.cheats.doom_mode = 1;
		music_stop();
		mem_stack_release(STACK_MUSIC);
		music_load_sequence("\\ASSETS\\MUSIC\\SEQUENCE\\E1M1.DSS");
		music_load_soundbank("\\ASSETS\\MUSIC\\INSTR.SBK");
		music_play_sequence(0);
	}

	// Paused text
	renderer_draw_text((vec2_t){256*ONE, 64*ONE}, text_pause_menu[0], 1, 1, white);

	// Draw background
	renderer_draw_2d_quad_axis_aligned((vec2_t){128*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 3, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){384*ONE, 128*ONE}, (vec2_t){256*ONE, 256*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){255*ONE, 255*ONE}, (pixel32_t){128, 128, 128}, 3, 4, 1);

	// Draw buttons
	for (int y = 0; y < 3; ++y) {
		// Left handle, right handle, middle bar, text
		pixel32_t color = (pixel32_t){128, 128, 128};
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
				current_state = STATE_IN_GAME;
				break;
			case 1: // settings
				current_state = STATE_SETTINGS;
				break;
			case 2: // exit game
				music_stop();
				current_state = STATE_TITLE_SCREEN;
				break;
		}
	}

	renderer_end_frame();
}

void state_exit_pause_menu(void) {
}