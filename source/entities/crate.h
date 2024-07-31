#ifndef CRATE_H
#define CRATE_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	entity_header_t entity_header;
    uint8_t pickup_to_spawn;
} entity_crate_t;

entity_crate_t* entity_crate_new(void);
void entity_crate_update(int slot, player_t* player, int dt);
void entity_crate_on_hit(int slot, int hitbox_index);

#ifdef __cplusplus
}
#endif

#endif