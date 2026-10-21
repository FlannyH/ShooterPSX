#include "camera.h"

#include "engine/input_mapping.h"
#include "input_map.h"
#include "engine/math/vec3.h"
#include "engine/math/vec2.h"

debug_camera_t debug_camera_new(void) {
    return (debug_camera_t) {
        .transform = {
            .position = vec3_from_scalar(0),
            .rotation = vec3_from_scalar(0),
            .scale = vec3_from_scalar(ONE)
        },
        .velocity = vec3_from_scalar(0),
        .max_speed = SCALAR(2500),
        .drag = SCALAR(4000),
        .acceleration = SCALAR(7500),
    };
}

void move_look(debug_camera_t* self, const scalar_t dt) {
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

    const vec2_t look_mouse = (vec2_t) {
        input_mapping_value(IM_LOOK_MOUSE_X, 0),
        input_mapping_value(IM_LOOK_MOUSE_Y, 0)
    };

    const vec2_t look_stick = (vec2_t) {
        input_mapping_value(IM_LOOK_STICK_X, 0),
        input_mapping_value(IM_LOOK_STICK_Y, 0)
    };

    const vec2_t move_acc = vec2_muls(move, self->acceleration);
    const vec2_t move_acc_dt = vec2_muls(move_acc, dt);

    // Moving horizontally
    self->velocity = vec3_add(self->velocity, vec3_muls(forward, move_acc_dt.y));
    self->velocity = vec3_add(self->velocity, vec3_muls(right, move_acc_dt.x));

    // Moving up and down
    if (input_mapping_held(IM_CAMERA_DOWN, 0)) self->velocity.y -= scalar_mul(self->acceleration, dt);
    if (input_mapping_held(IM_CAMERA_UP, 0))  self->velocity.y += scalar_mul(self->acceleration, dt);

    // Looking up and down
    self->transform.rotation.x += scalar_mul(look_stick.y, dt) + look_mouse.y;
    self->transform.rotation.x = scalar_clamp(self->transform.rotation.x, SCALAR(-0.22), SCALAR(0.22));

    // Looking left and right
    self->transform.rotation.y += scalar_mul(look_stick.x, dt) + look_mouse.x;
}

void handle_drag(debug_camera_t* self, const scalar_t dt) {
    scalar_t curr_drag = scalar_mul(self->drag, dt);

    const scalar_t length = vec3_magnitude(self->velocity);
    const vec3_t dir = vec3_divs(self->velocity, length);
    if (length > self->max_speed) {
        self->velocity = vec3_muls(dir, self->max_speed);
    }
    else if (length > curr_drag) {
        self->velocity = vec3_sub(self->velocity, vec3_muls(dir, curr_drag));
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
        self->max_speed = scalar_mul(self->max_speed, SCALAR(1.1f));
    }
    if (input_mapping_value(IM_CAMERA_SPEED_DOWN, 0) > 0) {
        self->max_speed = scalar_mul(self->max_speed, SCALAR(1.0f / 1.1f));
    }

    if (register_input) {
        move_look(self, dt);
    }
    handle_drag(self, dt);

    // Move the player based on velocity
    self->transform.position = vec3_add(self->transform.position, vec3_muls(self->velocity, dt));
}
