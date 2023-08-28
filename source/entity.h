#ifndef ENTITY_H
#define ENTITY_H
#include "structs.h"
#include "player.h"
#include <stdint.h>
#include <stdio.h>

#define ENTITY_NOT_SECTION_BOUND 255
#define ENTITY_LIST_LENGTH 16

typedef enum {
	ENTITY_NONE,
	ENTITY_DOOR,
	ENTITY_PICKUP,
} entity_type_t; // typically represented by a uint8_t

typedef enum {
	ENTITY_MESH_DOOR_SMALL_LOCKED = 0,
	ENTITY_MESH_DOOR_SMALL_LOCKED_ROTATED,
	ENTITY_MESH_DOOR_SMALL_UNLOCKED,
	ENTITY_MESH_DOOR_SMALL_UNLOCKED_ROTATED,
	ENTITY_MESH_DOOR_BIG_LOCKED,
	ENTITY_MESH_DOOR_BIG_LOCKED_ROTATED,
	ENTITY_MESH_DOOR_BIG_UNLOCKED,
	ENTITY_MESH_DOOR_BIG_UNLOCKED_ROTATED,
	ENTITY_MESH_PICKUP_AMMO_SMALL,
	ENTITY_MESH_PICKUP_AMMO_BIG,
	ENTITY_MESH_PICKUP_ARMOR_SMALL,
	ENTITY_MESH_PICKUP_ARMOR_BIG,
	ENTITY_MESH_PICKUP_HEALTH_SMALL,
	ENTITY_MESH_PICKUP_HEALTH_BIG,
	ENTITY_MESH_PICKUP_KEY_BLUE,
	ENTITY_MESH_PICKUP_KEY_YELLOW,
	ENTITY_MESH_PICKUP_DAMAGE,
	ENTITY_MESH_PICKUP_FIRE_RATE,
	ENTITY_MESH_PICKUP_INVINCIBILITY,
} entity_mesh_id_t;

// All entities are assumed have this data. Add it as the first member of an entity struct.
typedef struct {
	vec3_t position;
	vec3_t rotation;
	vec3_t scale;
} entity_header_t;

typedef struct {
	uint8_t type;
	uint8_t curr_section;
	entity_header_t* data;
} entity_slot_t;

extern model_t* entity_models;
extern entity_slot_t entity_list[ENTITY_LIST_LENGTH];
extern aabb_t entity_aabb_queue[ENTITY_LIST_LENGTH];
extern size_t entity_n_active_aabb;
void entity_init();
int entity_register(entity_header_t* entity, uint8_t entity_type);
void entity_register_collision_box(const aabb_t* box);

void entity_update_all(player_t* player, int dt);
void entity_kill(int slot);

#ifdef _DEBUG
void entity_debug();
int entity_how_many_active();
#endif

#endif