#ifndef BENCHMARKER_H
#define BENCHMARKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "subnivis/entity.h"

#define PATH_CAMERA_MAX_NODES 256

typedef struct {
    scalar_t time;
    scalar_t delta;
    int fps;
} benchmark_sample_t;

typedef struct {
	entity_header_t entity_header;
	benchmark_sample_t* samples;
	size_t max_samples;
	size_t sample_cursor;
	scalar_t second_counter;
	scalar_t time_counter;
	int fps_counter;
	int fps;
} entity_benchmarker_t;

entity_header_t* entity_benchmarker_new(vec3_t spawn_pos);
void entity_benchmarker_destroy(void);
size_t entity_benchmarker_serialize(int slot, uint8_t* data);
size_t entity_benchmarker_deserialize(int slot, const uint8_t* data);
void entity_benchmarker_update(int slot, player_t* player, scalar_t dt, level_t* level);
void entity_benchmarker_on_hit(int slot, int hitbox_index);
void entity_benchmarker_player_intersect(int slot, player_t* player);

#ifdef _LEVEL_EDITOR
int entity_benchmarker_inspect(int slot);
#endif

#ifdef __cplusplus
}
#endif

#endif
