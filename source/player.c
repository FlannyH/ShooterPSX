#include "player.h"
#include "collision.h"
#include "common.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#include "common.h"
#include "input.h"
#include "entity.h"
#include "common.h"
#include "music.h"
#include "random.h"

#include <string.h>

#define FOOTSTEP_TIMER_MAX 350

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

void collide(player_t* self, level_t* level) {
    // player - level collision
    shape_t player = {0};
    player.type = SHAPE_CAPSULE;
    player.capsule.a = self->position;
    player.capsule.a.y -= player_radius;
    player.capsule.b = self->position;
    player.capsule.b.y += player_height + player_radius;
    player.capsule.radius = player_radius;

    convex_hull_mesh_t polytope = {0};

    for (size_t i = 0; (i < level->n_shapes) && level->shapes; ++i) {
        if (level->shapes[i].type == SHAPE_NONE) continue;
        if (gjk(&polytope, &level->shapes[i], &player)) {
            printf("colliding with shape %i\n", i);
            const vec3_t penetration = epa(&polytope, &level->shapes[i], &player);
            // const vec3_t old_pos = player.capsule.a;
            move_shape(&player, penetration);
        }
    }

    // todo: entity collision

    self->position = player.capsule.a;
    self->position.y += player_radius;
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
    const scalar_t curr_acceleration = (self->is_grounded) ? (walking_acceleration * dt_ms) : ((walking_acceleration * dt_ms) / air_acceleration_divider);

    // todo: merge parts of this code so we only do one set of additions to the player pos rot and vel

    if (input_mouse_connected()) {
        // Moving forwards and backwards
        self->velocity.x += trig_sin(self->rotation.y) * input_left_stick_y(0) * (curr_acceleration) >> 16;
        self->velocity.z += trig_cos(self->rotation.y) * input_left_stick_y(0) * (curr_acceleration) >> 16;

        // Strafing left and right
        self->velocity.x += trig_cos(self->rotation.y) * input_left_stick_x(0) * (curr_acceleration) >> 16;
        self->velocity.z -= trig_sin(self->rotation.y) * input_left_stick_x(0) * (curr_acceleration) >> 16;

        // Look up and down
        self->rotation.x -= (int32_t)(input_mouse_movement_y()) * (mouse_sensitivity) >> 12;
        if (self->rotation.x > 32768) {
            self->rotation.x = 32768;
        }
        if (self->rotation.x < -32768) {
            self->rotation.x = -32768;
        }

        // Look left and right
        self->rotation.y -= (int32_t)(input_mouse_movement_x()) * (mouse_sensitivity) >> 12;
    }

    if (input_has_analog(0)) {
        // Moving forwards and backwards
        self->velocity.x += trig_sin(self->rotation.y) * input_left_stick_y(0) * (curr_acceleration) >> 16;
        self->velocity.z += trig_cos(self->rotation.y) * input_left_stick_y(0) * (curr_acceleration) >> 16;

        // Strafing left and right
        self->velocity.x += trig_cos(self->rotation.y) * input_left_stick_x(0) * (curr_acceleration) >> 16;
        self->velocity.z -= trig_sin(self->rotation.y) * input_left_stick_x(0) * (curr_acceleration) >> 16;

        // Look up and down
        self->rotation.x -= (int32_t)(input_right_stick_y(0)) * (stick_sensitivity * dt_ms) >> 12;
        if (self->rotation.x > SCALAR(0.22)) {
            self->rotation.x = SCALAR(0.22);
        }
        if (self->rotation.x < -SCALAR(0.22)) {
            self->rotation.x = -SCALAR(0.22);
        }

        // Look left and right
        self->rotation.y += (int32_t)(input_right_stick_x(0)) * (stick_sensitivity * dt_ms) >> 12;

#ifdef _DEBUG
        if (input_held(PAD_UP, 0)) {
            self->position.y += ONE * dt_ms;
            self->velocity.y = 0;
        }
        if (input_held(PAD_DOWN, 0)) {
            self->position.y -= ONE * dt_ms;
            self->velocity.y = 0;
        }
#endif
    } else {
        // Look left and right
        const int32_t dpad_x = ((int32_t)(input_held(PAD_RIGHT, 0) != 0) * 127) + ((int32_t)(input_held(PAD_LEFT, 0) != 0) * -127);
        self->rotation.y += dpad_x * (stick_sensitivity * dt_ms) >> 12;

        // Moving forwards and backwards
        const int32_t dpad_y = ((int32_t)(input_held(PAD_UP, 0) != 0) * 127) + ((int32_t)(input_held(PAD_DOWN, 0) != 0) * -127);
        self->velocity.x += trig_sin(self->rotation.y) * dpad_y * (curr_acceleration) >> 16;
        self->velocity.z += trig_cos(self->rotation.y) * dpad_y * (curr_acceleration) >> 16;

        // Strafing left and right
        const int32_t shoulder_x = ((int32_t)(input_held(PAD_L1, 0) != 0) * 127) + ((int32_t)(input_held(PAD_R1, 0) != 0) * -127);
        self->velocity.x -= trig_cos(self->rotation.y) * shoulder_x * (curr_acceleration) >> 16;
        self->velocity.z += trig_sin(self->rotation.y) * shoulder_x * (curr_acceleration) >> 16;
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
        audio_play_sound(sfx_jump_land1, 0, 0, (vec3_t){}, 1);
    }
    if (!was_grounded && self->is_grounded) audio_play_sound(sfx_jump_land2, 0, 0, (vec3_t){}, 1);
    was_grounded = self->is_grounded;
}

