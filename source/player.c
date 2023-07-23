#include "player.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#include "input.h"

transform_t t_level = { {0,0,0},{0,0,0},{-4096,-4096,-4096} };

const int32_t eye_height = 200 * COL_SCALE;
const int32_t player_radius = 150 * COL_SCALE;
const int32_t step_height = 100 * COL_SCALE;
const int32_t terminal_velocity_down = -12400 / 8;
const int32_t terminal_velocity_up = 40000 / 8;
const int32_t gravity = -5;
const int32_t walking_acceleration = 50 / 8;
const int32_t walking_max_speed = 6660 / 8;
const int32_t stick_sensitivity = 3200;
const int32_t walking_drag = 32 / 8;
const int32_t initial_jump_velocity = 8000 / 8;
const int32_t jump_ground_threshold = 16000 / 8;
static scalar_t player_radius_squared = 0;
int32_t is_grounded = 0;
int n_sections;
int sections[N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE];

void check_ground_collision(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    WARN_IF("player radius squared was not computed, and is equal to 0", player_radius_squared == 0);

    // Cast a cylinder from the player's feet + step height, down to the ground
    int32_t distance_to_check = 12000000;
    rayhit_t hit = { 0 };
    const vertical_cylinder_t player = {
        .bottom = vec3_sub(self->position, vec3_from_int32s(0, distance_to_check, 0)),
        .height = distance_to_check + step_height,
        .radius = player_radius,
        .radius_squared = player_radius_squared
    };
    bvh_intersect_vertical_cylinder(level_bvh, player, &hit);

    // If nothing was hit, there is no ground below the player. Ignore the rest of this function
    if (hit.distance == INT32_MAX)
        return;

    // Check the Y distance from the ground to the player's feet
    const scalar_t distance = self->position.y - eye_height - hit.position.y;
    if (distance <= (-self->velocity.y * dt_ms)) {
        // Set speed to 0
        if (self->velocity.y < 0)
            self->velocity.y = 0;

        // Set player camera height to eye_height units above the ground
        self->position.y = (hit.position.y + eye_height);
    }
}

void apply_gravity(player_t* self, const int dt_ms) {
    if (is_grounded) return;

    self->velocity.y = (self->velocity.y + gravity * dt_ms);
    if (self->velocity.y > terminal_velocity_up) {
        self->velocity.y = terminal_velocity_up;
    }
    else if (self->velocity.y < terminal_velocity_down) {
        self->velocity.y = terminal_velocity_down;
    }
}

void handle_stick_input(player_t* self, const int dt_ms) {
    if (input_has_analog(0)) {

        // Moving forwards and backwards
        self->velocity.x += hisin(self->rotation.y) * input_left_stick_y(0) * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hicos(self->rotation.y) * input_left_stick_y(0) * (walking_acceleration * dt_ms) >> 12;

        // Strafing left and right
        self->velocity.x -= hicos(self->rotation.y) * input_left_stick_x(0) * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hisin(self->rotation.y) * input_left_stick_x(0) * (walking_acceleration * dt_ms) >> 12;

        // Look up and down
        self->rotation.x -= (int32_t)(input_right_stick_y(0)) * (stick_sensitivity * dt_ms) >> 12;
        if (self->rotation.x > 32768) {
            self->rotation.x = 32768;
        }
        if (self->rotation.x < -32768) {
            self->rotation.x = -32768;
        }

        // Look left and right
        self->rotation.y += (int32_t)(input_right_stick_x(0)) * (stick_sensitivity * dt_ms) >> 12;

        // Debug
        if (input_held(PAD_UP, 0)) {
            self->position.y += 40960;
        }
        if (input_held(PAD_DOWN, 0)) {
            self->position.y -= 40960;
        }
    } else {
        // Look left and right
        const int32_t dpad_x = ((int32_t)(input_held(PAD_RIGHT, 0) != 0) * 127) + ((int32_t)(input_held(PAD_LEFT, 0) != 0) * -127);
        self->rotation.y += dpad_x * (stick_sensitivity * dt_ms) >> 12;
        
        // Moving forwards and backwards
        const int32_t dpad_y = ((int32_t)(input_held(PAD_UP, 0) != 0) * -127) + ((int32_t)(input_held(PAD_DOWN, 0) != 0) * 127);
        self->velocity.x += hisin(self->rotation.y) * dpad_y * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hicos(self->rotation.y) * dpad_y * (walking_acceleration * dt_ms) >> 12;
    }
}

