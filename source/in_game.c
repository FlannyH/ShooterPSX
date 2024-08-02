#include "main.h"

#include "entities/pickup.h"
#include "entities/chaser.h"
#include "entities/crate.h"
#include "entities/door.h"
#include "renderer.h"
#include "random.h"
#include "memory.h"
#include "input.h"
#include "music.h"
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
#endif

void state_enter_in_game(void) {
	input_lock_mouse();
	if (prev_state == STATE_PAUSE_MENU) return;

	mem_stack_release(STACK_LEVEL);
	mem_stack_release(STACK_ENTITY);

	state.title_screen.assets_in_memory = 0;

	// todo: level_load()

    // Init player
    state.in_game.player.position.x = 11705653 / 2;
   	state.in_game.player.position.y = 11413985 / 2;
    state.in_game.player.position.z = 2112866  / 2;
	state.in_game.player.rotation.x = 0;
	state.in_game.player.rotation.y = 4096 * 16;
	state.in_game.player.rotation.z = 0;
    state.in_game.player.velocity.x = 0;
   	state.in_game.player.velocity.y = 0;
    state.in_game.player.velocity.z = 0;
	vec3_debug(state.in_game.player.position);
	vec3_debug(state.in_game.player.velocity);
	vec3_debug(state.in_game.player.rotation);

	// Load models
    state.in_game.m_level = model_load("\\assets\\models\\level.msh", 1, STACK_LEVEL);
	state.in_game.m_level_col_dbg = NULL; // model_load_collision_debug("\\assets\\models\\level.col", 1, STACK_LEVEL);
    state.in_game.m_level_col = model_load_collision("\\assets\\models\\level.col", 1, STACK_LEVEL);
	state.in_game.v_level = vislist_load("\\assets\\models\\level.vis", 1, STACK_LEVEL);
	state.in_game.m_weapons = model_load("\\assets\\models\\weapons.msh", 1, STACK_LEVEL);

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
    state.in_game.bvh_level_model = bvh_from_file("\\assets\\models\\level.col", 1, STACK_LEVEL);

	// Load textures
	texture_cpu_t *tex_level;
	texture_cpu_t *entity_textures;
	texture_cpu_t *weapon_textures;
	mem_stack_release(STACK_TEMP);
	printf("occupied STACK_TEMP: %i / %i\n", mem_stack_get_occupied(STACK_TEMP), mem_stack_get_size(STACK_TEMP));
	tex_level_start = 0;
	const uint32_t n_level_textures = texture_collection_load("\\assets\\models\\level.txc", &tex_level, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_level_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i + tex_level_start);
	}
    mem_stack_release(STACK_TEMP);
	tex_entity_start = tex_level_start + n_level_textures;
	const uint32_t n_entity_textures = texture_collection_load("\\assets\\models\\entity.txc", &entity_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
	    renderer_upload_texture(&entity_textures[i], i + tex_entity_start);
	}
    mem_stack_release(STACK_TEMP);
	tex_weapon_start = tex_entity_start + n_entity_textures;
	const uint32_t n_weapons_textures = texture_collection_load("\\assets\\models\\weapons.txc", &weapon_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_weapons_textures; ++i) {
	    renderer_upload_texture(&weapon_textures[i], i + tex_weapon_start);
	}
	mem_stack_release(STACK_TEMP);

	// Start music
	music_stop();
	music_load_soundbank("\\assets\\music\\instr.sbk");
	music_load_sequence("\\assets\\music\\sequence\\subnivis.dss");
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
	camera_transform.rotation.x += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_rotation);
	camera_transform.rotation.y += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_rotation);
	camera_transform.position.x += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	camera_transform.position.y += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	camera_transform.position.z += scalar_mul((random_u32() % 8192) - 4096, state.in_game.screen_shake_intensity_position);
	renderer_begin_frame(&camera_transform);

	// Draw crosshair
	renderer_draw_2d_quad_axis_aligned((vec2_t){256*ONE, (128 + 8*(!is_pal))*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){96*ONE, 40*ONE}, (vec2_t){127*ONE, 59*ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);

	// Draw HUD - background
	renderer_draw_2d_quad_axis_aligned((vec2_t){(121 - 16)*ONE, 236*ONE}, (vec2_t){50*ONE, 40*ONE}, (vec2_t){0*ONE, 0*ONE}, (vec2_t){50*ONE, 40*ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(260 - 16)*ONE, 236*ONE}, (vec2_t){228*ONE, 40*ONE}, (vec2_t){51*ONE, 0*ONE}, (vec2_t){51*ONE, 40*ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(413 - 16)*ONE, 236*ONE}, (vec2_t){80*ONE, 40*ONE}, (vec2_t){51*ONE, 0*ONE}, (vec2_t){129*ONE, 40*ONE}, (pixel32_t){128, 128, 128, 255}, 2, 5, 1);
	
	// Draw HUD - gauges
	renderer_draw_2d_quad_axis_aligned((vec2_t){(138 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){64*ONE, 40*ONE}, (vec2_t){96*ONE, 60*ONE}, (pixel32_t){128, 128, 128, 255}, 1, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(226 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){32*ONE, 40*ONE}, (vec2_t){64*ONE, 60*ONE}, (pixel32_t){128, 128, 128, 255}, 1, 5, 1);
	renderer_draw_2d_quad_axis_aligned((vec2_t){(314 - 16)*ONE, 236*ONE}, (vec2_t){32*ONE, 20*ONE}, (vec2_t){0*ONE, 40*ONE}, (vec2_t){32*ONE, 60*ONE}, (pixel32_t){128, 128, 128, 255}, 1, 5, 1);

	// Draw HUD - gauge text
	char gauge_text_buffer[4];
	snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.health);
	renderer_draw_text((vec2_t){(138 + 24 - 16)*ONE, 236*ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0});
	snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.armor);
	renderer_draw_text((vec2_t){(226 + 24 - 16)*ONE, 236*ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0});
	snprintf(gauge_text_buffer, 4, "%i", state.in_game.player.ammo);
	renderer_draw_text((vec2_t){(314 + 24 - 16)*ONE, 236*ONE}, gauge_text_buffer, 2, 0, (pixel32_t){255, 97, 0});

	// Draw HUD - keycards
	if (state.in_game.player.has_key_blue) renderer_draw_2d_quad_axis_aligned((vec2_t){(256 - 80 - 40)*ONE, 210*ONE}, (vec2_t){31*ONE, 20*ONE}, (vec2_t){194*ONE, 0*ONE}, (vec2_t){255*ONE, 40*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 5, 1);
	if (state.in_game.player.has_key_yellow) renderer_draw_2d_quad_axis_aligned((vec2_t){(256 + 80)*ONE, 210*ONE}, (vec2_t){31*ONE, 20*ONE}, (vec2_t){130*ONE, 0*ONE}, (vec2_t){193*ONE, 40*ONE}, (pixel32_t){128, 128, 128, 255}, 3, 5, 1);

	renderer_set_depth_bias(DEPTH_BIAS_LEVEL);

	int n_sections = player_get_level_section(&state.in_game.player, state.in_game.v_level);
	state.global.frame_counter += dt;
#if defined(_DEBUG) && defined(_PSX)
	if (input_pressed(PAD_SELECT, 0)) state.global.show_debug = !state.global.show_debug;
	if (state.global.show_debug) {
		PROFILE("input", input_update(), 1);
		PROFILE("lvl_gfx", renderer_draw_model_shaded(state.in_game.m_level, &state.in_game.t_level, state.in_game.v_level.vislists, 0), 1);
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
		renderer_draw_model_shaded(state.in_game.m_level, &state.in_game.t_level, state.in_game.v_level.vislists, 0);
		entity_update_all(&state.in_game.player, dt);
		player_update(&state.in_game.player, &state.in_game.bvh_level_model, dt, state.global.time_counter);
#if defined(_DEBUG) && defined(_PSX)
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
			scalar_t cosx = hicos(camera_transform.rotation.x);
			scalar_t sinx = hisin(camera_transform.rotation.x);
			scalar_t cosy = hicos(camera_transform.rotation.y);
			scalar_t siny = hisin(camera_transform.rotation.y);
			ray_t ray = {
				.length = INT32_MAX,
				.position = (vec3_t){camera_transform.position.x, camera_transform.position.y, camera_transform.position.z},
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
				switch (entity_types[hit.entity_hitbox.entity_index]) {
					case ENTITY_DOOR: entity_door_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
					case ENTITY_PICKUP: entity_pickup_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
					case ENTITY_CRATE: entity_crate_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
					case ENTITY_CHASER: entity_chaser_on_hit(hit.entity_hitbox.entity_index, hit.entity_hitbox.box_index); break;
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
	renderer_set_depth_bias(DEPTH_BIAS_VIEWMODELS);
	if (state.in_game.player.has_gun || 1) {
		transform_t gun_transform;
		vec2_t vel_2d = {state.in_game.player.velocity.x, state.in_game.player.velocity.z};
		scalar_t speed_1d = vec2_magnitude(vel_2d);
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
        input_rumble(state.in_game.gun_animation_timer_sqrt > 0 * 255, 0);
		renderer_draw_mesh_shaded(&state.in_game.m_weapons->meshes[1], &gun_transform, 1, tex_weapon_start);
	} 
	// Sword
	{
		transform_t sword_transform;
		vec2_t vel_2d = {state.in_game.player.velocity.x, state.in_game.player.velocity.z};
		scalar_t speed_1d = vec2_magnitude(vel_2d);
		sword_transform.position.x = -165 + (isin(state.global.time_counter * 6) * speed_1d) / (32 * ONE); if (widescreen) sword_transform.position.x -= 52;
		sword_transform.position.y = 135 + (icos(state.global.time_counter * 12) * speed_1d) / (64 * ONE);
		sword_transform.position.z = 180;
		sword_transform.rotation.x = 1800 * 16;
		sword_transform.rotation.y = 4096 * 16;
		sword_transform.rotation.z = -2048 * 16;
		sword_transform.scale.x = ONE;
		sword_transform.scale.y = ONE;
		sword_transform.scale.z = ONE;
		renderer_draw_mesh_shaded(&state.in_game.m_weapons->meshes[0], &sword_transform, 1, tex_weapon_start);
	} 

	renderer_end_frame();

	// free temporary allocations
	mem_stack_release(STACK_TEMP);
}
void state_exit_in_game(void) {
	input_unlock_mouse();
	return;
}
