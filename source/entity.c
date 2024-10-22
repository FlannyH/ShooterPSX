#include "entity.h"

#include "entities/platform.h"
#include "entities/trigger.h"
#include "entities/pickup.h"
#include "entities/chaser.h"
#include "entities/crate.h"
#include "entities/door.h"
#include "mesh.h"
#include "main.h"

#include <string.h>
extern state_vars_t state;

entity_collision_box_t entity_aabb_queue[ENTITY_LIST_LENGTH];
uint8_t entity_types[ENTITY_LIST_LENGTH];
uint8_t* entity_pool = NULL;
size_t entity_pool_stride = 0;
size_t entity_n_active_aabb;
model_t* entity_models = NULL;
int n_entity_textures = 0;

void entity_update_all(player_t* player, int dt) {
	// Reset counters
	entity_n_active_aabb = 0;

	// Loop over all entities
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		switch (entity_types[i]) {
			case ENTITY_NONE: break;
			case ENTITY_DOOR: entity_door_update(i, player, dt); break;
			case ENTITY_PICKUP: entity_pickup_update(i, player, dt); break;
			case ENTITY_CRATE: entity_crate_update(i, player, dt); break;
			case ENTITY_CHASER: entity_chaser_update(i, player, dt); break;
			case ENTITY_PLATFORM: entity_platform_update(i, player, dt); break;
			case ENTITY_TRIGGER: entity_trigger_update(i, player, dt); break;
		}
	}
}

int entity_alloc(uint8_t entity_type) {
	// Find an empty slot
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		if (entity_types[i] == ENTITY_NONE) {
			// Register the entity
			entity_types[i] = entity_type;
			return i;
		}
	}
	return -1;
}

typedef struct {
	union {
		entity_header_t header;
		entity_door_t door;
		entity_pickup_t pickup;
		entity_crate_t crate;
		entity_chaser_t chaser;
		entity_platform_t platform;
		entity_trigger_t trigger;
	};
} entity_union;

void entity_init(void) {
	// Zero initialize the entity list
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) entity_types[i] = ENTITY_NONE;

	entity_n_active_aabb = 0;

	// Allocate entity pool - make sure to update this when adding new entities
	entity_pool_stride = sizeof(entity_union);
	entity_pool = mem_stack_alloc(ENTITY_LIST_LENGTH * sizeof(entity_union), STACK_ENTITY);

    // Load entity textures
	texture_cpu_t *entity_textures;
    tex_entity_start = tex_alloc_cursor;
	n_entity_textures = texture_collection_load("\\assets\\models\\entity.txc", &entity_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
	    renderer_upload_texture(&entity_textures[i], i + tex_entity_start);
	}
	tex_alloc_cursor += n_entity_textures;
	mem_stack_release(STACK_TEMP);

	// Load model collection
    entity_models = model_load("\\assets\\models\\entity.msh", 1, STACK_ENTITY, tex_entity_start, 0);
	mem_stack_release(STACK_TEMP);
}

void entity_register_collision_box(const entity_collision_box_t* box) {
	memcpy(&entity_aabb_queue[entity_n_active_aabb++], box, sizeof(entity_collision_box_t));
}

void entity_defragment(void) {
	int start = 0;
	int end = ENTITY_LIST_LENGTH - 1;

	while (1) {
		while (entity_types[end] == ENTITY_NONE) --end;
		while (entity_types[start] != ENTITY_NONE) ++start;
		if (start >= end) break;

		// swap *end and *start
		{
			uint8_t temp = entity_types[start];
			entity_types[start] = entity_types[end];
			entity_types[end] = temp;
		}
		{
			entity_union* pool = (entity_union*)entity_pool;
			entity_union temp = pool[start];
			pool[start] = pool[end];
			pool[end] = temp;
		}
	}
}

// Sets the mesh pointer, which is only valid for the runtime of this program, to 
// null. It will be recalculated in the entity code after loading
void entity_sanitize(void) {
	entity_union* pool = (entity_union*)entity_pool;
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		pool[i].header.mesh = NULL;
	}
}

void entity_kill(int slot) {
	// todo: maybe support destructors?
	entity_types[slot] = ENTITY_NONE;
}

void entity_send_player_intersect(int slot, player_t* player) {
	switch (entity_types[slot]) {
		case ENTITY_NONE: break;
		case ENTITY_DOOR: entity_door_player_intersect(slot, player); break;
		case ENTITY_PICKUP: entity_pickup_player_intersect(slot, player); break;
		case ENTITY_CRATE: entity_crate_player_intersect(slot, player); break;
		case ENTITY_CHASER: entity_chaser_player_intersect(slot, player); break;
		case ENTITY_PLATFORM: entity_platform_player_intersect(slot, player); break;
		case ENTITY_TRIGGER: entity_trigger_player_intersect(slot, player); break;
	}
}

const char* entity_names[] = {
	"ENTITY_NONE",
	"ENTITY_DOOR",
	"ENTITY_PICKUP",
	"ENTITY_CRATE",
	"ENTITY_CHASER",
	"ENTITY_PLATFORM",
	"ENTITY_TRIGGER",
	NULL
};

#ifdef _DEBUG
void entity_debug(void) {
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		printf("entity %03i:\t%s\n", i, entity_names[entity_types[i]]);
	}
}

int entity_how_many_active(void) {
	int count = 0;
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		if (entity_types[i] != ENTITY_NONE) {
			count++;
		}
	}
	return count;
}
#endif
