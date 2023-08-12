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
} entity_type_t; // typically represented by a uint8_t

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
void entity_init();
int entity_register(entity_header_t* entity, uint8_t entity_type);

void entity_update_all(player_t* player, int dt);

#ifdef _DEBUG
void entity_debug();
int entity_how_many_active();
#endif

#endif