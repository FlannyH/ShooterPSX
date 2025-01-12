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
        .max_speed = 32768,
        .drag = 128,
        .acceleration = 128,
    };
}

void debug_camera_update(debug_camera_t* self, const int dt_ms, const int register_input) {
    // Change max move speed based on scroll input
    if (input_mouse_scroll() > 0) {
        self->max_speed = scalar_mul(self->max_speed, scalar_from_float(1.1f));
    }
    if (input_mouse_scroll() < 0) {
        self->max_speed = scalar_mul(self->max_speed, scalar_from_float(1.0f / 1.1f));
    }

    if (register_input) {
        // Moving forwards and backwards
        self->velocity.x += hisin(self->transform.rotation.y) * input_left_stick_y(0) * (self->acceleration * dt_ms) >> 16;
        self->velocity.z -= hicos(self->transform.rotation.y) * input_left_stick_y(0) * (self->acceleration * dt_ms) >> 16;

        // Strafing left and right
        self->velocity.x += hicos(self->transform.rotation.y) * input_left_stick_x(0) * (self->acceleration * dt_ms) >> 16;
        self->velocity.z += hisin(self->transform.rotation.y) * input_left_stick_x(0) * (self->acceleration * dt_ms) >> 16;

        // Moving up and down
        self->velocity.y -= input_held(PAD_SQUARE, 0) ? self->acceleration * dt_ms * 127 : 0;
        self->velocity.y += input_held(PAD_CROSS, 0) ? self->acceleration * dt_ms * 127 : 0;
    }

    // Implement drag
    {
        // Calculate magnitude for velocity on X and Z axes
        const scalar_t max_speed = self->max_speed;
        const scalar_t drag = self->drag * dt_ms;
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
        if (velocity_scalar > max_speed) {
            velocity_scalar = max_speed - drag;
        }
        // Apply drag
        else if (velocity_scalar > drag) {
            velocity_scalar -= drag;
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

        // Now for the Y axis
        scalar_t velocity_y_scalar = abs(self->velocity.y);
        if (velocity_y_scalar > max_speed) {
            velocity_y_scalar = max_speed - drag;
        }
        else if (velocity_y_scalar > drag) {
            velocity_y_scalar -= drag;
        }
        else {           
            velocity_y_scalar = 0;
        }
        if (self->velocity.y < 0) velocity_y_scalar *= -1;
        self->velocity.y = velocity_y_scalar;
    }

    // Move the player based on velocity - subtract instead of add; view transform should be inverted
    self->transform.position = vec3_sub(self->transform.position, vec3_muls(self->velocity, dt_ms * ONE));

    if (register_input) {
        // Look up and down
        self->transform.rotation.x -= (int32_t)(input_mouse_movement_y() * 8) * (mouse_sensitivity) >> 12;
        if (self->transform.rotation.x > 32768) {
            self->transform.rotation.x = 32768;
        }
        if (self->transform.rotation.x < -32768) {
            self->transform.rotation.x = -32768;
        }
    
        // Look left and right
        self->transform.rotation.y += (int32_t)(input_mouse_movement_x() * 8) * (mouse_sensitivity) >> 12;
    }
}
