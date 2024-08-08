#ifndef CHASER_H
#define CHASER_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHASER_WAIT,
    CHASER_RETURN_HOME,
    CHASER_CHASE_PLAYER,
    CHASER_FLEE_PLAYER,
} entity_chaser_state_t;

typedef struct {
    entity_header_t entity_header;
    int curr_navmesh_node;
    int target_navmesh_node;
    entity_chaser_state_t state;
    int behavior_timer;
} entity_chaser_t;

entity_chaser_t* entity_chaser_new(void);
void entity_chaser_update(int slot, player_t* player, int dt);
void entity_chaser_on_hit(int slot, int hitbox_index);

#ifdef __cplusplus
}
#endif

#endif