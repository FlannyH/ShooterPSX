#include "camera.h"

#ifdef _PSX
#include <psxgte.h>
#include <psxpad.h>
#elif defined(_NDS)
#include "nds/psx.h"
#else
#include "windows/psx.h"
#endif

#include "input.h"
#include "renderer.h"

// These are in 20.12 fixed point
#define STICK_SENSITIVITY 2000
#define MOVEMENT_SPEED 64

void camera_update_flycam(transform_t* camera_transform, int dt_ms) {
    if (input_is_connected(0))
    {
        // For dual-analog and dual-shock (analog input)
        if (input_has_analog(0)) {

            // Moving forwards and backwards
            camera_transform->position.x -= (((hisin(camera_transform->rotation.y) * hicos(camera_transform->rotation.x)) >> 12) * (input_left_stick_y(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.y += (hisin(camera_transform->rotation.x) * (input_left_stick_y(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.z += (((hicos(camera_transform->rotation.y) * hicos(camera_transform->rotation.x)) >> 12) * (input_left_stick_y(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;

            // Strafing left and right
            camera_transform->position.x -= (hicos(camera_transform->rotation.y) * (input_left_stick_x(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.z -= (hisin(camera_transform->rotation.y) * (input_left_stick_x(0))) * (MOVEMENT_SPEED * dt_ms) >> 12;

            // Look up and down
            camera_transform->rotation.x += (int32_t)(input_right_stick_y(0)) * (STICK_SENSITIVITY * dt_ms) >> 12;

            // Look left and right
            camera_transform->rotation.y -= (int32_t)(input_right_stick_x(0)) * (STICK_SENSITIVITY * dt_ms) >> 12;
        }
        if (input_held(PAD_R1, 0)) {
            // Slide up
            camera_transform->position.x -= ((hisin(camera_transform->rotation.y) * hisin(camera_transform->rotation.x))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.y -= hicos(camera_transform->rotation.x) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.z += ((hicos(camera_transform->rotation.y) * hisin(camera_transform->rotation.x))) * (MOVEMENT_SPEED * dt_ms) >> 12;
        }

        if (input_held(PAD_R2, 0)) {

            // Slide down
            camera_transform->position.x += ((hisin(camera_transform->rotation.y) * hisin(camera_transform->rotation.x))) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.y += hicos(camera_transform->rotation.x) * (MOVEMENT_SPEED * dt_ms) >> 12;
            camera_transform->position.z -= ((hicos(camera_transform->rotation.y) * hisin(camera_transform->rotation.x))) * (MOVEMENT_SPEED * dt_ms) >> 12;
        }
    }
}
