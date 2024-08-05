#ifndef MAIN_H
#define MAIN_H

#include "renderer.h"
#include "player.h"
#include "level.h"
#include "vec3.h"

int main(void);
void init(void);

typedef enum {
    STATE_NONE,
    STATE_TITLE_SCREEN,
    STATE_SETTINGS,
    STATE_CREDITS,
    STATE_IN_GAME,
    STATE_PAUSE_MENU,
    STATE_DEBUG_MENU_MAIN,
	STATE_DEBUG_MENU_LEVEL,
	STATE_DEBUG_MENU_MUSIC,
} state_t;

typedef struct {
	struct {
		int frame_counter;
		int time_counter;
		int show_debug;
		int fade_level; // 255 means black, 0 means no fade
		state_t state_to_return_to;
	} global;
	struct {
		unsigned int doom_mode : 1;
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
		level_t level;
		model_t* m_entity;
		model_t* m_weapons;
		player_t player;
		scalar_t gun_animation_timer;
		scalar_t gun_animation_timer_sqrt;
		scalar_t screen_shake_intensity_rotation;
		scalar_t screen_shake_dampening_rotation;
		scalar_t screen_shake_intensity_position;
		scalar_t screen_shake_dampening_position;
		char* level_load_path;
	} in_game;
	struct {
		int button_selected;
		int button_pressed;
	} pause_menu;
	struct {
		vec3_t shoot_origin_position;
		vec3_t shoot_hit_position;
	} debug;
	struct {
		int button_selected;
		int button_pressed;
	} debug_menu_main;
	struct {
		int button_selected;
		int button_pressed;
	} debug_menu_music;
	struct {
		int button_selected;
		int button_pressed;
	} debug_menu_level;
} state_vars_t;

extern state_vars_t state;
extern state_t current_state;
extern state_t prev_state;

#define FADE_SPEED 18
#define FADE_SPEED_SLOW 5

#define DEPTH_BIAS_VIEWMODELS 64
#define DEPTH_BIAS_LEVEL 256

// Title screen
void state_enter_title_screen(void);
void state_update_title_screen(int dt);
void state_exit_title_screen(void);

// Settings
void state_enter_settings(void);
void state_update_settings(int dt);
void state_exit_settings(void);

// Credits
void state_enter_credits(void);
void state_update_credits(int dt);
void state_exit_credits(void);

// In game
void state_enter_in_game(void);
void state_update_in_game(int dt);
void state_exit_in_game(void);

// Pause menu
void state_enter_pause_menu(void);
void state_update_pause_menu(int dt);
void state_exit_pause_menu(void);

// Debug menu main
void state_enter_debug_menu_main(void);
void state_update_debug_menu_main(int dt);
void state_exit_debug_menu_main(void);

// Debug menu music
void state_enter_debug_menu_music(void);
void state_update_debug_menu_music(int dt);
void state_exit_debug_menu_music(void);

// Debug menu level
void state_enter_debug_menu_level(void);
void state_update_debug_menu_level(int dt);
void state_exit_debug_menu_level(void);

#endif