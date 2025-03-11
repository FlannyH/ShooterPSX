#include "platform.h"

entity_platform_t* entity_platform_new(void) {
	// Allocate memory for the entity
	entity_platform_t* entity = (entity_platform_t*)entity_get_header(entity_alloc(ENTITY_PICKUP));
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = model_find_mesh(entity_get_models(), "29_platform_test_horizontal");
    entity->position_start = (vec3_t){0, 0, 0};;
    entity->position_end = (vec3_t){0, 0, 0};;
    entity->velocity = 512;
    entity->curr_timer_value = 0;
    entity->auto_start_timer = 1000;
    entity->auto_return_timer = 1000;
    entity->signal_id = 0;
	entity->listen_to_signal = 0;
	entity->target_is_end = 0;
	entity->auto_start = 0;
	entity->auto_return = 0;
	return entity;
}

void entity_platform_update(int slot, player_t* player, int dt) {
	(void)player;
	(void)dt;

	entity_platform_t* platform = (entity_platform_t*)entity_get_header(slot);
	const vec3_t platform_pos = platform->entity_header.position;

	// Move if signal is triggered
	if (platform->listen_to_signal && platform->curr_timer_value <= 0) {
		platform->target_is_end = entity_get_signal(platform->signal_id) % 2;
	}

	// Movement
	const vec3_t target_position = (platform->target_is_end) ? (platform->position_end) : (platform->position_start); // where do we want to go?
	vec3_t direction = vec3_sub(target_position, platform->entity_header.position); // what direction is that in?
	const scalar_t target_distance = scalar_sqrt(vec3_magnitude_squared(direction)); // how far away from the target are we?

	// If we're not there yet, keep moving
	if (target_distance > platform->velocity * dt) {
		direction = vec3_divs(direction, target_distance);
		platform->entity_header.position = vec3_add(platform->entity_header.position, vec3_muls(direction, platform->velocity * dt));
		platform->curr_timer_value = (platform->target_is_end) ? (platform->auto_return_timer) : (platform->auto_start_timer);
	}
	// When we get there, snap to the target position and start the return timer if that's enabled
	else {
		platform->entity_header.position = target_position;
		// Handle auto move timer
		if (platform->curr_timer_value > 0) platform->curr_timer_value -= dt;
		if (platform->curr_timer_value <= 0) {
			if (platform->target_is_end == 0 && platform->auto_start == 1) platform->target_is_end = 1;
			else if (platform->target_is_end == 1 && platform->auto_return == 1) platform->target_is_end = 0;
		}
	}

	// Make this platform solid based on the mesh bounding box
	if (platform->entity_header.mesh == NULL) {
		platform->entity_header.mesh = model_find_mesh(entity_get_models(), "29_platform_test_horizontal");
	}

	const aabb_t mesh_bounds_translated = {
		.min = {
			platform_pos.x - (platform->entity_header.mesh->bounds.max.x * COL_SCALE),
			platform_pos.y - (platform->entity_header.mesh->bounds.max.y * COL_SCALE),
			platform_pos.z - (platform->entity_header.mesh->bounds.max.z * COL_SCALE),
		},
		.max = {
			platform_pos.x - (platform->entity_header.mesh->bounds.min.x * COL_SCALE),
			platform_pos.y - (platform->entity_header.mesh->bounds.min.y * COL_SCALE),
			platform_pos.z - (platform->entity_header.mesh->bounds.min.z * COL_SCALE),
		},
	};

	const entity_collision_box_t box = {
		.aabb = mesh_bounds_translated,
		.box_index = 0,
		.entity_index = slot,
		.is_solid = 1,
		.is_trigger = 0,
	};
	entity_register_collision_box(&box);

	// Render
	transform_t render_transform;
	render_transform.position.x = -platform_pos.x / COL_SCALE;
	render_transform.position.y = -platform_pos.y / COL_SCALE;
	render_transform.position.z = -platform_pos.z / COL_SCALE;
	render_transform.rotation.x = -platform->entity_header.rotation.x;
	render_transform.rotation.y = -platform->entity_header.rotation.y;
	render_transform.rotation.z = -platform->entity_header.rotation.z;
	render_transform.scale.x = platform->entity_header.scale.x;
	render_transform.scale.y = platform->entity_header.scale.x;
	render_transform.scale.z = platform->entity_header.scale.x;
#ifdef _LEVEL_EDITOR
	renderer_set_drawing_id(slot, 1);
#endif
	renderer_draw_mesh_shaded(platform->entity_header.mesh, &render_transform, 0, 0, tex_entity_start);
}

void entity_platform_on_hit(int slot, int hitbox_index) {
	(void)slot;
	(void)hitbox_index;
}

void entity_platform_player_intersect(int slot, player_t* player) {
	(void)player;

	entity_platform_t* platform = (entity_platform_t*)entity_get_header(slot);
	if (platform->move_on_player_collision && platform->curr_timer_value <= 0) {
		platform->target_is_end = !platform->target_is_end;
	}
}
