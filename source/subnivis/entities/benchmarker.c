#include "benchmarker.h"
#include "engine/renderer.h"

#ifdef _LEVEL_EDITOR
#include "editor/editor.h"

int entity_benchmarker_inspect(int slot) {
    (void)slot;
    return 0;
}
#endif

entity_header_t* entity_benchmarker_new(vec3_t spawn_pos) {
	entity_benchmarker_t* entity = (entity_benchmarker_t*)entity_get_header(entity_alloc(ENTITY_BENCHMARKER));
	entity->entity_header.position = spawn_pos;
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = NULL;
	entity->samples = NULL;
	entity->max_samples = 0;
	entity->sample_cursor = 0;
	entity->second_counter = 0;
	entity->time_counter = 0;
	entity->fps_counter = 0;
	entity->fps = 0;
	return (entity_header_t*)entity;
}

void entity_benchmarker_destroy(void) {
    return;
}

size_t entity_benchmarker_serialize(int slot, uint8_t* data) {
    // nothing to serialize
    (void)slot;
    (void)data;
    return 3 * sizeof(vec3_t);
}

size_t entity_benchmarker_deserialize(int slot, const uint8_t* data) {
    // nothing to deserialize
    (void)slot;
    (void)data;

    entity_benchmarker_t* benchmarker = (entity_benchmarker_t*)entity_get_header(slot);
    benchmarker->max_samples = 512;
    benchmarker->samples = mem_stack_alloc(benchmarker->max_samples * sizeof(benchmark_sample_t), STACK_ENTITY);
    benchmarker->sample_cursor = 0;
    benchmarker->fps = 0;
    benchmarker->fps_counter = 0;
    benchmarker->time_counter = 0;
    benchmarker->second_counter = 0;
    return 3 * sizeof(vec3_t);
}

void entity_benchmarker_update(int slot, player_t* player, scalar_t dt, level_t* level) {
    (void)player;
    (void)level;
    entity_benchmarker_t* benchmarker = (entity_benchmarker_t*)entity_get_header(slot);

    benchmarker->fps_counter++;
    benchmarker->time_counter += dt;
    benchmarker->second_counter += dt;
    if (benchmarker->second_counter > ONE) {
        benchmarker->second_counter -= ONE;
        benchmarker->fps = benchmarker->fps_counter;
        benchmarker->fps_counter = 0;
    }

    char text[32] = {0};
    int int_part = (dt * 1000) / ONE;
    int frac_part = ((dt * 1000 * 10) / (ONE / 10)) % 10;

    snprintf(text, sizeof(text), "%i fps\n%i.%01i ms", benchmarker->fps, int_part, frac_part);
    renderer_draw_text((vec2_t){SCALAR(14), SCALAR(62)}, text, 1, 0, white);
    renderer_draw_text((vec2_t){SCALAR(16), SCALAR(64)}, text, 1, 0, black);

    if (benchmarker->max_samples == 0) return;

    benchmarker->samples[benchmarker->sample_cursor++] = (benchmark_sample_t) { benchmarker->time_counter, dt, benchmarker->fps };
    benchmarker->sample_cursor %= benchmarker->max_samples;

    // graph
    const scalar_t delta_mul = -1200;
    const scalar_t y_offset = 0;
    renderer_draw_2d_line((vec2_t){SCALAR(-256), SCALAR(y_offset)}, (vec2_t){SCALAR(256), SCALAR(y_offset)}, black, 3);
    renderer_draw_2d_line((vec2_t){SCALAR(-256), SCALAR(y_offset) - delta_mul * SCALAR(0.05)}, (vec2_t){SCALAR(256), SCALAR(y_offset) - delta_mul * SCALAR(0.05)}, red, 3);
    renderer_draw_2d_line((vec2_t){SCALAR(-256), SCALAR(y_offset) - delta_mul * SCALAR(0.033)}, (vec2_t){SCALAR(256), SCALAR(y_offset) - delta_mul * SCALAR(0.033)}, yellow, 3);
    renderer_draw_2d_line((vec2_t){SCALAR(-256), SCALAR(y_offset) - delta_mul * SCALAR(0.016)}, (vec2_t){SCALAR(256), SCALAR(y_offset) - delta_mul * SCALAR(0.016)}, green, 3);
    for (size_t i = 0; i < benchmarker->max_samples; ++i) {
        size_t offset1 = (benchmarker->sample_cursor + i - 1) % benchmarker->max_samples;
        size_t offset2 = (benchmarker->sample_cursor + i) % benchmarker->max_samples;
        renderer_draw_2d_line(
            (vec2_t){SCALAR(i - 1 - 256), SCALAR(y_offset) - (delta_mul * benchmarker->samples[offset1].delta)},
            (vec2_t){SCALAR(i - 256), SCALAR(y_offset) - (delta_mul * benchmarker->samples[offset2].delta)},
            white, 1
        );
        renderer_draw_2d_line(
            (vec2_t){SCALAR(i - 1 - 256 + 1), SCALAR(y_offset + 1) - (delta_mul * benchmarker->samples[offset1].delta)},
            (vec2_t){SCALAR(i - 256 + 1), SCALAR(y_offset + 1) - (delta_mul * benchmarker->samples[offset2].delta)},
            black, 2
        );
    }
}

void entity_benchmarker_on_hit(int slot, int hitbox_index) {
    (void)slot;
    (void)hitbox_index;
}

void entity_benchmarker_player_intersect(int slot, player_t* player) {
    (void)slot;
    (void)player;
}
