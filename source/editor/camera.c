#include "camera.h"

#include "../input_mapping.h"
#include "../player.h"
#include "input_map.h"
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

void move_look(debug_camera_t* self, const scalar_t dt, const int register_input) {
    if (!register_input) return;

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

    const vec2_t move = (vec2_t) {
        input_mapping_value(IM_MOVE_X, 0),
        input_mapping_value(IM_MOVE_Y, 0)
    };

    const vec2_t look = (vec2_t) {
        input_mapping_value(IM_LOOK_X, 0),
        input_mapping_value(IM_LOOK_Y, 0)
    };

    // Moving horizontally
    self->velocity = vec3_add(self->velocity, vec3_muls(forward, scalar_mul(scalar_mul(move.y, self->acceleration * PLAYER_VELOCITY_PRECISION), dt)));
    self->velocity = vec3_add(self->velocity, vec3_muls(right, scalar_mul(scalar_mul(move.x, self->acceleration * PLAYER_VELOCITY_PRECISION), dt)));

    // Moving up and down
    if (input_mapping_held(IM_CAMERA_DOWN, 0)) self->velocity.y -= scalar_mul(self->acceleration * PLAYER_VELOCITY_PRECISION, dt);
    if (input_mapping_held(IM_CAMERA_UP, 0))  self->velocity.y += scalar_mul(self->acceleration * PLAYER_VELOCITY_PRECISION, dt);

    // Looking up and down
    self->transform.rotation.x += scalar_mul(-look.y, dt);
    self->transform.rotation.x = scalar_clamp(self->transform.rotation.x, SCALAR(-0.22), SCALAR(0.22));

    // Looking left and right
    self->transform.rotation.y += scalar_mul(-look.x, dt);

    if (self->transform.rotation.x > ONE/4) {
        self->transform.rotation.x = ONE/4;
    }
    if (self->transform.rotation.x < -ONE/4) {
        self->transform.rotation.x = -ONE/4;
    }
}

void handle_drag(debug_camera_t* self, const scalar_t dt, const int register_input) {
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

void debug_camera_update(debug_camera_t* self, const scalar_t dt, const int register_input) {
    // Change max move speed based on scroll input
    if (input_mapping_value(IM_CAMERA_SPEED_UP, 0) > 0) {
        self->max_speed = scalar_mul(self->max_speed, scalar_from_float(1.1f));
    }
    if (input_mapping_value(IM_CAMERA_SPEED_DOWN, 0) > 0) {
        self->max_speed = scalar_mul(self->max_speed, scalar_from_float(1.0f / 1.1f));
    }

    move_look(self, dt, register_input);

    handle_drag(self, dt, register_input);

    // Move the player based on velocity
    self->transform.position = vec3_add(self->transform.position, vec3_divs(vec3_muls(self->velocity, dt), PLAYER_VELOCITY_PRECISION));
}
