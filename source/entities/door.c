#include "door.h"
#include <string.h>
#include "../memory.h"
#include "../vec3.h"
#include "../renderer.h"

entity_door_t* entity_door_new() {
	// Allocate memory for the entity
	entity_door_t* entity = mem_stack_alloc(sizeof(entity_door_t), STACK_ENTITY);
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->curr_interpolation_value = 0;
	entity->is_open = 0;
	entity->is_locked = 0;
	entity->is_big_door = 0;
	entity->is_rotated = 0;
	entity->open_offset = (vec3_t){0, 64 * ONE, 0};

	// Register it in the entity list
	entity_register(&entity->entity_header, ENTITY_DOOR);

	return entity;
}

void entity_door_update(int slot, player_t* player, int dt) {
	entity_door_t* door = (entity_door_t*)entity_list[slot].data;

	vec3_t door_pos = door->entity_header.position;
	vec3_t player_pos = player->position;

	scalar_t distance_from_door_to_player_squared = vec3_magnitude_squared(vec3_sub(door_pos, player_pos));
	int player_close_enough = (distance_from_door_to_player_squared < 4500 * ONE) && (distance_from_door_to_player_squared > 0);

	// Handle unlocking with key cards
	if (door->is_locked && player_close_enough) {
		if (!door->is_big_door && player->has_key_blue) {
			door->is_locked = 0;
			player->has_key_blue = 0;
		}
		else if (door->is_big_door && player->has_key_yellow) {
			door->is_locked = 0;
			player->has_key_yellow = 0;
		}
	}

	door->is_open = player_close_enough && !door->is_locked;

	// todo: implement delta time
	door->curr_interpolation_value = scalar_lerp(door->curr_interpolation_value, door->is_open ? ONE : 0, ONE / 8);

	// Find mesh to use - a bit verbose for the sake of avoiding magic numbers, preventing me from shooting myself in the foot later
	size_t mesh_id;
	if (door->is_rotated) {
		if (door->is_big_door) {
			if (door->is_locked) {
				mesh_id = ENTITY_MESH_DOOR_BIG_LOCKED_ROTATED;
			}
			else {
				mesh_id = ENTITY_MESH_DOOR_BIG_UNLOCKED_ROTATED;
			}
		}
		else {
			if (door->is_locked) {
				mesh_id = ENTITY_MESH_DOOR_SMALL_LOCKED_ROTATED;
			}
			else {
				mesh_id = ENTITY_MESH_DOOR_SMALL_UNLOCKED_ROTATED;
			}
		}
	}
	else {
		if (door->is_big_door) {
			if (door->is_locked) {
				mesh_id = ENTITY_MESH_DOOR_BIG_LOCKED;
			}
			else {
				mesh_id = ENTITY_MESH_DOOR_BIG_UNLOCKED;
			}
		}
		else {
			if (door->is_locked) {
				mesh_id = ENTITY_MESH_DOOR_SMALL_LOCKED;
			}
			else {
				mesh_id = ENTITY_MESH_DOOR_SMALL_UNLOCKED;
			}
		}
	}

	// Add collision for the door
	if (door->is_locked) {
		aabb_t bounds = entity_models->meshes[mesh_id].bounds;
		bounds.min.x *= COL_SCALE; bounds.min.y *= COL_SCALE; bounds.min.z *= COL_SCALE; 
		bounds.max.x *= COL_SCALE; bounds.max.y *= COL_SCALE; bounds.max.z *= COL_SCALE; 
		const aabb_t collision_box = {
			.min = vec3_add(door_pos, bounds.min),
			.max = vec3_add(door_pos, bounds.max)
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

	// Render mesh - we need to shift it because my implementation of multiplication loses some precision
	door_pos = vec3_add(door_pos, vec3_shift_right(vec3_muls(door->open_offset, door->curr_interpolation_value << 3), 3));
	transform_t render_transform;

    render_transform.position.vx = -door_pos.x / COL_SCALE;
    render_transform.position.vy = -door_pos.y / COL_SCALE;
    render_transform.position.vz = -door_pos.z / COL_SCALE;
    render_transform.rotation.vx = -door->entity_header.rotation.x;
    render_transform.rotation.vy = -door->entity_header.rotation.y;
    render_transform.rotation.vz = -door->entity_header.rotation.z;
	render_transform.scale.vx = door->entity_header.scale.x;
	render_transform.scale.vy = door->entity_header.scale.x;
	render_transform.scale.vz = door->entity_header.scale.x;
	renderer_draw_mesh_shaded_offset(&entity_models->meshes[mesh_id], &render_transform, 64);
}

void entity_door_on_hit(int slot, int hitbox_index) {}