#include "camera.h"

#include "../player.h" 
#include "../input.h"  

debug_camera_t debug_camera_new(void) {
    return (debug_camera_t) {
        .transform = {
            .position = vec3_from_scalar(0),
            .rotation = vec3_from_scalar(0),
            .scale = vec3_from_scalar(ONE)
        },
        .velocity = vec3_from_scalar(0),
        .move_speed = ONE,
        .acceleration = 0,
    };
}

void debug_camera_update(debug_camera_t* self, const int dt_ms) {
    // Moving forwards and backwards
    {
        self->velocity.x += hisin(self->transform.rotation.y) * input_left_stick_y(0) * (self->acceleration) >> 16;
        self->velocity.z += hicos(self->transform.rotation.y) * input_left_stick_y(0) * (self->acceleration) >> 16;

        // Strafing left and right
        self->velocity.x -= hicos(self->transform.rotation.y) * input_left_stick_x(0) * (self->acceleration) >> 16;
        self->velocity.z += hisin(self->transform.rotation.y) * input_left_stick_x(0) * (self->acceleration) >> 16;

        // Moving up and down
        self->velocity.y += input_held(PAD_SQUARE, 0) ? self->acceleration : 0;
        self->velocity.y -= input_held(PAD_CROSS, 0) ? self->acceleration : 0;
    }

    // Implement drag
    {
        // Calculate magnitude for velocity on X and Z axes
        scalar_t velocity_x = self->velocity.x;
        scalar_t velocity_z = self->velocity.z;
        const scalar_t velocity_x2 = scalar_mul(velocity_x, velocity_x);
        const scalar_t velocity_z2 = scalar_mul(velocity_z, velocity_z);
        const scalar_t velocity_magnitude_squared = (velocity_x2 + velocity_z2);
        scalar_t velocity_scalar = scalar_sqrt(velocity_magnitude_squared);

        // Normalize the speed
        velocity_x = scalar_div(velocity_x, velocity_scalar);
        velocity_z = scalar_div(velocity_z, velocity_scalar);

        // Clamp magnitude
        if (velocity_scalar > walking_max_speed) {
            velocity_scalar = walking_max_speed - walking_drag;
        }
        // Apply drag
        else if (velocity_scalar > walking_drag) {
            velocity_scalar -= walking_drag;
        }
        else {           
            velocity_scalar = 0;
        }

        // Apply new magnitude to the velocity
        velocity_x = scalar_mul(velocity_x, (velocity_scalar));
        velocity_z = scalar_mul(velocity_z, (velocity_scalar));

        // Put it back in the velocity component
        self->velocity.x = velocity_x;
        self->velocity.z = velocity_z;
    }

    // Move the player based on velocity
    self->transform.position = vec3_add(self->transform.position, vec3_muls(self->velocity, (dt_ms * ONE) / 1000));

    // Look up and down
    self->transform.rotation.x -= (int32_t)(input_right_stick_y(0)) * (mouse_sensitivity * dt_ms) >> 12;
    if (self->transform.rotation.x > 32768) {
        self->transform.rotation.x = 32768;
    }
    if (self->transform.rotation.x < -32768) {
        self->transform.rotation.x = -32768;
    }

    // Look left and right
    self->transform.rotation.y += (int32_t)(input_right_stick_x(0)) * (mouse_sensitivity * dt_ms) >> 12;
}
