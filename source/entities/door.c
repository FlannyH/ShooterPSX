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
	entity->is_open = 1;
	entity->open_offset = (vec3_t){0, 64, 0};

	// Register it in the entity list
	entity_register(&entity->entity_header, ENTITY_DOOR);

	return entity;
}

void entity_door_update(int slot, player_t* player, int dt) {
	entity_door_t* this = entity_list[slot].data;

	vec3_t door_pos = *(vec3_t*)&this->entity_header.position;
	vec3_t player_pos = *(vec3_t*)&player->position;

	scalar_t distance_from_door_to_player_squared = vec3_magnitude_squared(vec3_sub(door_pos, player_pos));
	this->is_open = (distance_from_door_to_player_squared < 3 * ONE * ONE);

	// todo: implement delta time
	this->curr_interpolation_value = scalar_lerp(this->curr_interpolation_value, this->is_open ? ONE : 0, ONE / 8);

	// Render mesh - we need to shift it because my implementation of multiplication loses some precision
	door_pos = vec3_add(door_pos, vec3_shift_right(vec3_muls(this->open_offset, this->curr_interpolation_value << 3), 3));
	transform_t render_transform;

    render_transform.position.vx = -door_pos.x * (4096 / COL_SCALE);
    render_transform.position.vy = -door_pos.y * (4096 / COL_SCALE);
    render_transform.position.vz = -door_pos.z * (4096 / COL_SCALE);
    render_transform.rotation.vx = -this->entity_header.rotation.x;
    render_transform.rotation.vy = -this->entity_header.rotation.y;
    render_transform.rotation.vz = -this->entity_header.rotation.z;
	render_transform.scale.vx = this->entity_header.scale.x;
	render_transform.scale.vy = this->entity_header.scale.x;
	render_transform.scale.vz = this->entity_header.scale.x;
	renderer_draw_mesh_shaded_offset(&entity_models->meshes[0], &render_transform, 64);
}