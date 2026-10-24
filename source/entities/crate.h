#ifndef CRATE_H
#define CRATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/entity.h"

typedef struct {
	entity_header_t entity_header;
    uint8_t pickup_to_spawn;
} entity_crate_t;

entity_crate_t* entity_crate_new(void);
size_t entity_crate_deserialize(int slot, const uint8_t* data);
size_t entity_crate_serialize(int slot, uint8_t* data);
void entity_crate_update(int slot, player_t* player, scalar_t dt);
void entity_crate_on_hit(int slot, int hitbox_index);
void entity_crate_player_intersect(int slot, player_t* player);

#ifdef __cplusplus
}
#endif

#endif
