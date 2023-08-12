#include "entity.h"
#include "mesh.h"
#include "entities/door.h"

entity_slot_t entity_list[ENTITY_LIST_LENGTH];
model_t* entity_models = NULL;

void entity_update_all(player_t* player, int dt) {
	// Loop over all entities
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		switch (entity_list[i].type) {
			case ENTITY_NONE: break;
			case ENTITY_DOOR: entity_door_update(i, player, dt); break;
		}
	}
}

int entity_register(entity_header_t* entity, uint8_t entity_type) {
	// Find an empty slot
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) {
		if (entity_list[i].type == ENTITY_NONE) {
			// Register the entity
			entity_list[i].type = entity_type;
			entity_list[i].curr_section = ENTITY_NOT_SECTION_BOUND;
			entity_list[i].data = entity;
			return i;
		}
	}
	return -1;
}

void entity_init() {
	// Zero initialize the entity list
	for (int i = 0; i < ENTITY_LIST_LENGTH; ++i) entity_list[i].type = ENTITY_NONE;

	// Load model collection
    entity_models = model_load("\\ASSETS\\MODELS\\ENTITY.MSH;1", 1, STACK_ENTITY);
}

char* entity_names[] = {
	"ENTITY_NONE",
	"ENTITY_DOOR",
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