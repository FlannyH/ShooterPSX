#include "main.h"

#include "entities/platform.h"
#include "entities/trigger.h"
#include "entities/pickup.h"
#include "entities/chaser.h"
#include "entities/crate.h"
#include "entities/door.h"
#include "entity.h"
#include "renderer.h"
#include "random.h"
#include "memory.h"
#include "input.h"
#include "level.h"
#include "music.h"
#include "text.h"

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
#include <nds.h>
#include <filesystem.h>
#endif

void update_screen_shake_intensity(int dt);
void load_weapon_textures(void);
void benchmark_mode(void);
void fps_counter(int dt);
void draw_hud(void);
void draw_debug_info(int dt, const int n_sections);
void shoot(const transform_t camera_transform);
char debug_text_buffer[64] = {0};
int fps = 0;

void state_enter_in_game(void) {
#ifdef BENCHMARK_MODE
	state.global.time_counter = 0;
#endif

	input_lock_mouse();
	if (get_prev_state() == STATE_PAUSE_MENU) return;

	mem_stack_release(STACK_LEVEL);
	mem_stack_release(STACK_ENTITY);

	state.title_screen.assets_in_memory = 0;
	tex_alloc_cursor = 0;

	entity_init();
	state.in_game.level = level_load(state.in_game.level_load_path, LEVEL_LOAD_ALL);
	player_init(&state.in_game.player,
		vec3_from_svec3(state.in_game.level.player_spawn_position),
		state.in_game.level.player_spawn_rotation,
		40, 0, 0
	);

	// Load weapon textures
    load_weapon_textures();
    mem_stack_release(STACK_TEMP);
	
	// Load weapon models
	state.in_game.m_weapons = model_load("models/weapons.msh", 1, STACK_LEVEL, tex_weapon_start, 1);

	#if defined(_DEBUG) && defined(_PSX)
		FntLoad(320,256);
		FntOpen(32, 32, 256, 192, 0, 512);
	#endif

	renderer_start_fade_in(FADE_SPEED);
}

void load_weapon_textures(void) {
    tex_weapon_start = tex_alloc_cursor;
    texture_cpu_t *weapon_textures;
    const uint32_t n_weapon_textures = texture_collection_load("models/weapons.txc", &weapon_textures, 1, STACK_TEMP);
    for (uint8_t i = 0; i < n_weapon_textures; ++i) {
        renderer_upload_texture(&weapon_textures[i], i + tex_weapon_start);
    }
    tex_alloc_cursor += n_weapon_textures;
}

void state_update_in_game(int dt) {
    update_screen_shake_intensity(dt);

    // Apply screen shake to camera transform, then begin graphics frame
	transform_t camera_transform = state.in_game.player.transform;
	camera_transform.rotation.x += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_rotation);
	camera_transform.rotation.y += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_rotation);
	camera_transform.position.x += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	camera_transform.position.y += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	camera_transform.position.z += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	renderer_begin_frame(&camera_transform);

#ifdef BENCHMARK_MODE
    benchmark_mode();
#endif
#ifdef FPS_COUNTER
    fps_counter(dt);
#endif

    draw_hud();

    // Set depth bias to render level geometry in front of UI and weapon models
	renderer_set_depth_bias(DEPTH_BIAS_LEVEL);
	
	// Figure out where the player is so we can use the right vislist
	const int n_sections = renderer_get_camera_level_section(state.in_game.player.position, state.in_game.level.vislist);
	
	state.global.frame_counter += 1;

