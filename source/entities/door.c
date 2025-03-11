#include "door.h"

#include "../renderer.h"
#include "../music.h"
#include "../vec3.h"

entity_door_t* entity_door_new(void) {
	// Allocate memory for the entity
	entity_door_t* entity = (entity_door_t*)entity_get_header(entity_alloc(ENTITY_DOOR));
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->curr_interpolation_value = 0;
	entity->is_open = 0;
	entity->is_locked = 0;
	entity->is_big_door = 0;
	entity->is_rotated = 0;
	entity->open_by_signal = 0;
	entity->signal_id = 0;
	entity->open_offset = (vec3_t){0, 64 * ONE, 0};
	entity->entity_header.mesh = NULL;
	return entity;
}

mesh_t* update_mesh(entity_door_t* door) {	
	mesh_t* mesh;
	if (door->is_rotated) {
		if (door->is_big_door) {
			if (door->is_locked) {
				mesh = model_find_mesh(entity_get_models(), "05_door_big_locked_rotated");
			}
			else {
				mesh = model_find_mesh(entity_get_models(), "07_door_big_unlocked_rotated");
			}
		}
		else {
			if (door->is_locked) {
				mesh = model_find_mesh(entity_get_models(), "01_door_small_locked_rotated");
			}
			else {
				mesh = model_find_mesh(entity_get_models(), "03_door_small_unlocked_rotated");
			}
		}
	}
	else {
		if (door->is_big_door) {
			if (door->is_locked) {
				mesh = model_find_mesh(entity_get_models(), "04_door_big_locked");
			}
			else {
				mesh = model_find_mesh(entity_get_models(), "06_door_big_unlocked");
			}
		}
		else {
			if (door->is_locked) {
				mesh = model_find_mesh(entity_get_models(), "00_door_small_locked");
			}
			else {
				mesh = model_find_mesh(entity_get_models(), "02_door_small_unlocked");
			}
		}
	}
	
	PANIC_IF("Door entity could not find mesh!", mesh == NULL);
	return mesh;
}

void entity_door_update(int slot, player_t* player, int dt) {
	(void)dt;
	entity_door_t* door = (entity_door_t*)entity_get_header(slot);

	vec3_t door_pos = door->entity_header.position;
	const vec3_t player_pos = player->position;

	const scalar_t distance_from_door_to_player_squared = vec3_magnitude_squared(vec3_sub(door_pos, player_pos));
	const int player_close_enough = (distance_from_door_to_player_squared < 4500 * ONE) && (distance_from_door_to_player_squared > 0);

	// Handle unlocking with key cards
	if (door->is_locked && player_close_enough) {
		if (!door->is_big_door && player->has_key_blue) {
			door->is_locked = 0;
			player->has_key_blue = 0;
			door->state_changed = 1;
			audio_play_sound(sfx_door_unlock, 0, 1, door_pos, 3600 * ONE); 
		}
		else if (door->is_big_door && player->has_key_yellow) {
			door->is_locked = 0;
			player->has_key_yellow = 0;
			door->state_changed = 1;
			audio_play_sound(sfx_door_unlock, 0, 1, door_pos, 3600 * ONE); 
		}
	}

	// Handle unlocking by signal
	if (door->is_locked && door->open_by_signal && entity_get_signal(door->signal_id) > 0) {
		door->is_locked = 0;
		door->state_changed = 1;
		audio_play_sound(sfx_door_unlock, 0, 1, door_pos, 3600 * ONE); 
	}

	const int prev = door->is_open;
	door->is_open = player_close_enough && !door->is_locked;
	if (door->is_open && !prev) audio_play_sound(sfx_door_open, 0, 1, door_pos, 3600 * ONE); 
	else if (!door->is_open && prev) audio_play_sound(sfx_door_close, 0, 1, door_pos, 3600 * ONE); 

	// todo: implement delta time
	door->curr_interpolation_value = scalar_lerp(door->curr_interpolation_value, door->is_open ? ONE : 0, ONE / 8);

	// Find mesh to use - a bit verbose for the sake of avoiding magic numbers, preventing me from shooting myself in the foot later
	if (door->state_changed) door->entity_header.mesh = update_mesh(door);
	if (!door->entity_header.mesh) door->entity_header.mesh = update_mesh(door);

	// Add collision for the door
	if (door->is_locked) {
		aabb_t bounds = door->entity_header.mesh->bounds;
		bounds.min.x *= COL_SCALE; bounds.min.y *= COL_SCALE; bounds.min.z *= COL_SCALE; 
		bounds.max.x *= COL_SCALE; bounds.max.y *= COL_SCALE; bounds.max.z *= COL_SCALE; 
		const aabb_t collision_box = {
			.min = vec3_sub(door_pos, bounds.max),
			.max = vec3_sub(door_pos, bounds.min)
		};
		const entity_collision_box_t box = {
			.aabb = collision_box,
			.entity_index = slot,
			.box_index = 0,
			.is_solid = 1,
			.is_trigger = 0,
		};
		entity_register_collision_box(&box);
	}

	door->state_changed = 0;

	// Render mesh
	door_pos = vec3_add(door_pos, vec3_muls(door->open_offset, door->curr_interpolation_value));
	transform_t render_transform;

    render_transform.position.x = -door_pos.x / COL_SCALE;
    render_transform.position.y = -door_pos.y / COL_SCALE;
    render_transform.position.z = -door_pos.z / COL_SCALE;
    render_transform.rotation.x = -door->entity_header.rotation.x;
    render_transform.rotation.y = -door->entity_header.rotation.y;
    render_transform.rotation.z = -door->entity_header.rotation.z;
	render_transform.scale.x = door->entity_header.scale.x;
	render_transform.scale.y = door->entity_header.scale.x;
	render_transform.scale.z = door->entity_header.scale.x;
#ifdef _LEVEL_EDITOR
	renderer_set_drawing_id(slot, 1);
#endif
	renderer_draw_mesh_shaded(door->entity_header.mesh, &render_transform, 0, 0, tex_entity_start);
}

void entity_door_on_hit(int slot, int hitbox_index) {
	(void)slot;
	(void)hitbox_index;
}

void entity_door_player_intersect(int slot, player_t* player) {
	(void)slot;
	(void)player;
}
