#ifndef CHASER_H
#define CHASER_H
#include "../entity.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    int16_t behavior_timer;
    vec3_t last_known_player_pos;
    vec3_t home_position;
    vec3_t velocity; // scaled by 256 for precision sake
} entity_chaser_t;

entity_chaser_t* entity_chaser_new(void);
void entity_chaser_update(int slot, player_t* player, int dt);
void entity_chaser_on_hit(int slot, int hitbox_index);
void entity_chaser_player_intersect(int slot, player_t* player);

const static int32_t chaser_acceleration = 600;
const static int32_t chaser_max_speed = 70000;
const static int32_t chaser_drag = 100;

#ifdef __cplusplus
}
#endif

#endif