#if defined(_DEBUG) && defined(_PSX)
	if (input_pressed(PAD_SELECT, 0)) state.global.show_debug = !state.global.show_debug;

	if (state.global.show_debug) {
        draw_debug_info(dt, n_sections);
    }
	else {
#endif
		(void)n_sections;
		input_update();
#if defined(_PSX) && defined(FPS_COUNTER)
		const uint32_t timer_value_before = TIMER_VALUE(1) & 0xFFFF; // Get start time
		renderer_draw_model_shaded(state.in_game.level.graphics, &state.in_game.level.transform, state.in_game.level.vislist.vislists, 0);
		uint32_t timer_value_after = TIMER_VALUE(1) & 0xFFFF; // Get end time

		// Correct for int16_t overflow
		if (timer_value_after < timer_value_before) timer_value_after += 0x10000;

		// Show how long it took to render the level
		snprintf(debug_text_buffer, 64, "LEVEL: %i HBLANKS", timer_value_after - timer_value_before);	
		renderer_set_depth_bias(0);
		renderer_draw_text((vec2_t){32 * ONE, 64 * ONE}, debug_text_buffer, 0, 0, (fps >= 30) ? green : red);	
#else
		renderer_draw_model_shaded(state.in_game.level.graphics, &state.in_game.level.transform, state.in_game.level.vislist.vislists, 0);
#endif

		entity_update_all(&state.in_game.player, dt);
#ifdef BENCHMARK_MODE
		// In benchmark mode the world should be paused, so dt = 0
		player_update(&state.in_game.player, &state.in_game.level.collision_bvh, 0, state.global.time_counter);
#else
		player_update(&state.in_game.player, &state.in_game.level.collision_bvh, dt, state.global.time_counter);
#endif
#if defined(_DEBUG) && defined(_PSX)
	}
#endif

	if (input_pressed(PAD_START, 0)) {
		set_current_state(STATE_PAUSE_MENU);
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
			shoot(camera_transform);
		}
		if (state.global.show_debug) {
			const size_t n_active_aabb = entity_get_n_active_aabb();
			for (size_t i = 0; i < n_active_aabb; ++i) {
				renderer_debug_draw_aabb(&entity_get_aabb_queue_entry(i)->aabb, pink, &id_transform);
			}
		}
	}

	// We render the player's weapons last, because we need the view matrices to be reset
	PANIC_IF("trying to render non-existent weapon meshes!", (state.in_game.m_weapons == NULL) || (state.in_game.m_weapons->meshes == NULL ) || (state.in_game.m_weapons->n_meshes < 2));
	renderer_set_depth_bias(DEPTH_BIAS_VIEWMODELS);

	// Animate and render gun
	const vec2_t vel_2d = {state.in_game.player.velocity.x, state.in_game.player.velocity.z};
	const scalar_t speed_1d = vec2_magnitude(vel_2d) / PLAYER_VELOCITY_PRECISION;
	if (state.in_game.player.has_gun || 1) {

		transform_t gun_transform;
		if (state.cheats.doom_mode) {
			gun_transform.position.x = 0 + (isin(state.global.time_counter * 6) * speed_1d) / (40 * ONE); if (widescreen) gun_transform.position.x += 30;
			gun_transform.position.y = 165 + (icos(state.global.time_counter * 12) * speed_1d) / (80 * ONE);
			gun_transform.position.z = 110;
		} else {
			gun_transform.position.x = 145 + (isin(state.global.time_counter * 6) * speed_1d) / (40 * ONE); if (widescreen) gun_transform.position.x += 30;
			gun_transform.position.y = 135 + (icos(state.global.time_counter * 12) * speed_1d) / (80 * ONE) + scalar_mul(state.in_game.gun_animation_timer_sqrt, 50);
			gun_transform.position.z = 125 - scalar_mul(state.in_game.gun_animation_timer_sqrt, 225);
		}
		gun_transform.rotation.x = 0 + scalar_mul(state.in_game.gun_animation_timer_sqrt, 9500);
		gun_transform.rotation.y = 4096 * 16;
		gun_transform.rotation.z = 0;
		gun_transform.scale.x = ONE;
		gun_transform.scale.y = ONE;
		gun_transform.scale.z = ONE;

		renderer_draw_mesh_shaded(&state.in_game.m_weapons->meshes[1], &gun_transform, 1, 0, tex_weapon_start);
        input_rumble(state.in_game.gun_animation_timer_sqrt > 0 * 255, 0);
	}

	// Animate and render sword
	{
		transform_t sword_transform;
		sword_transform.position.x = -165 + (isin(state.global.time_counter * 6) * speed_1d) / (32 * ONE); if (widescreen) sword_transform.position.x -= 52;
		sword_transform.position.y = 135 + (icos(state.global.time_counter * 12) * speed_1d) / (64 * ONE);
		sword_transform.position.z = 180;
		sword_transform.rotation.x = 1800 * 16;
		sword_transform.rotation.y = 4096 * 16;
		sword_transform.rotation.z = -2048 * 16;
		sword_transform.scale.x = ONE;
		sword_transform.scale.y = ONE;
		sword_transform.scale.z = ONE;
		renderer_draw_mesh_shaded(&state.in_game.m_weapons->meshes[0], &sword_transform, 1, 0, tex_weapon_start);
	} 

	renderer_end_frame();

	// free temporary allocations
	mem_stack_release(STACK_TEMP);
}

