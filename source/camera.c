#include "camera.h"

#ifdef _PSX
#include <psxgte.h>
#include <psxpad.h>
#else
#include "win/psx.h"
#endif

#include "input.h"
#include "renderer.h"

void camera_update_flycam(Transform* camera_transform) {
    if (input_is_connected(0))
    {
        // For dual-analog and dual-shock (analog input)
        if (input_has_analog(0)) {

            // Moving forwards and backwards
            camera_transform->position.vx -= (((hisin(camera_transform->rotation.vy) * hicos(camera_transform->rotation.vx)) >> 12) * (input_left_stick_y(0))) >> 2;
            camera_transform->position.vy += (hisin(camera_transform->rotation.vx) * (input_left_stick_y(0))) >> 2;
            camera_transform->position.vz += (((hicos(camera_transform->rotation.vy) * hicos(camera_transform->rotation.vx)) >> 12) * (input_left_stick_y(0))) >> 2;

            // Strafing left and right
            camera_transform->position.vx -= (hicos(camera_transform->rotation.vy) * (input_left_stick_x(0))) >> 2;
            camera_transform->position.vz -= (hisin(camera_transform->rotation.vy) * (input_left_stick_x(0))) >> 2;

            // Look up and down
            camera_transform->rotation.vx += (int32_t)(input_right_stick_y(0)) << 3;

            // Look left and right
            camera_transform->rotation.vy -= (int32_t)(input_right_stick_x(0)) << 3;
        }
        if (input_held(PAD_R1, 0)) {
            // Slide up
            camera_transform->position.vx -= ((hisin(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) >> 2;
            camera_transform->position.vy -= hicos(camera_transform->rotation.vx) >> 2;
            camera_transform->position.vz += ((hicos(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) >> 2;
        }

        if (input_held(PAD_R2, 0)) {

            // Slide down
            camera_transform->position.vx += ((hisin(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) >> 2;
            camera_transform->position.vy += hicos(camera_transform->rotation.vx) >> 2;
            camera_transform->position.vz -= ((hicos(camera_transform->rotation.vy) * hisin(camera_transform->rotation.vx))) >> 2;
        }
    }
}
