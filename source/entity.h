#ifndef ENTITY_H
#define ENTITY_H
#include "structs.h"
#include "player.h"

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENTITY_NOT_SECTION_BOUND 255
#define ENTITY_AABB_QUEUE_LENGTH 256
#define ENTITY_LIST_LENGTH 256
#define ENTITY_SIGNAL_COUNT 64

typedef enum {
	ENTITY_NONE,
	ENTITY_DOOR,
	ENTITY_PICKUP,
	ENTITY_CRATE,
	ENTITY_CHASER,
	ENTITY_PLATFORM,
	ENTITY_TRIGGER,
} entity_type_t; // typically represented by a uint8_t

typedef enum {
	ENTITY_MESH_DOOR_SMALL_LOCKED = 0,
	ENTITY_MESH_DOOR_SMALL_LOCKED_ROTATED,
	ENTITY_MESH_DOOR_SMALL_UNLOCKED,
	ENTITY_MESH_DOOR_SMALL_UNLOCKED_ROTATED,
	ENTITY_MESH_DOOR_BIG_LOCKED,
	ENTITY_MESH_DOOR_BIG_LOCKED_ROTATED,
	ENTITY_MESH_DOOR_BIG_UNLOCKED,
	ENTITY_MESH_DOOR_BIG_UNLOCKED_ROTATED,
	ENTITY_MESH_PICKUP_AMMO_SMALL,
	ENTITY_MESH_PICKUP_AMMO_BIG,
	ENTITY_MESH_PICKUP_ARMOR_SMALL,
	ENTITY_MESH_PICKUP_ARMOR_BIG,
	ENTITY_MESH_PICKUP_HEALTH_SMALL,
	ENTITY_MESH_PICKUP_HEALTH_BIG,
	ENTITY_MESH_PICKUP_KEY_BLUE,
	ENTITY_MESH_PICKUP_KEY_YELLOW,
	ENTITY_MESH_PICKUP_DAMAGE,
	ENTITY_MESH_PICKUP_FIRE_RATE,
	ENTITY_MESH_PICKUP_INVINCIBILITY,
	ENTITY_MESH_CHASER_IDLE,
	ENTITY_MESH_CHASER_AIM,
	ENTITY_MESH_CHASER_HEAD_BROKEN,
	ENTITY_MESH_CHASER_HEAD_NORMAL,
	ENTITY_MESH_CHASER_BODY,
	ENTITY_MESH_CHASER_ARM,
	ENTITY_MESH_CHASER_BOTTOM1,
	ENTITY_MESH_CHASER_BOTTOM2,
	ENTITY_MESH_CHASER_GUN,
	ENTITY_MESH_CRATE,
	ENTITY_MESH_PLATFORM_TEST_HORIZONTAL,
	N_ENTITY_MESH_IDS,
} entity_mesh_id_t;

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

void entity_init(void);
int entity_alloc(uint8_t entity_type);
void entity_set_type(int index, uint8_t type);
void entity_deserialize_and_write_slot(int slot, const entity_header_serialized_t* header);
void entity_register_collision_box(const entity_collision_box_t* box); // (*box) gets copied, can safely be freed after calling this function
void entity_defragment(void);
void entity_sanitize(void);
void entity_update_all(player_t* player, int dt);
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
