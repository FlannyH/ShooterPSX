#include "trigger.h"

#include "engine/renderer.h"
#include "subnivis/main.h"

extern state_vars_t state;

size_t entity_trigger_serialize(int slot, uint8_t* dest) {
    entity_trigger_t* trigger_source = (entity_trigger_t*)entity_get_header(slot);

    entity_trigger_t* trigger_dest = (entity_trigger_t*)dest;
    *trigger_dest = *trigger_source;

    if (trigger_source->trigger_type == ENTITY_TRIGGER_TYPE_TELEPORT) {
        serialize_vec3(&trigger_dest->teleport.destination_pos, trigger_source->teleport.destination_pos);
        serialize_vec3(&trigger_dest->teleport.destination_rotation_offset, trigger_source->teleport.destination_rotation_offset);
    }
    else if (trigger_source->trigger_type == ENTITY_TRIGGER_TYPE_TEXT) {
        serialize_scalar(&trigger_dest->text.total_display_time, trigger_source->text.total_display_time);
        serialize_scalar(&trigger_dest->text.curr_display_time, 0);
    }
    return sizeof(entity_trigger_t);
}

size_t entity_trigger_deserialize(int slot, const uint8_t* source) {
    entity_trigger_t* trigger = (entity_trigger_t*)entity_get_header(slot);
    *trigger = *(entity_trigger_t*)source;
    if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TELEPORT) {
        deserialize_vec3(&trigger->teleport.destination_pos, &trigger->teleport.destination_pos);
        deserialize_vec3(&trigger->teleport.destination_rotation_offset, &trigger->teleport.destination_rotation_offset);
    }
    else if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TEXT) {
        deserialize_scalar(&trigger->text.total_display_time, &trigger->text.total_display_time);
        deserialize_scalar(&trigger->text.curr_display_time, &trigger->text.curr_display_time);
    }
    return sizeof(entity_trigger_t);
}

void entity_trigger_update_text(int slot, scalar_t dt) {
    entity_trigger_t* trigger = (entity_trigger_t*)entity_get_header(slot);

    // display text for a specified amount of time
    if (trigger->text.curr_display_time > 0) {
        trigger->text.curr_display_time -= dt;
        const char* text = state.in_game.level.text_entries[trigger->text.id];
        renderer_draw_text((vec2_t){256 * ONE, 176 * ONE}, text, 2, 1, trigger->text.color);
    }

    // if the timer is up, either destroy the trigger, or reset it
    else if (trigger->destroy_on_player_intersect) {
        entity_kill(slot);
    }
    else {
        trigger->activated = 0;
    }
}

void entity_trigger_update_signal(int slot) {
    entity_trigger_t* trigger = (entity_trigger_t*)entity_get_header(slot);

    entity_set_signal(trigger->signal.id, trigger->signal.value_to_send);

    // if the timer is up, either destroy the trigger, or reset it
    if (trigger->destroy_on_player_intersect) {
        entity_kill(slot);
    }
    else {
        trigger->activated = 0;
    }
}

void entity_trigger_update_teleport(int slot, player_t* player) {
    entity_trigger_t* trigger = (entity_trigger_t*)entity_get_header(slot);

    player->position = trigger->teleport.destination_pos;

    // if the timer is up, either destroy the trigger, or reset it
    if (trigger->destroy_on_player_intersect) {
        entity_kill(slot);
    }
    else {
        trigger->activated = 0;
    }
}

entity_trigger_t* entity_trigger_new(void) {
    entity_trigger_t* entity = (entity_trigger_t*)entity_get_header(entity_alloc(ENTITY_TRIGGER));
    entity->entity_header.position = (vec3_t){0, 0, 0};
    entity->entity_header.rotation = (vec3_t){0, 0, 0};
    entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
    entity->destroy_on_player_intersect = 1;
    entity->intersecting_curr = 0;
    entity->intersecting_prev = 0;
    entity->activated = 0;
    entity->trigger_type = ENTITY_TRIGGER_TYPE_NONE;
    return entity;
}

void entity_trigger_update(int slot, player_t* player, scalar_t dt) {
    (void)player;
    entity_trigger_t* trigger = (entity_trigger_t*)entity_get_header(slot);

    // when the trigger is idle, detect player collision
    if (trigger->activated) {
        switch (trigger->trigger_type) {
            case ENTITY_TRIGGER_TYPE_NONE:     entity_trigger_update_text(slot, dt);         break;
            case ENTITY_TRIGGER_TYPE_TEXT:     entity_trigger_update_text(slot, dt);         break;
            case ENTITY_TRIGGER_TYPE_SIGNAL:   entity_trigger_update_signal(slot);           break;
            case ENTITY_TRIGGER_TYPE_TELEPORT: entity_trigger_update_teleport(slot, player); break;
        }

        return;
    }
    else {
        // throw this trigger into the collision universe
        const vec3_t trigger_pos = trigger->entity_header.position;
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

        // activate on enter
        if (trigger->intersecting_curr && !trigger->intersecting_prev) {
            trigger->activated = 1;

            if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TEXT) {
                trigger->text.curr_display_time = trigger->text.total_display_time;
            }
        }

        trigger->intersecting_prev = trigger->intersecting_curr;
        trigger->intersecting_curr = 0;
    }
}

void entity_trigger_on_hit(int slot, int hitbox_index) {
    (void)slot;
    (void)hitbox_index;
}

void entity_trigger_player_intersect(int slot, player_t* player) {
    (void)player;

    entity_trigger_t* trigger = (entity_trigger_t*)entity_get_header(slot);
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
