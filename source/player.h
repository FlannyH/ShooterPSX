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
} player_t;

void player_update(player_t* self, bvh_t* level_bvh, int dt_ms);
#endif // PLAYER_H