void draw_debug_info(int dt, const int n_sections) {
#if defined(_DEBUG) && defined(_PSX)
    // Run the game logic within PROFILE calls, which prints the time (in hblanks) a function took to complete
    PROFILE("input", input_update(), 1);
    PROFILE("lvl_gfx", renderer_draw_model_shaded(state.in_game.level.graphics, &state.in_game.level.transform, state.in_game.level.vislist.vislists, 0), 1);
    PROFILE("entity", entity_update_all(&state.in_game.player, dt), 1);
    PROFILE("player", player_update(&state.in_game.player, &state.in_game.level.collision_bvh, dt, state.global.time_counter), 1);

    // Print some useful debug info to the screen
    FntPrint(-1, "\n");
    FntPrint(-1, "dt: %i\n", dt);
    FntPrint(-1, "meshes drawn: %i\n", renderer_n_meshes_drawn());
    FntPrint(-1, "frame: %i\n", state.global.frame_counter);
    FntPrint(-1, "time: %i.%03i\n", state.global.time_counter / 1000, state.global.time_counter % 1000);
    FntPrint(-1, "player pos: %i, %i, %i\n",
             state.in_game.player.position.x / 4096,
             state.in_game.player.position.y / 4096,
             state.in_game.player.position.z / 4096);
    FntPrint(-1, "player velocity: %i, %i, %i\n",
             state.in_game.player.velocity.x,
             state.in_game.player.velocity.y,
             state.in_game.player.velocity.z);
    FntPrint(-1, "origin: %i, %i, %i\n",
             state.debug.shoot_origin_position.x / 4096,
             state.debug.shoot_origin_position.y / 4096,
             state.debug.shoot_origin_position.z / 4096);
    FntPrint(-1, "hit: %i, %i, %i\n",
             state.debug.shoot_hit_position.x / 4096,
             state.debug.shoot_hit_position.y / 4096,
             state.debug.shoot_hit_position.z / 4096);
    FntPrint(-1, "gun anim timer: %i\n", state.in_game.gun_animation_timer);
    for (int i = 0; i < N_STACK_TYPES; ++i) {
        FntPrint(-1, "%s: %i / %i KiB (%i%%)\n",
                 mem_stack_get_name(i),
                 mem_stack_get_occupied(i) / KiB,
                 mem_stack_get_size(i) / KiB,
                 (mem_stack_get_occupied(i) * 100) / mem_stack_get_size(i));
    }
    collision_clear_stats();
    FntFlush(-1);
#endif
	(void)dt;
	(void)n_sections;
}

