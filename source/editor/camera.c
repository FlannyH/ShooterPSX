#include "camera.h"

#include "../player.h"
#include "../input.h"
#include "math/vec3.h"
#include "math/vec2.h"

debug_camera_t debug_camera_new(void) {
    return (debug_camera_t) {
        .transform = {
            .position = vec3_from_scalar(0),
            .rotation = vec3_from_scalar(0),
            .scale = vec3_from_scalar(ONE)
        },
        .velocity = vec3_from_scalar(0),
        .max_speed = SCALAR(100),
        .drag = SCALAR(7),
        .acceleration = SCALAR(14),
    };
}

void debug_camera_update(debug_camera_t* self, const scalar_t dt, const int register_input) {
    // Change max move speed based on scroll input
    if (input_mouse_scroll() > 0) {
        self->max_speed = scalar_mul(self->max_speed, scalar_from_float(1.1f));
    }
    if (input_mouse_scroll() < 0) {
        self->max_speed = scalar_mul(self->max_speed, scalar_from_float(1.0f / 1.1f));
    }

    if (register_input) {
        // Moving forwards and backwards
        const vec3_t forward = (vec3_t) {
            trig_sin(self->transform.rotation.y),
            0,
            trig_cos(self->transform.rotation.y)
        };

        const vec3_t right = (vec3_t) {
            forward.z,
            0,
            -forward.x
        };

        const vec2_t stick_left = (vec2_t) {
            SCALAR((float)input_left_stick_x(0) / 127.0f),
            SCALAR((float)input_left_stick_y(0) / 127.0f)
        };

        const vec2_t stick_right = (vec2_t) {
            SCALAR((float)input_right_stick_x(0) / 127.0f),
            SCALAR((float)input_right_stick_y(0) / 127.0f)
        };

        const vec2_t mouse_delta = (vec2_t) {
            SCALAR(input_mouse_movement_x()),
            SCALAR(input_mouse_movement_y())
        };

        // Moving horizontally
        self->velocity = vec3_add(self->velocity, vec3_muls(forward, scalar_mul(scalar_mul(stick_left.y, self->acceleration * PLAYER_VELOCITY_PRECISION), dt)));
        self->velocity = vec3_add(self->velocity, vec3_muls(right, scalar_mul(scalar_mul(stick_left.x, self->acceleration * PLAYER_VELOCITY_PRECISION), dt)));

        // Moving up and down
        if (input_held(PAD_SQUARE, 0)) self->velocity.y -= scalar_mul(self->acceleration * PLAYER_VELOCITY_PRECISION, dt);
        if (input_held(PAD_CROSS, 0))  self->velocity.y += scalar_mul(self->acceleration * PLAYER_VELOCITY_PRECISION, dt);

        // Looking up and down
        self->transform.rotation.x += scalar_mul(scalar_mul(-stick_right.y, dt), stick_sensitivity);
        self->transform.rotation.x += scalar_mul(scalar_mul(-mouse_delta.y, dt), mouse_sensitivity);
        self->transform.rotation.x = scalar_clamp(self->transform.rotation.x, SCALAR(-0.22), SCALAR(0.22));

        // Looking left and right
        self->transform.rotation.y += scalar_mul(scalar_mul(-stick_right.x, dt), stick_sensitivity);
        self->transform.rotation.y += scalar_mul(scalar_mul(-mouse_delta.x, dt), mouse_sensitivity);
    }

    // Implement drag
    {
        scalar_t curr_drag = scalar_mul(drag, dt);

        const scalar_t length = vec3_magnitude(self->velocity);
        const vec3_t dir = vec3_divs(self->velocity, length);
        if (length > (walking_max_speed * PLAYER_VELOCITY_PRECISION)) {
            self->velocity = vec3_muls(dir, length);
        }
        else if (length > (curr_drag * PLAYER_VELOCITY_PRECISION)) {
            self->velocity = vec3_sub(self->velocity, vec3_muls(dir, curr_drag * PLAYER_VELOCITY_PRECISION));
        }
        else {
            self->velocity.x = 0;
            self->velocity.y = 0;
            self->velocity.z = 0;
        }
    }

    // Move the player based on velocity
    self->transform.position = vec3_add(self->transform.position, vec3_divs(vec3_muls(self->velocity, dt), PLAYER_VELOCITY_PRECISION));

    if (register_input) {
        // Look up and down
        self->transform.rotation.x -= (int32_t)(input_mouse_movement_y() * 8) * (mouse_sensitivity) >> 12;
        if (self->transform.rotation.x > ONE/4) {
            self->transform.rotation.x = ONE/4;
        }
        if (self->transform.rotation.x < -ONE/4) {
            self->transform.rotation.x = -ONE/4;
        }

        // Look left and right
        self->transform.rotation.y -= (int32_t)(input_mouse_movement_x() * 8) * (mouse_sensitivity) >> 12;
    }
}
