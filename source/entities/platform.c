#include "platform.h"

entity_platform_t* entity_platform_new(void) {
	// Allocate memory for the entity
	entity_platform_t* entity = (entity_platform_t*)&entity_pool[entity_alloc(ENTITY_PLATFORM) * entity_pool_stride];
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = model_find_mesh(entity_models, "29_platform_test_horizontal");

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
	(void)dt;

	entity_platform_t* platform = (entity_platform_t*)&entity_pool[slot * entity_pool_stride];
	vec3_t platform_pos = platform->entity_header.position;
	vec3_t player_pos = vec3_sub(player->position, (vec3_t){0, 200 * COL_SCALE, 0});

	// Make this platform solid based on the mesh bounding box
	if (platform->entity_header.mesh == NULL) {
		platform->entity_header.mesh = model_find_mesh(entity_models, "29_platform_test_horizontal");
	}
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
	drawing_entity_id = slot;
#endif
	renderer_draw_mesh_shaded(platform->entity_header.mesh, &render_transform, 0, 0, tex_entity_start);
}

void entity_platform_on_hit(int slot, int hitbox_index) {
	(void)slot;
	(void)hitbox_index;
}
