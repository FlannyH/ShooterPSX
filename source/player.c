#include "player.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#include "input.h"
#include "entity.h"
#include "common.h"
#include "music.h"
#include "random.h"

#include <string.h>

#define FOOTSTEP_TIMER_MAX 350

transform_t t_level = { {0,0,0},{0,0,0},{-4096,-4096,-4096} };

static scalar_t player_radius_squared = 0;

void player_init(player_t* player, vec3_t position, vec3_t rotation, int health, int armor, int ammo) {
    player->transform = (transform_t){
        .position = vec3_from_scalar(0),
        .rotation = vec3_from_scalar(0),
        .scale = vec3_from_scalar(ONE)
    };
    player->position = position;
    player->velocity = vec3_from_scalar(0);
    player->rotation = rotation;
    player->footstep_timer = 0;
    player->ground_entity_id_prev = -1;
    player->ground_entity_id_curr = -1;
    player->ground_entity_prev = (transform_t){0};
    player->ground_entity_curr = (transform_t){0};
    player->health = health;
    player->armor = armor;
    player->ammo = ammo;
    player->has_key_blue = 0;
    player->has_key_yellow = 0;
    player->has_gun = 1;
    player->is_grounded = 1;
}

void check_ground_collision(player_t* self, level_collision_t* level_bvh, const int dt_ms) {
    WARN_IF("player radius squared was not computed, and is equal to 0", player_radius_squared == 0);

    if (self->ground_entity_id_curr != -1) {
        const entity_header_t* entity = entity_get_header(self->ground_entity_id_curr);
        self->ground_entity_prev = self->ground_entity_curr;
        self->ground_entity_curr = (transform_t){
            .position = entity->position,
            .rotation = entity->rotation,
            .scale = entity->scale,
        };
    }

    // If we entered the entity this frame, notify the entity
    if (self->ground_entity_id_prev == -1 && self->ground_entity_id_curr != -1) {
        entity_send_player_intersect(self->ground_entity_id_curr, self);
    }

    // If we're on an entity this frame, move the player along with it    
    if (self->ground_entity_id_prev == self->ground_entity_id_curr && self->ground_entity_id_curr != -1) {
        self->position = vec3_add(self->position, vec3_sub(self->ground_entity_curr.position, self->ground_entity_prev.position));
        self->rotation = vec3_add(self->rotation, vec3_sub(self->ground_entity_curr.rotation, self->ground_entity_prev.rotation));
    }
    // If we left the entity this frame, add momentum to the player
    else if (self->ground_entity_id_prev != -1 && self->ground_entity_id_curr == -1) {
        self->velocity = vec3_add(self->velocity, vec3_divs(vec3_sub(self->ground_entity_curr.position, self->ground_entity_prev.position), ONE * dt_ms));
    }

    self->ground_entity_id_prev = self->ground_entity_id_curr;
    self->ground_entity_id_curr = -1;

    // Cast a cylinder from the player's feet + step height, down to the ground
    const int32_t distance_to_check = 120000;
    rayhit_t hit = { 0 };
    const vertical_cylinder_t player = {
        .bottom = vec3_sub(self->position, vec3_from_int32s(0, distance_to_check, 0)),
        .height = distance_to_check + step_height,
        .radius = player_radius,
        .radius_squared = player_radius_squared,
        .is_wall_check = 0,
    };
    bvh_intersect_vertical_cylinder(level_bvh, player, &hit);

    const size_t n_active_aabb = entity_get_n_active_aabb();
    for (size_t i = 0; i < n_active_aabb; ++i) {
        const entity_collision_box_t* const box = entity_get_aabb_queue_entry(i);
        rayhit_t curr_hit;
        if (!box->is_solid && !box->is_trigger) continue;

        int intersect = vertical_cylinder_aabb_intersect_fancy(&box->aabb, player, &curr_hit);
        if (intersect && box->is_trigger) {
            entity_send_player_intersect(box->entity_index, self);
        }
        if (!box->is_solid) continue;
        if (!vertical_cylinder_aabb_intersect_fancy(&box->aabb, player, &curr_hit)) continue;
        if (!intersect) continue;
        if (curr_hit.distance < hit.distance) memcpy(&hit, &curr_hit, sizeof(rayhit_t));
        hit.type = RAY_HIT_TYPE_ENTITY_HITBOX;
        hit.entity_hitbox.entity_index = box->entity_index;
        hit.entity_hitbox.box_index = box->box_index;
        hit.entity_hitbox.not_move_player_along = box->not_move_player_along;
    }

    // If nothing was hit, there is no ground below the player. Ignore the rest of this function
    self->is_grounded = 0;
    if (hit.distance == INT32_MAX)
        return;

    // If the player is standing on an entity, move the player along with the entity
    if (hit.type == RAY_HIT_TYPE_ENTITY_HITBOX && !hit.entity_hitbox.not_move_player_along) {
        self->ground_entity_id_curr = hit.entity_hitbox.entity_index;
    }

    // Check the Y distance from the ground to the player's feet
    const scalar_t distance = self->position.y - eye_height - hit.position.y;
    if (distance <= (-self->velocity.y * dt_ms)) {
        // Set speed to 0
        if (self->velocity.y < 0)
            self->velocity.y = 0;

        // Set player camera height to eye_height units above the ground
        self->position.y = (hit.position.y + eye_height);
        
        self->is_grounded = 1;
    }
}

