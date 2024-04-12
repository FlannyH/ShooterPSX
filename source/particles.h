#ifndef PARTICLES_H
#define PARTICLES_H

#include "texture.h"
#include "vec2.h"
#include "vec3.h"
#include "structs.h"
#include "random.h"

enum BlendMode
{
	BLEND_MODE_MIX,
	BLEND_MODE_ADD,
	BLEND_MODE_SUB
};

//This struct defines a particle system. A pointer to an instance of this struct will be used in a particle system entity to determine its behaviour and look.
typedef struct {
	uint8_t texture_id;						// ResourceHandle to the texture used by the particles in this particle system
	uint8_t blend_mode;						// Blend mode specifies how the particles are mixed into the scene
	uint8_t scale_uniformly;                   // If set, the particle system will scale all axes the same way
	int n_animation_frames;					// number of animation frames
	int loop_start;							// -1 = no loop, otherwise it will loop from loop_start to animation_frames-1
	scalar_t animation_frame_rate;			// (frames / second)
	scalar_t lifetime_min;					// Minimum lifetime of an individual particle in seconds
	scalar_t lifetime_max;					// Maximum lifetime of an individual particle in seconds
	scalar_t system_lifetime;				// The time that this particle system is alive and spawning particles (seconds)
	scalar_t spawn_rate_start;				// Particles to spawn per second at the start
	scalar_t spawn_rate_end;				// Particles to spawn per second at the end
	int n_particles_max;					// Maximum number of particles to spawn in this particle system. Do not change this value during runtime!
	vec3_t constant_acceleration;			// Constant acceleration (world units / second^2). Useful for effects like gravity or wind
	vec3_t initial_velocity_min;			// Minimum initial velocity (collision space units / second)
	vec3_t initial_velocity_max;			// Maximum initial velocity (collision space units / second)
	vec3_t initial_position_offset_min;		// Minimum initial position offset (collision space units)
	vec3_t initial_position_offset_max;		// Maximum initial position offset (collision space units)
	vec2_t initial_scale_min;				// Minimum initial scale (graphics space units)
	vec2_t initial_scale_max;				// Maximum initial scale (graphics space units)
	pixel32_t colour_start_min;				// Minimum starting colour value (RGBA 0 - 255)
	pixel32_t colour_start_max;				// Maximum starting colour value (RGBA 0 - 255)
	pixel32_t colour_end_min;				// Ending colour value (RGBA 0 - 255). If the red value is set to a value below zero, the particles will stay the same colour.
	pixel32_t colour_end_max;				// Ending colour value (RGBA 0 - 255). If the red value is set to a value below zero, the particles will stay the same colour.
	vec2_t scale_multiplier_over_time;		// Value to multiply the scale by every second - TODO: assess whether this causes any fixed point precision problems
	vec3_t velocity_multiplier_over_time;	// Value to multiply the velocity by every second. Can be thought of like a friction value per axis
} particle_system_params_t;

typedef struct {
	scalar_t total_lifetime;	// Time before despawning (seconds)
	scalar_t time_alive;		// Time currently alive. If this exceeds `total_lifetime`, the particle gets destroyed
	pixel32_t start_colour;	    // Starting colour (RGBA 0 - 255)
	pixel32_t end_colour;	    // Ending colour (RGBA 0 - 255)
	vec3_t velocity;		    // Current velocity (world units / second)
	vec3_t position;		    // Position in world space (world units)
	vec2_t scale;		        // Scale in world space (world units)
	scalar_t curr_frame;		// Current frame ID
} particle_t;

typedef struct {
    particle_system_params_t* params;
    particle_t* particle_buffer;
    scalar_t curr_spawn_rate;
    scalar_t curr_spawn_timer;
    scalar_t time_since_first_particle_seconds;
    int curr_spawn_index;
} particle_system_t;

particle_system_t* particle_system_new(particle_system_params_t* params) {
    particle_system_t* particle_system = mem_stack_alloc(sizeof(particle_system_t), STACK_ENTITY);
    params->n_particles_max = ((params->n_particles_max + 2) / 3) * 3; // align to grid of 3, allows for neat gte_rtpt in rendering
    particle_system->params = params;
    particle_system->particle_buffer = mem_stack_alloc(sizeof(particle_t) * params->n_particles_max, STACK_ENTITY);

    // todo
    return NULL;
}

void particle_system_update(particle_system_t* system, transform_t* transform) {
    particle_system_params_t* params = system->params;

    if (system->time_since_first_particle_seconds > params->system_lifetime) return;

    // Handle particle spawning
    while(system->curr_spawn_timer > 0) {
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
        system->curr_spawn_timer = scalar_div(ONE, system->curr_spawn_rate);

        // Move buffer cursor
        system->curr_spawn_index++;
        system->curr_spawn_index %= params->n_particles_max;
    }
}

#endif