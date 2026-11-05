#ifndef CINECAM_H
#define CINECAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "subnivis/entity.h"

#define PATH_CAMERA_MAX_NODES 256

typedef struct {
    vec3_t position;
    vec3_t rotation;
    scalar_t time_to_next;
} cinecam_node_t;

typedef struct {
	entity_header_t entity_header;
	int32_t n_nodes;
	cinecam_node_t* nodes;
	scalar_t progress;
	int active;
	int signal_id;
} entity_cinecam_t;

entity_header_t* entity_cinecam_new(vec3_t spawn_pos);
size_t entity_cinecam_serialize(int slot, uint8_t* data);
size_t entity_cinecam_deserialize(int slot, const uint8_t* data);
void entity_cinecam_update(int slot, player_t* player, scalar_t dt, level_t* level);
void entity_cinecam_on_hit(int slot, int hitbox_index);
void entity_cinecam_player_intersect(int slot, player_t* player);

#ifdef _LEVEL_EDITOR
int entity_cinecam_inspect(int slot);
#endif

#ifdef __cplusplus
}
#endif

#endif