void handle_movement(player_t* self, level_t* level, const int dt_ms) {
    (void)level;
    // Move the player, ask questions later
    self->position.x += self->velocity.x * dt_ms / PLAYER_VELOCITY_PRECISION;
    self->position.z += self->velocity.z * dt_ms / PLAYER_VELOCITY_PRECISION;

    for (size_t i = 0; i < 2; ++i) {
        rayhit_t hit = {};
#ifndef _DEBUG_CAMERA
        // Collide
        const vertical_cylinder_t cyl = {
            .bottom = (vec3_t){self->position.x, self->position.y - eye_height - ONE + step_height, self->position.z},
            .height = eye_height + ONE - step_height,
            .radius = player_radius,
            .is_wall_check = 1,
        };
        // bvh_intersect_vertical_cylinder(level_bvh, cyl, &hit);

        const size_t n_active_aabb = entity_get_n_active_aabb();
        for (size_t i = 0; i < n_active_aabb; ++i) {
            const entity_collision_box_t* const box = entity_get_aabb_queue_entry(i);
            rayhit_t curr_hit;
            curr_hit.distance = INT32_MAX;
            if (!box->is_solid && !box->is_trigger) continue;

            // int intersect = vertical_cylinder_aabb_intersect_fancy(&box->aabb, cyl, &curr_hit);
            // if (intersect && box->is_trigger) {
            //     entity_send_player_intersect(box->entity_index, self);
            // }
            // if (!box->is_solid) continue;
            // if (!intersect) continue;
            // if (curr_hit.distance < hit.distance) memcpy(&hit, &curr_hit, sizeof(rayhit_t));
        }
#else
        (void)level;
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

void player_update(player_t* self, level_t* level, const int dt_ms, const int time_counter) {
    if (!self) return;
    if (!level) return;

    const vec3_t player_right = (vec3_t) {
        -trig_cos(self->rotation.y),
        0,
        +trig_sin(self->rotation.y),
    };
    audio_update_listener(self->position, player_right);

    self->is_grounded = 0;
#ifndef _DEBUG_CAMERA
    apply_gravity(self, dt_ms);
#endif
    // check_ground_collision(self, level_bvh, dt_ms);
    collide(self, level);
    handle_stick_input(self, dt_ms);
    handle_drag(self, dt_ms);
#ifndef _DEBUG_CAMERA
    handle_jump(self);
#endif
    handle_movement(self, level, dt_ms);

    const vec2_t vel_2d = {self->velocity.x, self->velocity.z};
    const scalar_t speed_1d = vec2_magnitude(vel_2d) / PLAYER_VELOCITY_PRECISION;

#ifdef _DEBUG_CAMERA
    (void)time_counter;
    self->position.y += 1000 * ((dt_ms * (input_held(PAD_UP, 0) != 0)) - (dt_ms * (input_held(PAD_DOWN, 0) != 0)));
    self->transform.position.y = self->position.y;

#else
    self->transform.position.y = self->position.y + trig_sin(time_counter * 12) * speed_1d / 64;

    self->footstep_timer += dt_ms;
    if (self->footstep_timer >= FOOTSTEP_TIMER_MAX) {
        self->footstep_timer -= FOOTSTEP_TIMER_MAX;
        if (self->is_grounded && (speed_1d > ONE / 16)) {
            audio_play_sound(random_range(sfx_footstep1, sfx_footstep7 + 1), 0, 0, (vec3_t){}, 1);
        }
    }
#endif

    self->transform.position.x = self->position.x;
    self->transform.position.y = self->position.y + trig_sin(time_counter * 12) * speed_1d / 64;
    self->transform.position.z = self->position.z;

    self->transform.rotation.x = self->rotation.x;
    self->transform.rotation.y = self->rotation.y;
    self->transform.rotation.z = self->rotation.z;
}
