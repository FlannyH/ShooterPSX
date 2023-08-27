#ifndef PLAYER_H
#define PLAYER_H
#include "fixed_point.h"
#include "renderer.h"

typedef struct {
    transform_t transform;
    vec3_t position;
    vec3_t velocity;
    vec3_t rotation;
    scalar_t distance_from_ground;
    int has_key_blue : 1;
    int has_key_yellow : 1;
} player_t;

void player_update(player_t* self, bvh_t* level_bvh, int dt_ms);
int player_get_level_section(player_t* self, const model_t* model);
#endif // PLAYER_H
