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
}
void entity_platform_on_hit(int slot, int hitbox_index) {
}
