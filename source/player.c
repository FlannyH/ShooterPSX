#include "player.h"
#include "collision.h"
#include "math/vec2.h"
#include "math/vec3.h"

#ifdef _PSX
#include <psxpad.h>
#endif

#ifdef _PC
#include "pc/psx.h"
#endif

#ifdef _NDS
#include "nds/psx.h"
#endif

#include "subnivis/input_map.h"
#include "input_mapping.h"
#include "music.h"
#include "random.h"

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

    for (int i = 0; (i < level->n_shapes) && level->shapes; ++i) {
        if (level->shapes[i].type == SHAPE_NONE) continue;
        if (gjk(&polytope, &level->shapes[i], &player)) {
            printf("colliding with shape %i\n", i);
            const vec3_t penetration = epa(&polytope, &level->shapes[i], &player);
            // const vec3_t old_pos = player.capsule.a;
            move_shape(&player, penetration);

            if (penetration.y > 0) {
                self->is_grounded = 1;
            }

            // get part of the vector along the move axis
            const vec3_t nrm_pen = vec3_normalize(penetration);
            const scalar_t length_along_nrm_pen = vec3_dot(self->velocity, nrm_pen);
            self->velocity = vec3_sub(self->velocity, vec3_muls(nrm_pen, length_along_nrm_pen));
        }
    }

    // todo: entity collision

    self->position = player.capsule.a;
    self->position.y += player_radius;
}

void apply_gravity(player_t* self, const scalar_t dt) {
    if (self->is_grounded) return;

    self->velocity.y += scalar_mul(gravity, dt) * PLAYER_VELOCITY_PRECISION;
    if (self->velocity.y > terminal_velocity_up * PLAYER_VELOCITY_PRECISION) {
        self->velocity.y = terminal_velocity_up * PLAYER_VELOCITY_PRECISION;
    }
    else if (self->velocity.y < terminal_velocity_down * PLAYER_VELOCITY_PRECISION) {
        self->velocity.y = terminal_velocity_down * PLAYER_VELOCITY_PRECISION;
    }
}

void handle_stick_input(player_t* self, const scalar_t dt) {
    scalar_t curr_acceleration = scalar_mul(walking_acceleration, dt);
    if (!self->is_grounded) {
        curr_acceleration = scalar_div(curr_acceleration, air_acceleration_divider);
    }

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
        input_mapping_value(IM_LOOK_MOUSE_X, 0),
        input_mapping_value(IM_LOOK_MOUSE_Y, 0)
    };

    // Moving horizontally
    self->velocity = vec3_add(self->velocity, vec3_muls(forward, scalar_mul(move.y, walking_acceleration * PLAYER_VELOCITY_PRECISION)));
    self->velocity = vec3_add(self->velocity, vec3_muls(right, scalar_mul(move.x, walking_acceleration * PLAYER_VELOCITY_PRECISION)));

    // Looking up and down
    self->rotation.x += look.y;
    self->rotation.x = scalar_clamp(self->rotation.x, SCALAR(-0.22 * PLAYER_ROTATION_PRECISION), SCALAR(0.22 * PLAYER_ROTATION_PRECISION));

    // Looking left and right
    self->rotation.y += look.x;
}

void player_handle_drag(player_t* self, const scalar_t dt) {
    scalar_t curr_drag = scalar_mul(drag, dt);
    if (self->is_grounded) {
        curr_drag = scalar_div(drag, jump_drag_divider);
    }

    const scalar_t length = vec3_magnitude(self->velocity);
    const vec3_t dir = vec3_divs(self->velocity, length);
    if (length > (walking_max_speed * PLAYER_VELOCITY_PRECISION)) {
        const scalar_t preserve_y = self->velocity.y;
        self->velocity = vec3_muls(dir, length);
        self->velocity.y = preserve_y;
    }
    else if (length > (curr_drag * PLAYER_VELOCITY_PRECISION)) {
        const scalar_t preserve_y = self->velocity.y;
        self->velocity = vec3_sub(self->velocity, vec3_muls(dir, curr_drag * PLAYER_VELOCITY_PRECISION));
        self->velocity.y = preserve_y;
    }
    else {
        self->velocity.x = 0;
        self->velocity.z = 0;
    }
}

int was_grounded = 0;

void handle_jump(player_t* self) {
    if (self->is_grounded && input_mapping_pressed(IM_JUMP, 0)) {
        self->velocity.y = initial_jump_velocity;
        audio_play_sound(sfx_jump_land1, 0, 0, (vec3_t){}, 1);
    }
    if (!was_grounded && self->is_grounded) audio_play_sound(sfx_jump_land2, 0, 0, (vec3_t){}, 1);
    was_grounded = self->is_grounded;
}

void update_transform(player_t* self, scalar_t dt, scalar_t time_counter) {
    const vec2_t vel_2d = {self->velocity.x, self->velocity.z};
    const scalar_t speed_1d = vec2_magnitude(vel_2d);

    #ifdef _DEBUG_CAMERA
    (void)time_counter;
    // Moving up and down
    if (input_mapping_held(IM_DEBUG_DOWN, 0)) self->position.y -= scalar_mul(SCALAR(100), dt);
    if (input_mapping_held(IM_DEBUG_UP, 0))  self->position.y += scalar_mul(SCALAR(100), dt);
    self->transform.position.y = self->position.y;

    #else
    self->transform.position.y = self->position.y + scalar_mul(trig_sin(time_counter * 4), speed_1d / 64);

    self->footstep_timer += dt;
    if (self->footstep_timer >= FOOTSTEP_TIMER_MAX) {
        self->footstep_timer -= FOOTSTEP_TIMER_MAX;
        if (self->is_grounded && (speed_1d > ONE / 16)) {
            audio_play_sound(random_range(sfx_footstep1, sfx_footstep7 + 1), 0, 0, (vec3_t){}, 1);
        }
    }
    #endif

    self->transform.position.x = self->position.x;
    self->transform.position.z = self->position.z;

    self->transform.rotation.x = self->rotation.x / PLAYER_ROTATION_PRECISION;
    self->transform.rotation.y = self->rotation.y / PLAYER_ROTATION_PRECISION;
    self->transform.rotation.z = self->rotation.z / PLAYER_ROTATION_PRECISION;
}

void player_update(player_t* self, level_t* level, const scalar_t dt, const scalar_t time_counter) {
    if (!self) return;
    if (!level) return;

    printf("dt: "); scalar_debug(dt);

    const vec3_t player_right = (vec3_t) {-trig_cos(self->rotation.y),0,+trig_sin(self->rotation.y)};
    audio_update_listener(self->position, player_right);

    self->is_grounded = 0;
    apply_gravity(self, dt);
    handle_stick_input(self, dt);
    player_handle_drag(self, dt);
    handle_jump(self);

    self->position = vec3_add(self->position, vec3_muls(vec3_divs(self->velocity, SCALAR(PLAYER_VELOCITY_PRECISION)), dt));

    collide(self, level);
    update_transform(self, dt, time_counter);
}
