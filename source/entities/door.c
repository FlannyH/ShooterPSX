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
	entity->open_offset = (vec3_t){0, 64 * ONE, 0};

	// Register it in the entity list
	entity_register(&entity->entity_header, ENTITY_DOOR_SMALL);

	return entity;
}

void entity_door_update(int slot, player_t* player, int dt) {
	entity_door_t* this = (entity_door_t*)entity_list[slot].data;

	vec3_t door_pos = this->entity_header.position;
	vec3_t player_pos = player->position;

	scalar_t distance_from_door_to_player_squared = vec3_magnitude_squared(vec3_sub(door_pos, player_pos));
	int player_close_enough = (distance_from_door_to_player_squared < 4500 * ONE) && (distance_from_door_to_player_squared > 0);

	// Handle unlocking with key cards
	if (this->is_locked && player_close_enough) {
		if (!this->is_big_door && player->has_key_blue) {
			this->is_locked = 0;
			player->has_key_blue = 0;
		}
		else if (this->is_big_door && player->has_key_yellow) {
			this->is_locked = 0;
			player->has_key_yellow = 0;
		}
	}

	this->is_open = player_close_enough && !this->is_locked;

	// todo: implement delta time
	this->curr_interpolation_value = scalar_lerp(this->curr_interpolation_value, this->is_open ? ONE : 0, ONE / 8);

	// Add collision for the door
	if (this->is_locked) {
		const aabb_t collision_box = {
			.min = vec3_add(door_pos, (vec3_t){-4, 0, -20}),
			.max = vec3_add(door_pos, (vec3_t){4, 48, 20})
		};
		entity_register_collision_box(&collision_box);
	}

	// Render mesh - we need to shift it because my implementation of multiplication loses some precision
	door_pos = vec3_add(door_pos, vec3_shift_right(vec3_muls(this->open_offset, this->curr_interpolation_value << 3), 3));
	transform_t render_transform;

    render_transform.position.vx = -door_pos.x / COL_SCALE;
    render_transform.position.vy = -door_pos.y / COL_SCALE;
    render_transform.position.vz = -door_pos.z / COL_SCALE;
    render_transform.rotation.vx = -this->entity_header.rotation.x;
    render_transform.rotation.vy = -this->entity_header.rotation.y;
    render_transform.rotation.vz = -this->entity_header.rotation.z;
	render_transform.scale.vx = this->entity_header.scale.x;
	render_transform.scale.vy = this->entity_header.scale.x;
	render_transform.scale.vz = this->entity_header.scale.x;
	size_t mesh_id;
	if (this->is_big_door) {
		if (this->is_locked) {
			mesh_id = ENTITY_MESH_DOOR_BIG_LOCKED;
		}
		else {
			mesh_id = ENTITY_MESH_DOOR_BIG_UNLOCKED;
		}
	}
	else {
		if (this->is_locked) {
			mesh_id = ENTITY_MESH_DOOR_SMALL_LOCKED;
		}
		else {
			mesh_id = ENTITY_MESH_DOOR_SMALL_UNLOCKED;
		}
	}
	renderer_draw_mesh_shaded_offset(&entity_models->meshes[mesh_id], &render_transform, 64);
}