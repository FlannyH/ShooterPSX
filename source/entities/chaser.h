#ifndef CHASER_H
#define CHASER_H
#include "../entity.h"

typedef struct {
    entity_header_t entity_header;
} entity_chaser_t;

entity_chaser_t* entity_chaser_new();
void entity_chaser_update(int slot, player_t* player, int dt);
void entity_chaser_on_hit(int slot, int hitbox_index);

#endif