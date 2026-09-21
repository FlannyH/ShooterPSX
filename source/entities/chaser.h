#ifndef CHASER_H
#define CHASER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../entity.h"

typedef enum {
    CHASER_WAIT,
    CHASER_RETURN_HOME,
    CHASER_CHASE,
    CHASER_FLEE,
    CHASER_STRAFE,
    CHASER_SHOOT,
} entity_chaser_state_t;

typedef struct {
    entity_header_t entity_header;
    int16_t curr_navmesh_node;
    int16_t target_navmesh_node;
    int16_t state;
    scalar_t behavior_timer;
    vec3_t last_known_player_pos;
    vec3_t home_position;
    vec3_t velocity;
} entity_chaser_t;

entity_chaser_t* entity_chaser_new(void);
void entity_chaser_update(int slot, player_t* player, scalar_t dt);
void entity_chaser_on_hit(int slot, int hitbox_index);
void entity_chaser_player_intersect(int slot, player_t* player);

const static scalar_t chaser_acceleration = SCALAR(20);
const static scalar_t chaser_max_speed = SCALAR(300);
const static scalar_t chaser_drag = SCALAR(4);

#ifdef __cplusplus
}
#endif

#endif
