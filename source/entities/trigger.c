#include "trigger.h"

entity_trigger_t* entity_trigger_new(void) {
    entity_trigger_t* entity = (entity_trigger_t*)&entity_pool[entity_alloc(ENTITY_TRIGGER) * entity_pool_stride];
    entity->entity_header.position = (vec3_t){0, 0, 0};
    entity->entity_header.rotation = (vec3_t){0, 0, 0};
    entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
    entity->destroy_on_player_intersect = 1;
    entity->intersecting_curr = 0;
    entity->intersecting_prev = 0;
    entity->trigger_type = ENTITY_TRIGGER_TYPE_NONE;
}

void entity_trigger_update(int slot, player_t *player, int dt) {
    (void)dt;

    entity_trigger_t* trigger = (entity_trigger_t*)&entity_pool[slot * entity_pool_stride];
    const vec3_t trigger_pos = trigger->entity_header.position;
    const vec3_t player_pos = vec3_sub(player->position, (vec3_t){0, 200 * COL_SCALE, 0});
    const vec3_t trigger_scale_half = vec3_muls(trigger->entity_header.scale, ONE/2);

    const entity_collision_box_t box = {
        .aabb = {
            .min = vec3_sub(trigger_pos, trigger_scale_half),
            .max = vec3_add(trigger_pos, trigger_scale_half),
        },
        .box_index = 0,
        .entity_index = slot,
        .is_solid = 0,
        .is_trigger = 1,
    };
    entity_register_collision_box(&box);

    if (trigger->intersecting_curr && !trigger->intersecting_prev) {
        printf("trigger %i entered\n", slot);
    }

    trigger->intersecting_prev = trigger->intersecting_curr;
    trigger->intersecting_curr = 0;
}

void entity_trigger_on_hit(int slot, int hitbox_index) {
}

void entity_trigger_player_intersect(int slot, player_t *player) {
    entity_trigger_t* trigger = (entity_trigger_t*)&entity_pool[slot * entity_pool_stride];
    trigger->intersecting_curr = 1;
}

#ifdef _LEVEL_EDITOR 
const char* entity_trigger_type_names[] = {
    "ENTITY_TRIGGER_TYPE_NONE",
    "ENTITY_TRIGGER_TYPE_TEXT",
    NULL
};
#endif
