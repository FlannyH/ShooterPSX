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

entity_collision_box_t entity_aabb_queue[ENTITY_AABB_QUEUE_LENGTH];
uint8_t entity_types[ENTITY_LIST_LENGTH];
uint8_t* entity_pool = NULL;
size_t entity_pool_stride = 0;
size_t entity_n_active_aabb = 0;
model_t* entity_models = NULL;
int n_entity_textures = 0;
int entity_signals[ENTITY_SIGNAL_COUNT];

void entity_update_all(player_t* player, int dt) {
	// Reset counters
	entity_n_active_aabb = 0;

#ifndef _LIGHT_BAKE
	// Update all entities
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
#endif
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

// Make sure to update entity_union when adding new entity types
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
	mem_stack_release(STACK_ENTITY);

	// Zero initialize the entity list
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) entity_types[i] = ENTITY_NONE;

	entity_n_active_aabb = 0;

	// Allocate entity pool
	entity_pool_stride = sizeof(entity_union);
	entity_pool = mem_stack_alloc(ENTITY_LIST_LENGTH * sizeof(entity_union), STACK_ENTITY);

    // Load entity textures
	texture_cpu_t *entity_textures;
    tex_entity_start = tex_alloc_cursor;
	n_entity_textures = texture_collection_load("models/entity.txc", &entity_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
	    renderer_upload_texture(&entity_textures[i], i + tex_entity_start);
	}
	tex_alloc_cursor += n_entity_textures;
	mem_stack_release(STACK_TEMP);

	// Load model collection
#ifdef _LEVEL_EDITOR
	if (entity_models) {
		for (uint32_t i = 0; i < entity_models->n_meshes; ++i) {
			mem_free(entity_models->meshes[i].vertices);
			mem_free(entity_models->meshes[i].normals);
		}
	}
#endif
	entity_models = model_load("models/entity.msh", 1, STACK_ENTITY, tex_entity_start, 0);
	mem_stack_release(STACK_TEMP);

	memset(entity_signals, 0, sizeof(entity_signals));
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
			const uint8_t temp = entity_types[start];
			entity_types[start] = entity_types[end];
			entity_types[end] = temp;
		}
		{
			entity_union* pool = (entity_union*)entity_pool;
			const entity_union temp = pool[start];
			pool[start] = pool[end];
			pool[end] = temp;
		}
	}
}

// Sets the mesh pointer, which is only valid for the runtime of this program, to null. 
// It will be recalculated in the entity code later
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
#ifndef _LIGHT_BAKE
	switch (entity_types[slot]) {
		case ENTITY_NONE: break;
		case ENTITY_DOOR: entity_door_player_intersect(slot, player); break;
		case ENTITY_PICKUP: entity_pickup_player_intersect(slot, player); break;
		case ENTITY_CRATE: entity_crate_player_intersect(slot, player); break;
		case ENTITY_CHASER: entity_chaser_player_intersect(slot, player); break;
		case ENTITY_PLATFORM: entity_platform_player_intersect(slot, player); break;
		case ENTITY_TRIGGER: entity_trigger_player_intersect(slot, player); break;
	}
#endif
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

model_t* entity_get_models(void) {
	return entity_models;
}

void entity_set_type(int index, uint8_t type) {
#ifdef _DEBUG
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_LIST_LENGTH);
#endif
	entity_types[index] = type;
}

uint8_t entity_get_type(int index) {
#ifdef _DEBUG
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_LIST_LENGTH);
#endif
	return entity_types[index];
}

entity_header_t* entity_get_header(int index) {
#ifdef _DEBUG
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_LIST_LENGTH);
#endif
	return (entity_header_t*)(&entity_pool[index * entity_pool_stride]);
}

void entity_deserialize_and_write_slot(int slot, const entity_header_serialized_t* header) {
	const uint8_t* src_entity_data = (const uint8_t*)&header[1]; // right after the entity header

	// Find where data needs to be written
	entity_header_t* dst_entity_header = (entity_header_t*)(entity_pool + (slot * entity_pool_stride));
	uint8_t* dst_entity_data = (uint8_t*)&dst_entity_header[1]; // right after the entity header

	// Write entity header
	dst_entity_header->position = header->position;
	dst_entity_header->rotation = header->rotation;
	dst_entity_header->scale = header->scale;
	dst_entity_header->mesh = NULL;

	// Copy entity data - we just need to copy everything after the header, so subtract the header size
	memcpy(dst_entity_data, src_entity_data, entity_pool_stride - sizeof(entity_header_t));
}

size_t entity_get_pool_stride(void) {
	return entity_pool_stride;
}

size_t entity_get_n_active_aabb(void) {
	return entity_n_active_aabb;
}

entity_collision_box_t* entity_get_aabb_queue_entry(int index) {
#ifdef _DEBUG
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_AABB_QUEUE_LENGTH);
#endif
	return &entity_aabb_queue[index];
}

int entity_get_signal(int index) {
#ifdef _DEBUG
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_SIGNAL_COUNT);
#endif
	return entity_signals[index];
}

void entity_set_signal(int index, int value) {
#ifdef _DEBUG
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_SIGNAL_COUNT);
#endif
	entity_signals[index] = value;
}

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
