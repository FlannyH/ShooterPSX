#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "math/scalar.h"
#include "structs.h"
#include "level.h"
#include "math/vec3.h"

#define MAX_HEALTH 100
#define MAX_ARMOR 50
#define MAX_AMMO 128

typedef struct {
    transform_t transform;
    vec3_t position;
    vec3_t velocity;
    vec3_t rotation;
    scalar_t footstep_timer;
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
const static scalar_t eye_height = SCALAR(200);
const static scalar_t player_height = SCALAR(230);
const static scalar_t player_radius = SCALAR(320);
const static int32_t step_height = SCALAR(100);
const static scalar_t terminal_velocity_down = SCALAR(-5000.0);
const static scalar_t terminal_velocity_up = SCALAR(500.0);
const static scalar_t gravity = SCALAR(-1000);
const static scalar_t walking_acceleration = SCALAR(50);
const static scalar_t air_acceleration_divider = SCALAR(2);
const static scalar_t walking_max_speed = SCALAR(80);
const static scalar_t stick_sensitivity = SCALAR(1.0);
const static scalar_t mouse_sensitivity = SCALAR(0.2);
const static scalar_t drag = SCALAR(10);
const static scalar_t jump_drag_divider = SCALAR(2);
const static int32_t initial_jump_velocity = SCALAR(750);
const static int32_t jump_ground_threshold = 4000;

void player_init(player_t* player, vec3_t position, vec3_t rotation, int health, int armor, int ammo);
void player_update(player_t* self, level_t* level, const scalar_t dt, const scalar_t time_counter);

#ifdef __cplusplus
}
#endif
#endif // PLAYER_H
