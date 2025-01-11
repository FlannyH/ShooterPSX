#ifndef CAMERA_H
#define CAMERA_H

#include "../pc/psx.h"
#include "../structs.h"
#include "../vec3.h" 

typedef struct {
    transform_t transform;
    vec3_t velocity;
    scalar_t move_speed;
    scalar_t acceleration;
} debug_camera_t;

debug_camera_t debug_camera_new(void);
void debug_camera_update(debug_camera_t* self, const int dt_ms);

#endif
