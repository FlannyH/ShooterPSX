#ifndef TRIGGER_H
#define TRIGGER_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    entity_header_t entity_header;
    uint8_t destroy_on_player_intersect : 1;
    uint8_t intersecting_curr : 1;
    uint8_t intersecting_prev : 1;
} entity_trigger_t;

entity_trigger_t* entity_trigger_new(void);
void entity_trigger_update(int slot, player_t* player, int dt);
void entity_trigger_on_hit(int slot, int hitbox_index);
void entity_trigger_player_intersect(int slot, player_t* player);

#ifdef __cplusplus
}
#endif

#endif