void apply_gravity(player_t* self, const int dt_ms) {
    if (self->is_grounded) return;

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
    const scalar_t curr_acceleration = (self->is_grounded) ? (walking_acceleration * dt_ms) : ((walking_acceleration * dt_ms) / air_acceleration_divider);

    if (input_has_analog(0)) {
        // Moving forwards and backwards
        self->velocity.x += hisin(self->rotation.y) * input_left_stick_y(0) * (curr_acceleration) >> 16;
        self->velocity.z += hicos(self->rotation.y) * input_left_stick_y(0) * (curr_acceleration) >> 16;

        // Strafing left and right
        self->velocity.x -= hicos(self->rotation.y) * input_left_stick_x(0) * (curr_acceleration) >> 16;
        self->velocity.z += hisin(self->rotation.y) * input_left_stick_x(0) * (curr_acceleration) >> 16;

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
#ifdef _DEBUG
        if (input_held(PAD_UP, 0)) {
            self->position.y += 40960;
            self->velocity.y = 0;
        }
        if (input_held(PAD_DOWN, 0)) {
            self->position.y -= 40960;
            self->velocity.y = 0;
        }
#endif
    } else {
        // Look left and right
        const int32_t dpad_x = ((int32_t)(input_held(PAD_RIGHT, 0) != 0) * 127) + ((int32_t)(input_held(PAD_LEFT, 0) != 0) * -127);
        self->rotation.y += dpad_x * (sensitivity * dt_ms) >> 12;
        
        // Moving forwards and backwards
        const int32_t dpad_y = ((int32_t)(input_held(PAD_UP, 0) != 0) * -127) + ((int32_t)(input_held(PAD_DOWN, 0) != 0) * 127);
        self->velocity.x += hisin(self->rotation.y) * dpad_y * (curr_acceleration) >> 16;
        self->velocity.z += hicos(self->rotation.y) * dpad_y * (curr_acceleration) >> 16;

        // Strafing left and right
        const int32_t shoulder_x = ((int32_t)(input_held(PAD_L1, 0) != 0) * -127) + ((int32_t)(input_held(PAD_R1, 0) != 0) * 127);
        self->velocity.x -= hicos(self->rotation.y) * shoulder_x * (curr_acceleration) >> 16;
        self->velocity.z += hisin(self->rotation.y) * shoulder_x * (curr_acceleration) >> 16;
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
    const scalar_t curr_drag = (self->is_grounded) ? (walking_drag * dt_ms) : ((walking_drag * dt_ms) / jump_drag_divider);
    if (velocity_scalar > walking_max_speed) {
        velocity_scalar = walking_max_speed - curr_drag;
    }
    // Apply drag
    else if (velocity_scalar > curr_drag) {
        velocity_scalar -= curr_drag;
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

int was_grounded = 0;

void handle_jump(player_t* self) {
    if (self->is_grounded && input_pressed(PAD_CROSS, 0)) {
        self->velocity.y = initial_jump_velocity;
        audio_play_sound(sfx_jump_land1, ONE, 0, (vec3_t){}, 1);
    }
    if (!was_grounded && self->is_grounded) audio_play_sound(sfx_jump_land2, ONE, 0, (vec3_t){}, 1);
    was_grounded = self->is_grounded;
}

void handle_movement(player_t* self, level_collision_t* level_bvh, const int dt_ms) {
    // Move the player, ask questions later
    self->position.x += self->velocity.x * dt_ms;
    self->position.z += self->velocity.z * dt_ms;

    for (size_t i = 0; i < 2; ++i) {
        rayhit_t hit = {};
#ifndef _DEBUG_CAMERA
        // Collide
        const vertical_cylinder_t cyl = {
            .bottom = (vec3_t){self->position.x, self->position.y - eye_height - 4096 + step_height, self->position.z},
            .height = eye_height + 4096 - step_height,
            .radius = player_radius,
            .radius_squared = player_radius_squared,
            .is_wall_check = 1,
        };
        bvh_intersect_vertical_cylinder(level_bvh, cyl, &hit);
        const size_t n_active_aabb = entity_get_n_active_aabb();
        for (size_t i = 0; i < n_active_aabb; ++i) {
            const entity_collision_box_t* const box = entity_get_aabb_queue_entry(i);
            rayhit_t curr_hit;
            curr_hit.distance = INT32_MAX;
            if (!box->is_solid && !box->is_trigger) continue;

            int intersect = vertical_cylinder_aabb_intersect_fancy(&box->aabb, cyl, &curr_hit);
            if (intersect && box->is_trigger) {
                entity_send_player_intersect(box->entity_index, self);
            }
            if (!box->is_solid) continue;
            if (!intersect) continue;
            if (curr_hit.distance < hit.distance) memcpy(&hit, &curr_hit, sizeof(rayhit_t));
        }
#else
        (void)level_bvh;
        hit.distance = INT32_MAX;
#endif

        // Did we hit anything?
        if (!is_infinity(hit.distance) && hit.distance > 0) {
            // Eject player out of geometry
            const vec3_t amount_to_eject = (vec3_t){
                scalar_mul(hit.normal.x, player_radius - hit.distance),
                scalar_mul(hit.normal.y, hit.distance),
                scalar_mul(hit.normal.z, player_radius - hit.distance),
            };
            self->position = vec3_add(self->position, amount_to_eject);

            // Absorb penetration force (lol)
            const scalar_t velocity_length = scalar_sqrt(vec3_magnitude_squared(self->velocity));
            const vec3_t velocity_normalized = vec3_divs(self->velocity, velocity_length);
            const vec3_t undesired_motion = vec3_muls(hit.normal, vec3_dot(velocity_normalized, hit.normal));
            const vec3_t desired_motion = vec3_sub(velocity_normalized, undesired_motion);
            self->velocity = vec3_muls(desired_motion, velocity_length);
        }
    }
    self->position.y += self->velocity.y * dt_ms;
}

void player_update(player_t* self, level_collision_t* level_bvh, const int dt_ms, const int time_counter) {
    if (!self) return;
    if (!level_bvh) return;

    const vec3_t player_right = (vec3_t) {
        -hicos(self->rotation.y),
        0,
        +hisin(self->rotation.y),
    };
    audio_update_listener(self->position, player_right);

    if (player_radius_squared == 0) {
        const scalar_t player_radius_scalar = player_radius;
        player_radius_squared = scalar_mul(player_radius_scalar, player_radius_scalar);
    }
    self->is_grounded = 0;
#ifndef _DEBUG_CAMERA
    apply_gravity(self, dt_ms);
    check_ground_collision(self, level_bvh, dt_ms);
#endif
    handle_stick_input(self, dt_ms);
    handle_drag(self, dt_ms);
#ifndef _DEBUG_CAMERA
    handle_jump(self);
#endif
    handle_movement(self, level_bvh, dt_ms);
    
    self->transform.position.x = -self->position.x * (4096 / COL_SCALE);
#ifdef _DEBUG_CAMERA
    (void)time_counter;
    self->transform.position.y = -self->position.y * (4096 / COL_SCALE);
#else
    const vec2_t vel_2d = {self->velocity.x, self->velocity.z};
    const scalar_t speed_1d = vec2_magnitude(vel_2d);
    self->transform.position.y = -self->position.y * (4096 / COL_SCALE) + isin(time_counter * 12) * speed_1d / 64;
    self->footstep_timer += dt_ms;
    if (self->footstep_timer >= FOOTSTEP_TIMER_MAX) {
        self->footstep_timer -= FOOTSTEP_TIMER_MAX;
        if (self->is_grounded && (speed_1d > ONE / 16)) {
            audio_play_sound(random_range(sfx_footstep1, sfx_footstep7 + 1), ONE, 0, (vec3_t){}, 1);
        }
    }
#endif
    self->transform.position.z = -self->position.z * (4096 / COL_SCALE);
    self->transform.rotation.x = -self->rotation.x;
    self->transform.rotation.y = -self->rotation.y;
    self->transform.rotation.z = -self->rotation.z;
}
