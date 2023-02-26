#include "player.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#include "input.h"

const int32_t eye_height = 56 << 12;
const int32_t player_radius = 35;
const int32_t step_height = 20 << 12;
const int32_t terminal_velocity_down = -2000;
const int32_t terminal_velocity_up = 8000;
const int32_t gravity = -11;
const int32_t walking_acceleration = 15;
const int32_t walking_max_speed = 1800;
const int32_t stick_sensitivity = 6000;
const int32_t walking_drag = 30;
const int32_t initial_jump_velocity = 3200;
const int32_t jump_ground_threshold = 1024;

const pixel32_t white = { 255, 255, 255, 255 };
transform_t t_level = { {0,0,0},{0,0,0},{-4096,-4096,-4096} };

void check_ground_collision(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    // Cast a ray to the ground
    rayhit_t hit = { 0 };
    ray_t ray;
    ray.position = self->position;
    ray.position.x.raw = ray.position.x.raw;
    ray.position.y.raw = ray.position.y.raw;
    ray.position.z.raw = ray.position.z.raw;
    ray.direction = vec3_from_int32s(0, -4096, 0);
    ray.inv_direction = vec3_div(vec3_from_int32s(4096, 4096, 4096), ray.direction);
    bvh_intersect_ray(level_bvh, ray, &hit);
    self->distance_from_ground = hit.distance;

    // If the ray hit nothing, return
    if (hit.distance.raw == INT32_MAX) {
        return;
    }

    if (self->velocity.y.raw > 0) {
        return;
    }
    
    if (
        self->distance_from_ground.raw <= eye_height &&
        self->position.y.raw > hit.position.y.raw + step_height
    ) {
        // Set speed to 0
        self->velocity.y.raw = 0;

        // Set player camera height to eye_height units above the ground
        self->position.y.raw = (hit.position.y.raw + eye_height);
    }
}

void apply_gravity(player_t* self, const int dt_ms) {
    self->velocity.y = scalar_add(self->velocity.y, scalar_from_int32(gravity * dt_ms));
    if (self->velocity.y.raw > terminal_velocity_up) {
        self->velocity.y.raw = terminal_velocity_up;
    }
    else if (self->velocity.y.raw < terminal_velocity_down) {
        self->velocity.y.raw = terminal_velocity_down;
    }
}

void handle_stick_input(player_t* self, const int dt_ms) {
    if (input_has_analog(0)) {

        // Moving forwards and backwards
        self->velocity.x.raw += hisin(self->rotation.y.raw) * input_left_stick_y(0) * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z.raw += hicos(self->rotation.y.raw) * input_left_stick_y(0) * (walking_acceleration * dt_ms) >> 12;

        // Strafing left and right
        self->velocity.x.raw -= hicos(self->rotation.y.raw) * input_left_stick_x(0) * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z.raw += hisin(self->rotation.y.raw) * input_left_stick_x(0) * (walking_acceleration * dt_ms) >> 12;

        // Look up and down
        self->rotation.x.raw -= (int32_t)(input_right_stick_y(0)) * (stick_sensitivity * dt_ms) >> 12;
        if (self->rotation.x.raw > 32768) {
            self->rotation.x.raw = 32768;
        }
        if (self->rotation.x.raw < -32768) {
            self->rotation.x.raw = -32768;
        }

        // Look left and right
        self->rotation.y.raw += (int32_t)(input_right_stick_x(0)) * (stick_sensitivity * dt_ms) >> 12;
    }
}

void handle_drag(player_t* self, const int dt_ms) {
    // Calculate magnitude for velocity on X and Z axes
    scalar_t velocity_x = self->velocity.x;
    scalar_t velocity_z = self->velocity.z;
    const scalar_t velocity_x2 = scalar_mul(velocity_x, velocity_x);
    const scalar_t velocity_z2 = scalar_mul(velocity_z, velocity_z);
    const scalar_t velocity_magnitude_squared = scalar_add(velocity_x2, velocity_z2);
    scalar_t velocity_scalar = scalar_sqrt(velocity_magnitude_squared);

    // Normalize the speed
    velocity_x = scalar_div(velocity_x, velocity_scalar);
    velocity_z = scalar_div(velocity_z, velocity_scalar);

    // Clamp magnitude
    if (velocity_scalar.raw > walking_max_speed) {
        velocity_scalar.raw = walking_max_speed;
    }
    // Apply drag
    else if (velocity_scalar.raw > walking_drag * dt_ms) {
        velocity_scalar.raw -= walking_drag * dt_ms;
    }
    else {           
        velocity_scalar.raw = 0;
    }

    // Apply new magnitude to the velocity
    velocity_x = scalar_mul(velocity_x, (velocity_scalar));
    velocity_z = scalar_mul(velocity_z, (velocity_scalar));

    // Put it back in the velocity component
    self->velocity.x = velocity_x;
    self->velocity.z = velocity_z;
}

void handle_jump(player_t* self) {
    if (input_pressed(PAD_CROSS, 0)) {
        if (self->distance_from_ground.raw - eye_height < jump_ground_threshold) {
            self->velocity.y.raw = initial_jump_velocity;
        }
    }
}

void handle_movement(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    // Where are we headed this frame?
    vec3_t velocity;
    velocity.x.raw = self->velocity.x.raw * dt_ms;
    velocity.y.raw = self->velocity.y.raw * dt_ms;
    velocity.z.raw = self->velocity.z.raw * dt_ms;

    // Calculate the target position based on that
    const vec3_t target_position = vec3_add(self->position, velocity);

    // For now just set it, don't check
    self->position = target_position;
}

void player_update(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    apply_gravity(self, dt_ms);
    check_ground_collision(self, level_bvh, dt_ms);
    handle_stick_input(self, dt_ms);
    handle_drag(self, dt_ms);
    handle_jump(self);
    handle_movement(self, level_bvh, dt_ms);

    self->transform.position.vx = -self->position.x.raw;
    self->transform.position.vy = -self->position.y.raw;
    self->transform.position.vz = -self->position.z.raw;
    self->transform.rotation.vx = -self->rotation.x.raw;
    self->transform.rotation.vy = -self->rotation.y.raw;
    self->transform.rotation.vz = -self->rotation.z.raw;
}
