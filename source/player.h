#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fixed_point.h"
#include "renderer.h"
#include "vec3.h"

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
const static int32_t eye_height = 200 * COL_SCALE;
const static int32_t player_radius = 100 * COL_SCALE;
const static int32_t step_height = 100 * COL_SCALE;
const static int32_t terminal_velocity_down = -32000;
const static int32_t terminal_velocity_up = 40000;
const static int32_t gravity = -5;
const static int32_t walking_acceleration = 45;
const static int32_t air_acceleration_divider = 2;
const static int32_t walking_max_speed = 12000;
const static int32_t stick_sensitivity = 3200;
const static int32_t mouse_sensitivity = 128000;
const static int32_t walking_drag = 44;
const static int32_t jump_drag_divider = 4;
const static int32_t initial_jump_velocity = 1200;
const static int32_t jump_ground_threshold = 4000;

void player_init(player_t* player, vec3_t position, vec3_t rotation, int health, int armor, int ammo);
void player_update(player_t* self, level_collision_t* level_bvh, const int dt_ms, const int time_counter);
int player_get_level_section(player_t* self, const vislist_t vis);

#ifdef __cplusplus
}
#endif
#endif // PLAYER_H
