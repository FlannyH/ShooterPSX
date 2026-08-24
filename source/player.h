#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "math/scalar.h"
#include "collision.h"
#include "renderer.h"
#include "math/vec3.h"

#define MAX_HEALTH 100
#define MAX_ARMOR 50
#define MAX_AMMO 128

typedef struct {
    transform_t transform;
    vec3_t position;
    vec3_t velocity;
    vec3_t rotation;
    int footstep_timer;
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

#define PLAYER_VELOCITY_PRECISION 16
const static scalar_t eye_height = SCALAR(200);
const static scalar_t player_height = SCALAR(200);
const static scalar_t player_radius = SCALAR(160);
// const static int32_t step_height = SCALAR(100);
const static scalar_t terminal_velocity_down = SCALAR(-5.0);
// const static int32_t terminal_velocity_up = 40000;
const static scalar_t gravity = SCALAR(-5.0);
const static scalar_t walking_acceleration = SCALAR(16.0);
const static scalar_t air_acceleration_scalar = SCALAR(0.5);
const static scalar_t walking_max_speed = SCALAR(1.5);
const static scalar_t stick_sensitivity = SCALAR(0.25);
const static scalar_t mouse_sensitivity = SCALAR(2.0);
const static scalar_t walking_drag = SCALAR(8.0);
const static scalar_t vertical_drag = SCALAR(2.0);
const static int32_t jump_drag_scalar = 4;
// const static int32_t initial_jump_velocity = 1200;
// const static int32_t jump_ground_threshold = 4000;

void player_init(player_t* player, vec3_t position, vec3_t rotation, int health, int armor, int ammo);
void player_update(player_t* self, level_collision_t* level_bvh, const int dt_ms, const int time_counter);
int player_get_level_section(player_t* self, const vislist_t vis);

#ifdef __cplusplus
}
#endif
#endif // PLAYER_H
