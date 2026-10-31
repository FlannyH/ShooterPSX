#include "cinecam.h"
#include "engine/renderer.h"

entity_cinecam_t* entity_cinecam_new(void) {
	entity_cinecam_t* entity = (entity_cinecam_t*)entity_get_header(entity_alloc(ENTITY_CINECAM));
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = NULL;
	entity->n_nodes = 1;
	entity->nodes = mem_alloc(PATH_CAMERA_MAX_NODES * sizeof(*entity->nodes), MEM_CAT_UNDEFINED);
	return entity;
}

size_t entity_cinecam_serialize(int slot, uint8_t* data) {
    entity_cinecam_t* cinecam_source = (entity_cinecam_t*)entity_get_header(slot);

    uint32_t* n_nodes = (uint32_t*)(data + sizeof(vec3_t) * 3);
    cinecam_node_t* nodes = (cinecam_node_t*)&n_nodes[1];

    *n_nodes = cinecam_source->n_nodes;

    for (size_t i = 0; i < cinecam_source->n_nodes; ++i) {
        cinecam_node_t* node = &nodes[i];
        const cinecam_node_t* source = &cinecam_source->nodes[i];
        serialize_vec3(&node->position, source->position);
        serialize_vec3(&node->rotation, source->rotation);
        serialize_scalar(&node->time_to_next, source->time_to_next);
    }
    size_t size = 3 * sizeof(vec3_t); // header
    size += sizeof(cinecam_source->n_nodes);
    size += cinecam_source->n_nodes * sizeof(nodes[0].position);
    size += cinecam_source->n_nodes * sizeof(nodes[0].rotation);
    size += cinecam_source->n_nodes * sizeof(nodes[0].time_to_next);
    return size;
}

size_t entity_cinecam_deserialize(int slot, const uint8_t* data) {
    entity_cinecam_t* cinecam_dest = (entity_cinecam_t*)entity_get_header(slot);
    uint32_t* n_nodes = (uint32_t*)(data + (sizeof(vec3_t) * 3));
    cinecam_node_t* nodes_src = (cinecam_node_t*)(&n_nodes[1]);

    cinecam_dest->n_nodes = *n_nodes;
    #ifdef _LEVEL_EDITOR
	cinecam_dest->nodes = mem_alloc(PATH_CAMERA_MAX_NODES * sizeof(cinecam_node_t), MEM_CAT_UNDEFINED);
	#else
	cinecam_dest->nodes = mem_stack_alloc(*n_nodes * sizeof(cinecam_node_t), STACK_ENTITY);
	#endif

	cinecam_dest->n_nodes = *n_nodes;

	for (int i = 0; i < *n_nodes; ++i) {
        const cinecam_node_t* node = &nodes_src[i];
        cinecam_node_t* dest = &cinecam_dest->nodes[i];
        deserialize_vec3(&node->position, &dest->position);
        deserialize_vec3(&node->rotation, &dest->rotation);
        deserialize_scalar(&node->time_to_next, &dest->time_to_next);
	}

	cinecam_dest->progress = 0;

    size_t size = 3 * sizeof(vec3_t); // header
    size += sizeof(*n_nodes);
    size += (*n_nodes) * sizeof(vec3_t); // position
    size += (*n_nodes) * sizeof(vec3_t); // rotation
    size += (*n_nodes) * sizeof(scalar_t); // velocity
    return size;
}

void entity_cinecam_update(int slot, player_t* player, scalar_t dt) {
    entity_cinecam_t* cinecam = (entity_cinecam_t*)entity_get_header(slot);

    // interpolate
    int prev_index = scalar_clamp((cinecam->progress / ONE), 0, cinecam->n_nodes - 2);
    int target_index = prev_index + 1;
    scalar_t t = cinecam->progress % ONE;
    cinecam->entity_header.position = vec3_lerp(cinecam->nodes[prev_index].position, cinecam->nodes[target_index].position, t);
    cinecam->entity_header.rotation = vec3_lerp(cinecam->nodes[prev_index].rotation, cinecam->nodes[target_index].rotation, t);
    scalar_t velocity = scalar_lerp(cinecam->nodes[prev_index].time_to_next, cinecam->nodes[target_index].time_to_next, t); // distance / seconds
    vec3_t dir = vec3_normalize(vec3_sub(cinecam->entity_header.position, cinecam->nodes[target_index].position));
    if (cinecam->n_nodes > 1) cinecam->progress += scalar_div(dt, cinecam->nodes[prev_index].time_to_next);
    if (cinecam->progress > 0 && cinecam->progress > SCALAR(cinecam->n_nodes-1)) {
        entity_kill(slot);
        return;
    }

    transform_t transform = {
        .position = cinecam->entity_header.position,
        .rotation = cinecam->entity_header.rotation,
        .scale = vec3_from_scalar(ONE)
    };
    renderer_set_view(&transform);
}

void entity_cinecam_on_hit(int slot, int hitbox_index) {

}

void entity_cinecam_player_intersect(int slot, player_t* player) {

}
