#include "pickup.h"

#include "music.h"

entity_pickup_t* entity_pickup_new(void) {
	// Allocate memory for the entity
    entity_pickup_t* entity = (entity_pickup_t*)entity_get_header(entity_alloc(ENTITY_PICKUP));
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
    entity->type = PICKUP_TYPE_AMMO_SMALL;
    entity->entity_header.mesh = NULL;
    return entity;
}

void entity_pickup_update(int slot, player_t* player, int dt) {
	entity_pickup_t* pickup = (entity_pickup_t*)entity_get_header(slot);
	const vec3_t pickup_pos = pickup->entity_header.position;
	const vec3_t player_pos = vec3_sub(player->position, (vec3_t){0, 200 * COL_SCALE, 0});
    vec3_t pickup_to_player = vec3_sub(player_pos, pickup_pos);
    const scalar_t distance_from_pickup_to_player_squared = vec3_magnitude_squared(pickup_to_player);
	const int close_enough_to_home_in = (distance_from_pickup_to_player_squared < 2000 * ONE) && (distance_from_pickup_to_player_squared > 0);
	const int close_enough_to_collect = (distance_from_pickup_to_player_squared < 200 * ONE) && (distance_from_pickup_to_player_squared > 0);

    // Rendering
    if (pickup->entity_header.mesh == NULL) {
        switch (pickup->type) {
            case PICKUP_TYPE_AMMO_SMALL:   pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "08_pickup_ammo_small");   break;
            case PICKUP_TYPE_AMMO_BIG:     pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "09_pickup_ammo_big");     break;
            case PICKUP_TYPE_ARMOR_SMALL:  pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "10_pickup_armor_small");  break;
            case PICKUP_TYPE_ARMOR_BIG:    pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "11_pickup_armor_big");    break;
            case PICKUP_TYPE_HEALTH_SMALL: pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "12_pickup_health_small"); break;
            case PICKUP_TYPE_HEALTH_BIG:   pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "13_pickup_health_big");   break;
            case PICKUP_TYPE_KEY_BLUE:     pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "14_pickup_key_blue");     break;
            case PICKUP_TYPE_KEY_YELLOW:   pickup->entity_header.mesh = model_find_mesh(entity_get_models(), "15_pickup_key_yellow");   break;
        }

    PANIC_IF("Pickup entity could not find mesh!", pickup->entity_header.mesh == NULL);
    }

    // Rotate
    pickup->entity_header.rotation.y += dt * 50;

	transform_t render_transform;
    render_transform.position.x = -pickup_pos.x / COL_SCALE;
    render_transform.position.y = -pickup_pos.y / COL_SCALE;
    render_transform.position.z = -pickup_pos.z / COL_SCALE;
    render_transform.rotation.x = -pickup->entity_header.rotation.x;
    render_transform.rotation.y = -pickup->entity_header.rotation.y;
    render_transform.rotation.z = -pickup->entity_header.rotation.z;
	render_transform.scale.x = pickup->entity_header.scale.x;
	render_transform.scale.y = pickup->entity_header.scale.x;
	render_transform.scale.z = pickup->entity_header.scale.x;
#ifdef _LEVEL_EDITOR
	renderer_set_drawing_id(slot, 1);
#endif
	renderer_draw_mesh_shaded(pickup->entity_header.mesh, &render_transform, 0, 0, tex_entity_start);

    if (close_enough_to_home_in) {
        pickup_to_player = vec3_normalize(pickup_to_player);
        const scalar_t home_in_speed = 3 * ONE;
        pickup->entity_header.position = vec3_add(pickup_pos, vec3_muls(pickup_to_player, home_in_speed));
    }

    if (close_enough_to_collect) {
        int sfx_to_play = sfx_generic;
        switch (pickup->type) {
            case PICKUP_TYPE_AMMO_SMALL:    sfx_to_play = sfx_ammo; player->ammo += 5; break;          
            case PICKUP_TYPE_AMMO_BIG:      sfx_to_play = sfx_ammo; player->ammo += 40; break;         
            case PICKUP_TYPE_ARMOR_SMALL:   sfx_to_play = sfx_generic; player->armor += 5; break;         
            case PICKUP_TYPE_ARMOR_BIG:     sfx_to_play = sfx_armor_big; player->armor += 15; break;        
            case PICKUP_TYPE_HEALTH_SMALL:  sfx_to_play = sfx_health_small; player->health += 15; break;       
            case PICKUP_TYPE_HEALTH_BIG:    sfx_to_play = sfx_health_large; player->health += 40; break;       
            case PICKUP_TYPE_KEY_BLUE:      sfx_to_play = sfx_key; player->has_key_blue = 1; break;   
            case PICKUP_TYPE_KEY_YELLOW:    sfx_to_play = sfx_key; player->has_key_yellow = 1; break; 
        }
        audio_play_sound(sfx_to_play, 0, 0, (vec3_t){}, 1); 
        entity_kill(slot);
    }
}

void entity_pickup_on_hit(int slot, int hitbox_index) {
    (void)slot;
    (void)hitbox_index;
}

void entity_pickup_player_intersect(int slot, player_t *player) {
    (void)slot;
    (void)player;
}

#ifdef _LEVEL_EDITOR 
const char* pickup_names[] = {
    "PICKUP_TYPE_NONE",
    "PICKUP_TYPE_AMMO_SMALL",
    "PICKUP_TYPE_AMMO_BIG",
    "PICKUP_TYPE_ARMOR_SMALL",
    "PICKUP_TYPE_ARMOR_BIG",
    "PICKUP_TYPE_HEALTH_SMALL",
    "PICKUP_TYPE_HEALTH_BIG",
    "PICKUP_TYPE_KEY_BLUE",
    "PICKUP_TYPE_KEY_YELLOW",
    "PICKUP_TYPE_DAMAGE",
    "PICKUP_TYPE_FIRE_RATE",
    "PICKUP_TYPE_INVINCIBILITY",
	NULL
};
#endif
