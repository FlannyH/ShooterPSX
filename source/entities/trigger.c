#include "trigger.h"

#include "../main.h"

extern state_vars_t state;

entity_trigger_t* entity_trigger_new(void) {
    entity_trigger_t* entity = (entity_trigger_t*)&entity_pool[entity_alloc(ENTITY_TRIGGER) * entity_pool_stride];
    entity->entity_header.position = (vec3_t){0, 0, 0};
    entity->entity_header.rotation = (vec3_t){0, 0, 0};
    entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
    entity->destroy_on_player_intersect = 1;
    entity->intersecting_curr = 0;
    entity->intersecting_prev = 0;
    entity->is_busy = 0;
    entity->trigger_type = ENTITY_TRIGGER_TYPE_NONE;
    return entity;
}

void entity_trigger_update(int slot, player_t *player, int dt) {
    entity_trigger_t* trigger = (entity_trigger_t*)&entity_pool[slot * entity_pool_stride];

    if (!trigger->is_busy) {
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
            trigger->is_busy = 1;
            
            if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TEXT) {
                trigger->data_text.curr_display_time_ms = trigger->data_text.total_display_time_ms;
            }
        }
        
        trigger->intersecting_prev = trigger->intersecting_curr;
        trigger->intersecting_curr = 0;
        
        return;
    }

    if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TEXT) {
        trigger->data_text.curr_display_time_ms -= dt;
        if (trigger->data_text.curr_display_time_ms <= 0) {
            entity_kill(slot);
        }

        const char* text = state.in_game.level.text_entries[trigger->data_text.id];
        renderer_draw_text((vec2_t){256 * ONE, 176 * ONE}, text, 2, 1, trigger->data_text.color);

        return;
    }

    // Return to not busy by default
    trigger->is_busy = 0;
}

void entity_trigger_on_hit(int slot, int hitbox_index) {
    (void)slot;
    (void)hitbox_index;
}

void entity_trigger_player_intersect(int slot, player_t* player) {
    (void)player;

    entity_trigger_t* trigger = (entity_trigger_t*)&entity_pool[slot * entity_pool_stride];
    trigger->intersecting_curr = 1;
}

#ifdef _LEVEL_EDITOR 
const char* entity_trigger_type_names[] = {
    "ENTITY_TRIGGER_TYPE_NONE",
    "ENTITY_TRIGGER_TYPE_TEXT",
    "ENTITY_TRIGGER_TYPE_SIGNAL",
    NULL
};
#endif
