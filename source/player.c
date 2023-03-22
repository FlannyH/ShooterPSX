#include "player.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#include "input.h"

const int32_t eye_height = 56 << 12;
const int32_t player_radius = 20 << 12;
const int32_t step_height = 20 << 12;
const int32_t terminal_velocity_down = -2000;
const int32_t terminal_velocity_up = 8000;
const int32_t gravity = -11;
const int32_t walking_acceleration = 15;
const int32_t walking_max_speed = 1100;
const int32_t stick_sensitivity = 6000;
const int32_t walking_drag = 30;
const int32_t initial_jump_velocity = 3200;
const int32_t jump_ground_threshold = 4000;
static scalar_t player_radius_squared = { .raw = 0 };

transform_t t_level = { {0,0,0},{0,0,0},{-4096,-4096,-4096} };

void check_ground_collision(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    WARN_IF("player radius squared was not computed, and is equal to 0", player_radius_squared.raw == 0);

    // Cast a sphere at the player's feet
    rayhit_t hit = { 0 };
    const vertical_cylinder_t player = {
        .bottom = vec3_sub(self->position, vec3_from_int32s(0, eye_height, 0)),
        .height.raw = step_height,
        .radius.raw = player_radius,
        .radius_squared = player_radius_squared
    };
    const sphere_t player_s = {
        .center = vec3_sub(self->position, vec3_from_int32s(0, eye_height, 0)),
        .radius.raw = player_radius,
        .radius_squared = player_radius_squared
    };

    bvh_intersect_vertical_cylinder(level_bvh, player, &hit);
    const sphere_t dbg = {
        .center = hit.position,
        .radius.raw = player_radius,
        .radius_squared = player_radius_squared
    };
    //bvh_intersect_sphere(level_bvh, player_s, &hit);

    if (hit.distance.raw != INT32_MAX) {
        // Set speed to 0
        if (self->velocity.y.raw < 0)
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
    if (input_held(PAD_UP, 0)) {
        self->position.y.raw += 4096;
    }
    if (input_held(PAD_DOWN, 0)) {
        self->position.y.raw -= 4096;
    }
}

void handle_movement(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    for (int i = 0; i < 1; ++i) {
        // Where are we headed this frame?
        vec3_t velocity;
        velocity.x.raw = ((i + 1) * self->velocity.x.raw / 1) * dt_ms;
        velocity.y.raw = 0;
        velocity.z.raw = ((i + 1) * self->velocity.z.raw / 1) * dt_ms;

        if ((velocity.x.raw | velocity.y.raw | velocity.z.raw) == 0) {
            break;
        }

        // Calculate the target position based on velocity
        vec3_t target_position = vec3_add(self->position, velocity);

        // Intersect with the geometry twice times
        rayhit_t hit;
        ray_t ray;
        ray.position = vec3_sub(self->position, vec3_from_int32s(0, eye_height - step_height, 0));
        ray.direction = vec3_normalize(velocity);
        ray.inv_direction = vec3_div(vec3_from_scalar(scalar_from_int32(4096)), ray.direction);
        ray.length.raw = walking_max_speed * 2;

        bvh_intersect_ray(level_bvh, ray, &hit);

        // If the ray hit something, move towards the hit position
        if (scalar_mul(hit.distance, hit.distance).raw < vec3_magnitude_squared(velocity).raw + player_radius_squared.raw) {
            //target_position = vec3_sub(hit.position, ray.direction);
            target_position.y = self->position.y;
            //self->velocity.x.raw = 0;
            //self->velocity.z.raw = 0;
        }

        for (int j = 0; j < 4; ++j) {
            // Create sphere
            const sphere_t sphere = {
                .center = target_position,
                .radius = {.raw = player_radius},
                .radius_squared = player_radius_squared
            };

            bvh_intersect_sphere(level_bvh, sphere, &hit);

            // If the sphere intersection hit something, move towards the hit position
            if (hit.distance.raw != INT32_MAX) {
                if (vec3_dot(velocity, vec3_sub(hit.position, target_position)).raw > 0) {
                    const scalar_t separation_distance = scalar_sub(sphere.radius, scalar_abs(hit.distance_along_normal));
                    const vec3_t separation_vector = vec3_mul(hit.normal, vec3_from_scalar(separation_distance));
                    target_position = vec3_add(target_position, separation_vector);
                    target_position.y = self->position.y;
                }
            }
            else {
                break;
            }
        }

        // Set the position
        if (!input_held(PAD_R1, 0)) {
            self->position.x = target_position.x;
            self->position.z = target_position.z;
        }
    }
    self->position.y.raw += self->velocity.y.raw * dt_ms; // we do this one separately to save the hassle
}

void player_update(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    if (player_radius_squared.raw == 0) {
        const scalar_t player_radius_scalar = scalar_from_int32(player_radius);
        player_radius_squared = scalar_mul(player_radius_scalar, player_radius_scalar);
    }

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
