#include "camera.h"

#ifdef _PSX
#include <psxgte.h>
#include <psxpad.h>
#else
#include "win/psx.h"
#endif

#include "input.h"
#include "renderer.h"

// These are in 20.12 fixed point
#define STICK_SENSITIVITY 5000
#define MOVEMENT_SPEED 96

void camera_update_flycam(transform_t* camera_transform, int dt_ms) {
    if (input_is_connected(0))
    {
        // For dual-analog and dual-shock (analog input)
        if (input_has_analog(0)) {

            // Moving forwards and backwards
            camera_transform->position.vx -= (((hisin(camera_transform->rotation.vy) * hicos(camera_transform->rotation.vx)) >> 12) * (input_left_stick_y(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vy += (hisin(camera_transform->rotation.vx) * (input_left_stick_y(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vz += (((hicos(camera_transform->rotation.vy) * hicos(camera_transform->rotation.vx)) >> 12) * (input_left_stick_y(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;

            // Strafing left and right
            camera_transform->position.vx -= (hicos(camera_transform->rotation.vy) * (input_left_stick_x(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vz -= (hisin(camera_transform->rotation.vy) * (input_left_stick_x(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;

            // Look up and down
            camera_transform->rotation.vx += (int32_t)(input_right_stick_y(0)) * (STICK_SENSITIVITY * dt_ms) >> 12;

            // Look left and right
            camera_transform->rotation.vy -= (int32_t)(input_right_stick_x(0)) * (STICK_SENSITIVITY * dt_ms) >> 12;
        }
        if (input_held(PAD_R1, 0)) {
            // Slide up
            camera_transform->position.vx -= ((hisin(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vy -= hicos(camera_transform->rotation.vx) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vz += ((hicos(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) * (MOVEMENT_SPEED * dt_ms) >> 12;
        }

        if (input_held(PAD_R2, 0)) {

            // Slide down
            camera_transform->position.vx += ((hisin(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vy += hicos(camera_transform->rotation.vx) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.vz -= ((hicos(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) * (MOVEMENT_SPEED * dt_ms) >> 12;
        }
    }
}
