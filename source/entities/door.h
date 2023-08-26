#ifndef DOOR_H
#define DOOR_H
#include "../entity.h"

typedef struct {
	entity_header_t entity_header;
	vec3_t open_offset;
	scalar_t curr_interpolation_value;
	int is_open;
	int is_locked;
	int is_big_door;
} entity_door_t;

entity_door_t* entity_door_new();
void entity_door_update(int slot, player_t* player, int dt);

#endif