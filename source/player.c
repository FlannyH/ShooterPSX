#include "player.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#include "input.h"
#include "entity.h"
#include "common.h"
#include <string.h>

transform_t t_level = { {0,0,0},{0,0,0},{-4096,-4096,-4096} };

static scalar_t player_radius_squared = 0;
int32_t is_grounded = 0;
int n_sections;
int sections[N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE];

void check_ground_collision(player_t* self, bvh_t* level_bvh, const int dt_ms) {
    WARN_IF("player radius squared was not computed, and is equal to 0", player_radius_squared == 0);

    // Cast a cylinder from the player's feet + step height, down to the ground
    int32_t distance_to_check = 120000;
    rayhit_t hit = { 0 };
    const vertical_cylinder_t player = {
        .bottom = vec3_sub(self->position, vec3_from_int32s(0, distance_to_check, 0)),
        .height = distance_to_check + step_height,
        .radius = player_radius,
        .radius_squared = player_radius_squared
    };
    bvh_intersect_vertical_cylinder(level_bvh, player, &hit);
    for (size_t i = 0; i < entity_n_active_aabb; ++i) {
        rayhit_t curr_hit;
        if (!entity_aabb_queue[i].is_solid) continue;
        if (!vertical_cylinder_aabb_intersect_fancy(&entity_aabb_queue[i].aabb, player, &curr_hit)) continue;
        curr_hit.normal = (vec3_t){0, ONE, 0};
        if (curr_hit.distance < hit.distance) memcpy(&hit, &curr_hit, sizeof(rayhit_t));
    }

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
    const int sensitivity = input_mouse_connected() ? mouse_sensitivity : stick_sensitivity;
    if (input_has_analog(0)) {

        // Moving forwards and backwards
        self->velocity.x += hisin(self->rotation.y) * input_left_stick_y(0) * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hicos(self->rotation.y) * input_left_stick_y(0) * (walking_acceleration * dt_ms) >> 12;

        // Strafing left and right
        self->velocity.x -= hicos(self->rotation.y) * input_left_stick_x(0) * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hisin(self->rotation.y) * input_left_stick_x(0) * (walking_acceleration * dt_ms) >> 12;

        // Look up and down
        self->rotation.x -= (int32_t)(input_right_stick_y(0)) * (sensitivity * dt_ms) >> 12;
        if (self->rotation.x > 32768) {
            self->rotation.x = 32768;
        }
        if (self->rotation.x < -32768) {
            self->rotation.x = -32768;
        }

        // Look left and right
        self->rotation.y += (int32_t)(input_right_stick_x(0)) * (sensitivity * dt_ms) >> 12;

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
        self->rotation.y += dpad_x * (sensitivity * dt_ms) >> 12;
        
        // Moving forwards and backwards
        const int32_t dpad_y = ((int32_t)(input_held(PAD_UP, 0) != 0) * -127) + ((int32_t)(input_held(PAD_DOWN, 0) != 0) * 127);
        self->velocity.x += hisin(self->rotation.y) * dpad_y * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hicos(self->rotation.y) * dpad_y * (walking_acceleration * dt_ms) >> 12;

        // Strafing left and right
        const int32_t shoulder_x = ((int32_t)(input_held(PAD_L1, 0) != 0) * -127) + ((int32_t)(input_held(PAD_R1, 0) != 0) * 127);
        self->velocity.x -= hicos(self->rotation.y) * shoulder_x * (walking_acceleration * dt_ms) >> 12;
        self->velocity.z += hisin(self->rotation.y) * shoulder_x * (walking_acceleration * dt_ms) >> 12;
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
#ifndef DEBUG_CAMERA
        bvh_intersect_vertical_cylinder(level_bvh, cyl, &hit);
        for (size_t i = 0; i < entity_n_active_aabb; ++i) {
            rayhit_t curr_hit;
            curr_hit.distance = INT32_MAX;
            if (!entity_aabb_queue[i].is_solid) continue;
            if (!vertical_cylinder_aabb_intersect_fancy(&entity_aabb_queue[i].aabb, cyl, &curr_hit)) continue;
            if (curr_hit.distance < hit.distance) memcpy(&hit, &curr_hit, sizeof(rayhit_t));
        }
#else
        hit.distance = INT32_MAX;
#endif

        // Did we hit anything?
        if (!is_infinity(hit.distance)) {
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
    self->position.y += self->velocity.y * dt_ms;
}

void player_update(player_t* self, bvh_t* level_bvh, const int dt_ms, const int time_counter) {
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
    
    vec2_t vel_2d = {self->velocity.x, self->velocity.z};
    scalar_t speed_1d = vec2_magnitude(vel_2d);
    self->transform.position.x = -self->position.x * (4096 / COL_SCALE);
#ifdef DEBUG_CAMERA
    self->transform.position.y = -self->position.y * (4096 / COL_SCALE);
#else
    self->transform.position.y = -self->position.y * (4096 / COL_SCALE) + isin(time_counter * 12) * speed_1d / 64;
#endif
    self->transform.position.z = -self->position.z * (4096 / COL_SCALE);
    self->transform.rotation.x = -self->rotation.x;
    self->transform.rotation.y = -self->rotation.y;
    self->transform.rotation.z = -self->rotation.z;
}

void draw_structure(const vislist_t vis) {
    uint32_t node_stack[32] = {0};
    uint32_t node_handle_ptr = 0;
    uint32_t node_add_ptr = 1;

    while ((node_handle_ptr != node_add_ptr) && (n_sections < N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE)) {
        // check a node
        visbvh_node_t* node = &vis.bvh_root[node_stack[node_handle_ptr]];
        aabb_t aabb = {
            .min = {
                .x = (scalar_t)(-node->min.x) * COL_SCALE,
                .y = (scalar_t)(-node->min.y) * COL_SCALE,
                .z = (scalar_t)(-node->min.z) * COL_SCALE,
            },
            .max = {
                .x = (scalar_t)(-node->max.x) * COL_SCALE,
                .y = (scalar_t)(-node->max.y) * COL_SCALE,
                .z = (scalar_t)(-node->max.z) * COL_SCALE,
            },
        };
        renderer_debug_draw_aabb(&aabb, white, &id_transform);

            // If the node is an interior node
            if ((node->child_or_vis_index & 0x80000000) == 0) {
                // Add the 2 children to the stack
                node_stack[node_add_ptr] = node->child_or_vis_index;
                node_add_ptr = (node_add_ptr + 1) % 32;
                node_stack[node_add_ptr] = node->child_or_vis_index + 1;
                node_add_ptr = (node_add_ptr + 1) % 32;
            }
            else {
                // Add this node index to the list
                sections[n_sections++] = node->child_or_vis_index & 0x7fffffff;
            }
        
        

        node_handle_ptr = (node_handle_ptr + 1) % 32;
    }

}

int player_get_level_section(player_t* self, const vislist_t vis) {
    // Get player position
    svec3_t position = {
        -self->position.x / COL_SCALE,
        -self->position.y / COL_SCALE,
        -self->position.z / COL_SCALE,
    };
    n_sections = 0;

    // Find all the vis leaf nodes we're currently inside of
    uint32_t node_stack[32] = {0};
    uint32_t node_handle_ptr = 0;
    uint32_t node_add_ptr = 1;

    while ((node_handle_ptr != node_add_ptr) && (n_sections < N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE)) {
        // check a node
        visbvh_node_t* node = &vis.bvh_root[node_stack[node_handle_ptr]];

        // If a node was hit
        if (
            position.x >= node->min.x &&  position.x <= node->max.x &&
            position.y >= node->min.y &&  position.y <= node->max.y &&
            position.z >= node->min.z &&  position.z <= node->max.z
        ) {
            // If the node is an interior node
            if ((node->child_or_vis_index & 0x80000000) == 0) {
                // Add the 2 children to the stack
                node_stack[node_add_ptr] = node->child_or_vis_index;
                node_add_ptr = (node_add_ptr + 1) % 32;
                node_stack[node_add_ptr] = node->child_or_vis_index + 1;
                node_add_ptr = (node_add_ptr + 1) % 32;
            }
            else {
                // Add this node index to the list
                sections[n_sections++] = node->child_or_vis_index & 0x7fffffff;
            }
        }
        

        node_handle_ptr = (node_handle_ptr + 1) % 32;
    }

    return n_sections; // -1 means no section
}