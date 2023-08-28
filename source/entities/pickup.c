#include "pickup.h"

entity_pickup_t* entity_pickup_new() {
	// Allocate memory for the entity
	entity_pickup_t* entity = mem_stack_alloc(sizeof(entity_pickup_t), STACK_ENTITY);
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
    entity->type = PICKUP_TYPE_AMMO_SMALL;
    entity_register(&entity->entity_header, ENTITY_PICKUP);
    return entity;
}

void entity_pickup_update(int slot, player_t* player, int dt) {
	entity_pickup_t* pickup = (entity_pickup_t*)entity_list[slot].data;
	vec3_t pickup_pos = pickup->entity_header.position;
	vec3_t player_pos = player->position;
    vec3_t pickup_to_player = vec3_sub(player_pos, pickup_pos);
    scalar_t distance_from_door_to_player_squared = vec3_magnitude_squared(pickup_to_player);
	int close_enough_to_home_in = (distance_from_door_to_player_squared < 2000 * ONE) && (distance_from_door_to_player_squared > 0);
	int close_enough_to_collect = (distance_from_door_to_player_squared < 100 * ONE) && (distance_from_door_to_player_squared > 0);

    // Rendering
    size_t mesh_to_render = 0;
    switch (pickup->type) {
        case PICKUP_TYPE_AMMO_SMALL:   mesh_to_render = ENTITY_MESH_PICKUP_AMMO_SMALL;   break;
        case PICKUP_TYPE_AMMO_BIG:     mesh_to_render = ENTITY_MESH_PICKUP_AMMO_BIG;     break;
        case PICKUP_TYPE_ARMOR_SMALL:  mesh_to_render = ENTITY_MESH_PICKUP_ARMOR_SMALL;  break;
        case PICKUP_TYPE_ARMOR_BIG:    mesh_to_render = ENTITY_MESH_PICKUP_ARMOR_BIG;    break;
        case PICKUP_TYPE_HEALTH_SMALL: mesh_to_render = ENTITY_MESH_PICKUP_HEALTH_SMALL; break;
        case PICKUP_TYPE_HEALTH_BIG:   mesh_to_render = ENTITY_MESH_PICKUP_HEALTH_BIG;   break;
    }

    // Rotate
    pickup->entity_header.rotation.y += dt * 50;

	transform_t render_transform;
    render_transform.position.vx = -pickup_pos.x / COL_SCALE;
    render_transform.position.vy = -pickup_pos.y / COL_SCALE;
    render_transform.position.vz = -pickup_pos.z / COL_SCALE;
    render_transform.rotation.vx = -pickup->entity_header.rotation.x;
    render_transform.rotation.vy = -pickup->entity_header.rotation.y;
    render_transform.rotation.vz = -pickup->entity_header.rotation.z;
	render_transform.scale.vx = pickup->entity_header.scale.x;
	render_transform.scale.vy = pickup->entity_header.scale.x;
	render_transform.scale.vz = pickup->entity_header.scale.x;
	renderer_draw_mesh_shaded_offset(&entity_models->meshes[mesh_to_render], &render_transform, 64);

    if (close_enough_to_home_in) {
        vec3_t pickup_to_player = vec3_divs(pickup_to_player, distance_from_door_to_player_squared);
        const scalar_t home_in_speed = 1 * ONE;
        pickup->entity_header.position = vec3_add(pickup_pos, vec3_muls(pickup_to_player, home_in_speed));
    }

    if (close_enough_to_collect) {
        switch (pickup->type) {
            case PICKUP_TYPE_AMMO_SMALL: player->ammo += 5; break;
            case PICKUP_TYPE_AMMO_BIG: player->ammo += 40; break;
            case PICKUP_TYPE_ARMOR_SMALL: player->armor += 5; break;
            case PICKUP_TYPE_ARMOR_BIG: player->armor += 15; break;
            case PICKUP_TYPE_HEALTH_SMALL: player->health += 15; break;
            case PICKUP_TYPE_HEALTH_BIG: player->health += 15; break;
        }
        printf("pickup was picked up\n");
        entity_kill(slot);
    }
}