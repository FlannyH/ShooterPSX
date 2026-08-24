#include "particles.h"

particle_system_t* particle_system_new(particle_system_params_t* params) {
    particle_system_t* particle_system = mem_stack_alloc(sizeof(particle_system_t), STACK_ENTITY);
    params->n_particles_max = ((params->n_particles_max + 2) / 3) * 3; // align to grid of 3, allows for neat gte_rtpt in rendering
    particle_system->params = params;
    particle_system->particle_buffer = mem_stack_alloc(sizeof(particle_t) * params->n_particles_max, STACK_ENTITY);

    // todo
    return NULL;
}

void particle_system_update(particle_system_t* system, transform_t* transform) {
    const particle_system_params_t* params = system->params;

    if (system->time_since_first_particle_seconds > params->system_lifetime) return;

    // Handle particle spawning
    while(system->curr_spawn_timer > 0) {
        particle_t* p = &system->particle_buffer[system->curr_spawn_index];

        if (p->time_alive >= p->total_lifetime) {
            system->particle_buffer[system->curr_spawn_index] = (particle_t) {
                .start_colour = {
                    .r = random_range(params->colour_start_min.r, params->colour_start_max.r),
                    .g = random_range(params->colour_start_min.g, params->colour_start_max.g),
                    .b = random_range(params->colour_start_min.b, params->colour_start_max.b),
                    .a = 255,
                },
                .end_colour = {
                    .r = random_range(params->colour_end_min.r, params->colour_end_max.r),
                    .g = random_range(params->colour_end_min.g, params->colour_end_max.g),
                    .b = random_range(params->colour_end_min.b, params->colour_end_max.b),
                    .a = 255,
                },
                .velocity = {
                    .x = random_range(params->initial_velocity_min.x, params->initial_velocity_max.x),
                    .y = random_range(params->initial_velocity_min.y, params->initial_velocity_max.y),
                    .z = random_range(params->initial_velocity_min.z, params->initial_velocity_max.z),
                },
                .position = {
                    .x = random_range(params->initial_position_offset_min.x, params->initial_position_offset_max.x),
                    .y = random_range(params->initial_position_offset_min.y, params->initial_position_offset_max.y),
                    .z = random_range(params->initial_position_offset_min.z, params->initial_position_offset_max.z),
                },
                .scale = {
                    .x = random_range(params->initial_scale_min.x, params->initial_scale_max.x),
                    .y = random_range(params->initial_scale_min.y, params->initial_scale_max.y),
                },
                .total_lifetime = random_range(params->lifetime_min, params->lifetime_max),
                .curr_frame = 0,
                .time_alive = 0,
            };

            if (params->scale_uniformly) {
                system->particle_buffer[system->curr_spawn_index].scale.y = system->particle_buffer[system->curr_spawn_index].scale.x;
            }

            // Move to entity transform
            system->particle_buffer[system->curr_spawn_index].position.x += transform->position.x;
            system->particle_buffer[system->curr_spawn_index].position.y += transform->position.y;
            system->particle_buffer[system->curr_spawn_index].position.z += transform->position.z;

            // Update spawn rate
            system->curr_spawn_rate = scalar_lerp(params->spawn_rate_start, params->spawn_rate_end, scalar_div(system->time_since_first_particle_seconds, params->system_lifetime));
            system->curr_spawn_timer -= scalar_div(ONE, system->curr_spawn_rate);
        }

        // Move buffer cursor
        system->curr_spawn_index++;
        system->curr_spawn_index %= params->n_particles_max;
    }
}