void handle_drag(player_t* self, const int dt_ms) {
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
        velocity_scalar = walking_max_speed - (walking_drag * dt_ms);
    }
    // Apply drag
    else if (velocity_scalar > walking_drag * dt_ms) {
        velocity_scalar -= walking_drag * dt_ms;
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

void handle_jump(player_t* self) {
    if (input_pressed(PAD_CROSS, 0)) {
        if (self->distance_from_ground - eye_height < jump_ground_threshold) {
            self->velocity.y = initial_jump_velocity;
        }
    }
}

void handle_movement(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    // Move the player, ask questions later
    self->position.x += self->velocity.x * dt_ms;
    self->position.z += self->velocity.z * dt_ms;

    // Don't check collision if we aren't moving
    if ((self->velocity.x | self->velocity.y | self->velocity.z) == 0) {
        //return;
    }

    for (size_t i = 0; i < 2; ++i) {
        // Collide
        const vertical_cylinder_t cyl = {
            .bottom = (vec3_t){self->position.x, self->position.y - eye_height - 4096 + step_height, self->position.z},
            .height = eye_height + 4096 - step_height,
            .radius = player_radius,
            .radius_squared = player_radius_squared,
            .is_wall_check = 1,
        };
        rayhit_t hit;
        bvh_intersect_vertical_cylinder(level_bvh, cyl, &hit);

        // Did we hit anything?
        if (!is_infinity(hit.distance)) {
            int is_wall = (hit.normal.y <= 0);
            // If this is a wall
            if (is_wall) {
                // Eject player out of geometry
                const vec3_t amount_to_eject = (vec3_t){
                    scalar_mul(hit.normal.x, player_radius - hit.distance),
                    scalar_mul(hit.normal.y, hit.distance),
                    scalar_mul(hit.normal.z, player_radius - hit.distance),
                };
                self->position = vec3_add(self->position, amount_to_eject);

                // Absorb penetration force (this sounds hella sus)
                scalar_t velocity_length = scalar_sqrt(vec3_magnitude_squared(self->velocity));
                vec3_t velocity_normalized = vec3_divs(self->velocity, velocity_length);
                vec3_t undesired_motion = vec3_muls(hit.normal, vec3_dot(velocity_normalized, hit.normal));
                vec3_t desired_motion = vec3_sub(velocity_normalized, undesired_motion);
                self->velocity = vec3_muls(desired_motion, velocity_length);
            }
        }
    }
    self->position.y += self->velocity.y * dt_ms;
}

void player_update(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    if (player_radius_squared == 0) {
        const scalar_t player_radius_scalar = player_radius;
        player_radius_squared = scalar_mul(player_radius_scalar, player_radius_scalar);
    }
    is_grounded = 0;
#ifndef DEBUG_CAMERA
    apply_gravity(self, dt_ms);
    check_ground_collision(self, level_bvh, dt_ms);
#endif
    handle_stick_input(self, dt_ms);
    handle_drag(self, dt_ms);
    handle_jump(self);
    handle_movement(self, level_bvh, dt_ms);
    //vec3_debug(self->position);
    self->transform.position.vx = -self->position.x * (4096 / COL_SCALE);
    self->transform.position.vy = -self->position.y * (4096 / COL_SCALE);
    self->transform.position.vz = -self->position.z * (4096 / COL_SCALE);
    self->transform.rotation.vx = -self->rotation.x;
    self->transform.rotation.vy = -self->rotation.y;
    self->transform.rotation.vz = -self->rotation.z;
}


int player_get_level_section(player_t* self, const model_t* model) {
    self->position.x = -self->position.x / COL_SCALE;
    self->position.y = -self->position.y / COL_SCALE;
    self->position.z = -self->position.z / COL_SCALE;
    n_sections = 0;
    for (size_t i = 0; i < model->n_meshes; ++i) {
        if (n_sections == N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE) break;
        if (vertical_cylinder_aabb_intersect(&model->meshes[i].bounds, (vertical_cylinder_t){
            .bottom = self->position,
            .height = eye_height,
            .radius = player_radius,
            .radius_squared = player_radius_squared,
        })) {
            sections[n_sections] = i;
            n_sections += 1;
        }
    }
    return n_sections; // -1 means no section
}