#include "entity.h"

#include "renderer.h"
#include "mesh.h"

#include <string.h>

entity_reg_t entity_register[ENTITY_LIST_LENGTH] = {0};
entity_collision_box_t entity_aabb_queue[ENTITY_AABB_QUEUE_LENGTH];
uint8_t entity_types[ENTITY_LIST_LENGTH];
uint8_t* entity_pool = NULL;
size_t entity_pool_stride = 0;
size_t entity_n_active_aabb = 0;
model_t* entity_models = NULL;
int n_entity_textures = 0;
int entity_signals[ENTITY_SIGNAL_COUNT];

void entity_update_all(player_t* player, scalar_t dt, level_t* level) {
	entity_n_active_aabb = 0;

	// Update all entities
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
    	entity_reg_t* entity_funcs = &entity_register[entity_get_type(i)];
    	if (entity_funcs->update) entity_funcs->update(i, player, dt, level);
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

void entity_init(void) {
	mem_stack_release(STACK_ENTITY);

	// Zero initialize the entity list
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) entity_types[i] = ENTITY_NONE;

	entity_n_active_aabb = 0;
	if (entity_pool_stride == 0) entity_pool_stride = sizeof(entity_header_t);

	// Allocate entity pool
	entity_pool = mem_stack_alloc(ENTITY_LIST_LENGTH * entity_pool_stride, STACK_ENTITY);

    // Load entity textures
	texture_cpu_t* entity_textures;
	n_entity_textures = texture_collection_load("models/entity.txc", &entity_textures, 1, STACK_TEMP);
	renderer_free_texture_category(TEX_CAT_ENTITY);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
		renderer_upload_texture(&entity_textures[(size_t)i], i, TEX_CAT_ENTITY);
	}
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
	entity_models = model_load("models/entity.msh", 1, STACK_ENTITY, TEX_CAT_ENTITY, 0);
	mem_stack_release(STACK_TEMP);

	memset(entity_signals, 0, sizeof(entity_signals));
}

void entity_register_type(uint8_t entity_type, entity_reg_t registration) {
    entity_register[entity_type] = registration;
}

const entity_reg_t* entity_register_get(uint8_t entity_type) {
    return &entity_register[entity_type];
}

void entity_register_collision_box(const entity_collision_box_t* box) {
    entity_aabb_queue[entity_n_active_aabb++] = *box;
}

void entity_call_on_hit(int entity_index, int box_index) {
	PANIC_IF("index out of bounds", entity_index < 0 || entity_index >= ENTITY_LIST_LENGTH);
	entity_reg_t* entity_funcs = &entity_register[entity_get_type(entity_index)];
	if (entity_funcs->on_hit) entity_funcs->on_hit(entity_index, box_index);
}

void entity_call_player_intersect(int entity_index, player_t* player) {
	PANIC_IF("index out of bounds", entity_index < 0 || entity_index >= ENTITY_LIST_LENGTH);
	entity_reg_t* entity_funcs = &entity_register[entity_get_type(entity_index)];
	if (entity_funcs->player_intersect) entity_funcs->player_intersect(entity_index, player);
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
            uint8_t* ptr_to_start = &entity_pool[start * entity_pool_stride];
            uint8_t* ptr_to_end = &entity_pool[end * entity_pool_stride];

            for (size_t i = 0; i < entity_pool_stride; ++i) {
    			const uint8_t temp = *ptr_to_start;
    			*ptr_to_start = *ptr_to_end;
    			*ptr_to_end = temp;
            }
		}
	}
}

// Sets the mesh pointer, which is only valid for the runtime of this program, to null.
// It will be recalculated in the entity code later
void entity_sanitize(void) {
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
	    entity_header_t* header = (entity_header_t*)(&entity_pool[i * entity_pool_stride]);
		header->mesh = NULL;
	}
}

void entity_kill(int slot) {
	PANIC_IF("index out of bounds", slot < 0 || slot >= ENTITY_LIST_LENGTH);
	// todo(entity_destructors): desc: maybe support destructors?
	entity_types[slot] = ENTITY_NONE;
}

const char* entity_names[] = {
	"(none)",
	"Door",
	"Pickup",
	"Crate",
	"Chaser enemy",
	"Platform",
	"Trigger box",
	"Cinecam",
	"Benchmarker",
	NULL
};

model_t* entity_get_models(void) {
	return entity_models;
}

void entity_set_type(int index, uint8_t type) {
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_LIST_LENGTH);
	entity_types[index] = type;
}

uint8_t entity_get_type(int index) {
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_LIST_LENGTH);
	return entity_types[index];
}

entity_header_t* entity_get_header(int index) {
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_LIST_LENGTH);
	return (entity_header_t*)(&entity_pool[index * entity_pool_stride]);
}

size_t entity_deserialize_and_write_slot(int slot, const entity_header_serialized_t* header, uint8_t type) {
    size_t entity_size = entity_get_pool_stride();
	const uint8_t* src_entity_data = (const uint8_t*)header;

	entity_reg_t* entity_funcs = &entity_register[type];
    entity_size = (entity_funcs->deserialize != NULL) ? entity_funcs->deserialize(slot, src_entity_data) : 0;

	// Write entity header
	entity_header_t* dst_entity_header = entity_get_header(slot);
	deserialize_vec3(&header->position, &dst_entity_header->position);
	deserialize_vec3(&header->rotation, &dst_entity_header->rotation);
	deserialize_vec3(&header->scale, &dst_entity_header->scale);
	dst_entity_header->mesh = NULL;

	return entity_size;
}

size_t entity_serialize(int slot, uint8_t* dest, uint8_t type) {
    size_t entity_size = entity_get_pool_stride();

	entity_reg_t* entity_funcs = &entity_register[type];
    entity_size = (entity_funcs->serialize != NULL) ? entity_funcs->serialize(slot, dest) : 0;

	entity_header_t* src_entity_header = entity_get_header(slot);
	entity_header_t* dst_entity_header = (entity_header_t*)dest;
	serialize_vec3(&dst_entity_header->position, src_entity_header->position);
	serialize_vec3(&dst_entity_header->rotation, src_entity_header->rotation);
	serialize_vec3(&dst_entity_header->scale, src_entity_header->scale);
	return entity_size;
}

size_t entity_get_pool_stride(void) {
	return entity_pool_stride;
}

size_t entity_get_n_active_aabb(void) {
	return entity_n_active_aabb;
}

entity_collision_box_t* entity_get_aabb_queue_entry(int index) {
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_AABB_QUEUE_LENGTH);
	return &entity_aabb_queue[index];
}

int entity_get_signal(int index) {
	if (index < 0 || index >= ENTITY_SIGNAL_COUNT) return 0;
	return entity_signals[index];
}

void entity_set_signal(int index, int value) {
	PANIC_IF("index out of bounds", index < 0 || index >= ENTITY_SIGNAL_COUNT);
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
