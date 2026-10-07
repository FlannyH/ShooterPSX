#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/math/scalar.h"
#include "engine/structs.h"
#include "engine/level.h"
#include "engine/math/vec3.h"

#define MAX_HEALTH 100
#define MAX_ARMOR 50
#define MAX_AMMO 128

typedef struct {
    transform_t transform;
    vec3_t position;
    vec3_t velocity;
    vec3_t rotation;
    scalar_t footstep_timer;
    scalar_t seconds_since_on_ground;
    int ground_entity_id_prev; // -1 = no entity
    int ground_entity_id_curr; // -1 = no entity
    transform_t ground_entity_prev;
    transform_t ground_entity_curr;
    uint8_t health;
    uint8_t armor;
    uint8_t ammo;
    unsigned int has_key_blue : 1;
    unsigned int has_key_yellow : 1;
    unsigned int has_gun : 1;
    unsigned int is_grounded : 1;
} player_t;

#define PLAYER_VELOCITY_PRECISION 4
#define PLAYER_ROTATION_PRECISION 4
const static scalar_t eye_height = SCALAR(200);
const static scalar_t player_height = SCALAR(180);
const static scalar_t player_radius = SCALAR(160);
const static int32_t step_height = SCALAR(100);
const static scalar_t terminal_velocity_down = SCALAR(-8000.0);
const static scalar_t terminal_velocity_up = SCALAR(50000.0);
const static scalar_t gravity = SCALAR(-3200);
const static scalar_t walking_acceleration = SCALAR(15500);
const static scalar_t air_acceleration_multiplyer = SCALAR(0.4);
const static scalar_t walking_max_speed = SCALAR(600);
const static scalar_t drag = SCALAR(6.5);
const static scalar_t jump_drag_multiplyer = SCALAR(0.95);
const static int32_t initial_jump_velocity = SCALAR(4800);

void player_init(player_t* player, vec3_t position, vec3_t rotation, int health, int armor, int ammo);
void player_update(player_t* self, level_t* level, const scalar_t dt, const scalar_t time_counter);

#ifdef __cplusplus
}
#endif
#endif // PLAYER_H
