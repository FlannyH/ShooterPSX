#ifndef DOOR_H
#define DOOR_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	entity_header_t entity_header;
	vec3_t open_offset;
	scalar_t curr_interpolation_value;
	unsigned int is_open : 1;
	unsigned int is_locked : 1;
	unsigned int is_big_door : 1;
	unsigned int is_rotated : 1;
	unsigned int state_changed : 1;
} entity_door_t;

entity_door_t* entity_door_new(void);
void entity_door_update(int slot, player_t* player, int dt);
void entity_door_on_hit(int slot, int hitbox_index);
void entity_door_player_intersect(int slot, player_t* player);

#ifdef __cplusplus
}
#endif
#endif
