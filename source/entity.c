#include "entity.h"
#include "mesh.h"
#include "entities/door.h"
#include "entities/pickup.h"
#include "entities/crate.h"
#include <string.h>

entity_slot_t entity_list[ENTITY_LIST_LENGTH];
entity_collision_box_t entity_aabb_queue[ENTITY_LIST_LENGTH];
uint8_t* entity_pool = NULL;
size_t entity_pool_stride = 0;
size_t entity_n_active_aabb;
model_t* entity_models = NULL;

void entity_update_all(player_t* player, int dt) {
	// Reset counters
	entity_n_active_aabb = 0;

	// Loop over all entities
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		switch (entity_list[i].type) {
			case ENTITY_NONE: break;
			case ENTITY_DOOR: entity_door_update(i, player, dt); break;
			case ENTITY_PICKUP: entity_pickup_update(i, player, dt); break;
			case ENTITY_CRATE: entity_crate_update(i, player, dt); break;
		}
	}
}

int entity_alloc(uint8_t entity_type) {
	// Find an empty slot
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		if (entity_list[i].type == ENTITY_NONE) {
			// Register the entity
			entity_list[i].type = entity_type;
			return i;
		}
	}
	return -1;
}

typedef struct {
	union {
		entity_door_t door;
		entity_pickup_t pickup;
		entity_crate_t crate;
	};
} entity_union;

void entity_init() {
	// Zero initialize the entity list
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) entity_list[i].type = ENTITY_NONE;

	entity_n_active_aabb = 0;

	// Allocate entity pool - make sure to update this when adding new entities
	entity_pool_stride = sizeof(entity_union);
	entity_pool = mem_stack_alloc(ENTITY_LIST_LENGTH * sizeof(entity_union), STACK_ENTITY);

	// Load model collection
    entity_models = model_load("\\ASSETS\\MODELS\\ENTITY.MSH;1", 1, STACK_ENTITY);
}

void entity_register_collision_box(const entity_collision_box_t* box) {
	memcpy(&entity_aabb_queue[entity_n_active_aabb++], box, sizeof(entity_collision_box_t));
}

void entity_kill(int slot) {
	// todo: maybe support destructors?
	entity_list[slot].type = ENTITY_NONE;
}

const char* entity_names[] = {
	"ENTITY_NONE",
	"ENTITY_DOOR",
	"ENTITY_PICKUP",
	"ENTITY_CRATE",
	NULL
};

#ifdef _DEBUG
void entity_debug() {
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		printf("entity %03i:\t%s\n", i, entity_names[entity_list[i].type]);
	}
}

int entity_how_many_active() {
	int count = 0;
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		if (entity_list[i].type != ENTITY_NONE) {
			count++;
		}
	}
	return count;
}
#endif