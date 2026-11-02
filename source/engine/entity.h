#ifndef ENTITY_H
#define ENTITY_H

#include "subnivis/player.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

#include <stdint.h>
#include <stdio.h>

#define ENTITY_NOT_SECTION_BOUND 255
#define ENTITY_AABB_QUEUE_LENGTH 256
#define ENTITY_LIST_LENGTH 256
#define ENTITY_SIGNAL_COUNT 64

// doc: desc: Entities with type 0 are considered empty slots
#define ENTITY_NONE 0

// All entities must have this data. Add it as the first member of an entity struct.
typedef struct {
	vec3_t position;
	vec3_t rotation;
	vec3_t scale;
	mesh_t* mesh;
} entity_header_t;

typedef struct {
	vec3_t position;
	vec3_t rotation;
	vec3_t scale;
} entity_header_serialized_t;

typedef struct {
	aabb_t aabb;
	uint8_t entity_index; // which entity this box belongs to, so a signal can be sent to the entity when this box is hit
	uint8_t box_index; // used to differentiate between different hitboxes, like body shot and headshot for enemies
	unsigned int is_solid : 1; // can the player move through it or not?
	unsigned int is_trigger : 1; // does it trigger any events? (e.g. text, moving geometry)
	unsigned int not_move_player_along : 1; // if the player is standing on this box, should they be moved along with the entity this collision box belongs to?
} entity_collision_box_t;

typedef entity_header_t* (*entity_new_func_t)(vec3_t position);
typedef size_t (*entity_deserialize_func_t)(int slot, const uint8_t* data);
typedef size_t (*entity_serialize_func_t)(int slot, uint8_t* data);
typedef void (*entity_update_func_t)(int slot, player_t* player, scalar_t dt);
typedef void (*entity_on_hit_func_t)(int slot, int hitbox_index);
typedef void (*entity_player_intersect_func_t)(int slot, player_t* player);
#ifdef _LEVEL_EDITOR
typedef int (*entity_inspect_func_t)(int slot);
#endif

typedef struct {
    uint32_t struct_size;
    entity_new_func_t new_fn;
    entity_deserialize_func_t deserialize;
    entity_serialize_func_t serialize;
    entity_update_func_t update;
    entity_on_hit_func_t on_hit;
    entity_player_intersect_func_t player_intersect;
    #ifdef _LEVEL_EDITOR
    entity_inspect_func_t inspect;
    #endif
} entity_reg_t;

void entity_init(void);
void entity_register_type(uint8_t entity_type, entity_reg_t registration);
const entity_reg_t* entity_register_get(uint8_t entity_type);
int entity_alloc(uint8_t entity_type);
void entity_set_type(int index, uint8_t type);
size_t entity_deserialize_and_write_slot(int slot, const entity_header_serialized_t* header, uint8_t type);
size_t entity_serialize(int slot, uint8_t* dest, uint8_t type);
void entity_register_collision_box(const entity_collision_box_t* box); // (*box) gets copied, can safely be freed after calling this function
void entity_call_on_hit(int entity_index, int box_index);
void entity_call_player_intersect(int entity_index, player_t* player);
void entity_defragment(void);
void entity_sanitize(void);
void entity_update_all(player_t* player, scalar_t dt);
void entity_kill(int slot);
void entity_send_player_intersect(int slot, player_t* player);
uint8_t entity_get_type(int index);
entity_header_t* entity_get_header(int index);
model_t* entity_get_models(void);
size_t entity_get_pool_stride(void);
size_t entity_get_n_active_aabb(void);
entity_collision_box_t* entity_get_aabb_queue_entry(int index);
int entity_get_signal(int index);
void entity_set_signal(int index, int value);

#ifdef _DEBUG
void entity_debug(void);
int entity_how_many_active(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