void draw_hud(void) {
    // Draw crosshair
    renderer_draw_2d_quad_axis_aligned((vec2_t){256 * ONE, (128 + 8 * (!is_pal)) * ONE}, (vec2_t){32 * ONE, 20 * ONE}, (vec2_t){96 * ONE, 40 * ONE}, (vec2_t){127 * ONE, 59 * ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);

    // Draw HUD - background
    renderer_draw_2d_quad_axis_aligned((vec2_t){(121 - 16) * ONE, 236 * ONE}, (vec2_t){50 * ONE, 40 * ONE}, (vec2_t){0 * ONE, 0 * ONE}, (vec2_t){50 * ONE, 40 * ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);
    renderer_draw_2d_quad_axis_aligned((vec2_t){(260 - 16) * ONE, 236 * ONE}, (vec2_t){228 * ONE, 40 * ONE}, (vec2_t){51 * ONE, 0 * ONE}, (vec2_t){51 * ONE, 40 * ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);
    renderer_draw_2d_quad_axis_aligned((vec2_t){(413 - 16) * ONE, 236 * ONE}, (vec2_t){80 * ONE, 40 * ONE}, (vec2_t){51 * ONE, 0 * ONE}, (vec2_t){129 * ONE, 40 * ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);

    // Draw HUD - gauges
    renderer_draw_2d_quad_axis_aligned((vec2_t){(138 - 16) * ONE, 236 * ONE}, (vec2_t){32 * ONE, 20 * ONE}, (vec2_t){64 * ONE, 40 * ONE}, (vec2_t){96 * ONE, 60 * ONE}, (pixel32_t){128, 128, 128, 255}, 1, 5, 1);
    renderer_draw_2d_quad_axis_aligned((vec2_t){(226 - 16) * ONE, 236 * ONE}, (vec2_t){32 * ONE, 20 * ONE}, (vec2_t){32 * ONE, 40 * ONE}, (vec2_t){64 * ONE, 60 * ONE}, (pixel32_t){128, 128, 128, 255}, 1, 5, 1);
    renderer_draw_2d_quad_axis_aligned((vec2_t){(314 - 16) * ONE, 236 * ONE}, (vec2_t){32 * ONE, 20 * ONE}, (vec2_t){0 * ONE, 40 * ONE}, (vec2_t){32 * ONE, 60 * ONE}, (pixel32_t){128, 128, 128, 255}, 1, 5, 1);

    // Draw HUD - gauge text
    char gauge_text_buffer[4];
    snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.health);
    renderer_draw_text((vec2_t){(138 + 24 - 16) * ONE, 236 * ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0, 255});
    snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.armor);
    renderer_draw_text((vec2_t){(226 + 24 - 16) * ONE, 236 * ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0, 255});
    snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.ammo);
    renderer_draw_text((vec2_t){(314 + 24 - 16) * ONE, 236 * ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0, 255});

    // Draw HUD - keycards
    if (state.in_game.player.has_key_blue)
        renderer_draw_2d_quad_axis_aligned((vec2_t){(256 - 80 - 40) * ONE, 210 * ONE}, (vec2_t){31 * ONE, 20 * ONE}, (vec2_t){194 * ONE, 0 * ONE}, (vec2_t){255 * ONE, 40 * ONE}, (pixel32_t){128, 128, 128, 255}, 3, 5, 1);
    if (state.in_game.player.has_key_yellow)
        renderer_draw_2d_quad_axis_aligned((vec2_t){(256 + 80) * ONE, 210 * ONE}, (vec2_t){31 * ONE, 20 * ONE}, (vec2_t){130 * ONE, 0 * ONE}, (vec2_t){193 * ONE, 40 * ONE}, (pixel32_t){128, 128, 128, 255}, 3, 5, 1);
}

void fps_counter(int dt) {
    static int timer = 0;
    static int prev_frame_counter = 0;

    // Format and render fps counter
    timer += dt;
    if (timer > 1000) {
        timer -= 1000;
        fps = state.global.frame_counter - prev_frame_counter;
        prev_frame_counter = state.global.frame_counter;
    }
    if (state.global.frame_counter) snprintf(debug_text_buffer, 64, "%i fps\n%i ms", fps, dt);
    renderer_draw_text((vec2_t){32 * ONE, 32 * ONE}, debug_text_buffer, 0, 0, (fps >= 30) ? green : red);
}

void benchmark_mode(void) {
    // Hack together some benchmark positions and fixed graphics settings
    widescreen = 1;
    vsync_enable = 0;
    const transform_t benchmark_positions[] = {
        (transform_t){.position = (vec3_t){5849088, 5363200, 1052672}, .rotation = (vec3_t){0, 65536, 0}},     // starting area
        (transform_t){.position = (vec3_t){2695122, 4257280, 7820815}, .rotation = (vec3_t){1272, 43970, 0}},  // near pillar bottom
        (transform_t){.position = (vec3_t){2798083, 4777824, 7800474}, .rotation = (vec3_t){-1134, 40496, 0}}, // near pillar top
        (transform_t){.position = (vec3_t){161404, 4247552, 6456851}, .rotation = (vec3_t){4492, 76632, 0}},   // final section lower area
    };

    // Teleport to each position for 5 sec
    const int teleport_index = state.global.time_counter / (1000 * 5);
    if (teleport_index < 4) {
        state.in_game.player.position = benchmark_positions[teleport_index].position;
        state.in_game.player.rotation = benchmark_positions[teleport_index].rotation;
    }
}

void update_screen_shake_intensity(int dt) {
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
}

void shoot(const transform_t camera_transform) {
	// Construct gun ray
	const scalar_t cosx = hicos(camera_transform.rotation.x);
	const scalar_t sinx = hisin(camera_transform.rotation.x);
	const scalar_t cosy = hicos(camera_transform.rotation.y);
	const scalar_t siny = hisin(camera_transform.rotation.y);
	ray_t ray = {
		.length = INT32_MAX,
		.position = (vec3_t){camera_transform.position.x, camera_transform.position.y, camera_transform.position.z},
		.direction = {scalar_mul(siny, cosx), -sinx, scalar_mul(-cosy, cosx)},
	};
	ray.position = vec3_muls(ray.position, -COL_SCALE);
	ray.inv_direction = vec3_div((vec3_t){ONE, ONE, ONE}, ray.direction);

	// Intersect level
	rayhit_t hit;
	bvh_intersect_ray(&state.in_game.level.collision_bvh, ray, &hit);

	// Intersect entities
	const size_t n_active_aabb = entity_get_n_active_aabb();
	for (size_t i = 0; i < n_active_aabb; ++i) {
		rayhit_t entity_hit;
		const entity_collision_box_t* const box = entity_get_aabb_queue_entry(i);
		if (ray_aabb_intersect_fancy(&box->aabb, ray, &entity_hit)) {
			if (entity_hit.distance < hit.distance) {
				hit = entity_hit;
				hit.type = RAY_HIT_TYPE_ENTITY_HITBOX;
				hit.entity_hitbox.box_index = box->box_index;
				hit.entity_hitbox.entity_index = box->entity_index;
			}
		}
	}
	
#ifdef _DEBUG
	// Update debug display
	if (!is_infinity(hit.distance)) {
		state.debug.shoot_origin_position = ray.position;
		state.debug.shoot_hit_position = hit.position;
	}
#endif
	// If we hit an entity, run its on_hit function
	if (!is_infinity(hit.distance) && hit.type == RAY_HIT_TYPE_ENTITY_HITBOX) {
		switch (entity_get_type(hit.entity_hitbox.entity_index)) {
			case ENTITY_DOOR: entity_door_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
			case ENTITY_PICKUP: entity_pickup_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
			case ENTITY_CRATE: entity_crate_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
			case ENTITY_CHASER: entity_chaser_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
			case ENTITY_PLATFORM: entity_platform_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
			case ENTITY_TRIGGER: entity_trigger_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
		}
	}

	// Start shoot animation
	state.in_game.gun_animation_timer = 4096;
	state.in_game.screen_shake_intensity_position = 80000;
	state.in_game.screen_shake_dampening_position = 200;
	state.in_game.screen_shake_intensity_rotation = 240;
	state.in_game.screen_shake_dampening_rotation = 2;

	// Play shoot sound
	audio_play_sound(random_range(sfx_rev_shot_1, sfx_rev_shot_4 + 1), 0, 0, (vec3_t){}, ONE);

	// Reduce ammo count
	state.in_game.player.ammo--;
}

void state_exit_in_game(void) {
	input_unlock_mouse();
}
