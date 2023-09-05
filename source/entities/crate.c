#include "crate.h"
#include "pickup.h"

entity_crate_t* entity_crate_new() {
	// Allocate memory for the entity
	entity_crate_t* entity = mem_stack_alloc(sizeof(entity_crate_t), STACK_ENTITY);
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = model_find_mesh(entity_models, "28_crate");
    entity->pickup_to_spawn = PICKUP_TYPE_NONE;

	// Register it in the entity list
	entity_register(&entity->entity_header, ENTITY_CRATE);

	return entity;
}

void entity_crate_update(int slot, player_t* player, int dt) {
	entity_crate_t* crate = (entity_crate_t*)entity_list[slot].data;
	vec3_t crate_pos = crate->entity_header.position;

	// Register collision
	aabb_t bounds = entity_models->meshes[ENTITY_MESH_CRATE].bounds;
	bounds.min.x *= COL_SCALE; bounds.min.y *= COL_SCALE; bounds.min.z *= COL_SCALE; 
	bounds.max.x *= COL_SCALE; bounds.max.y *= COL_SCALE; bounds.max.z *= COL_SCALE; 
	const aabb_t collision_box = {
		.min = vec3_sub(crate_pos, bounds.min),
		.max = vec3_sub(crate_pos, bounds.max)
	};
	entity_collision_box_t box = {
		.aabb = collision_box,
		.box_index = 0,
		.entity_index = slot,
		.is_solid = 1,
		.is_trigger = 0,
	};
	entity_register_collision_box(&box);

	// Render
	transform_t render_transform;
    render_transform.position.vx = -crate_pos.x / COL_SCALE;
    render_transform.position.vy = -crate_pos.y / COL_SCALE;
    render_transform.position.vz = -crate_pos.z / COL_SCALE;
    render_transform.rotation.vx = -crate->entity_header.rotation.x;
    render_transform.rotation.vy = -crate->entity_header.rotation.y;
    render_transform.rotation.vz = -crate->entity_header.rotation.z;
	render_transform.scale.vx = crate->entity_header.scale.x;
	render_transform.scale.vy = crate->entity_header.scale.x;
	render_transform.scale.vz = crate->entity_header.scale.x;
	renderer_draw_mesh_shaded_offset(crate->entity_header.mesh, &render_transform, tex_entity_start);
}

void entity_crate_on_hit(int slot, int hitbox_index) {
	entity_crate_t* crate = (entity_crate_t*)entity_list[slot].data;

	// Spawn pickup at this crate's location
	if (crate->pickup_to_spawn != PICKUP_TYPE_NONE) {
		entity_pickup_t* pickup = entity_pickup_new();
		pickup->entity_header = crate->entity_header;
		pickup->entity_header.mesh = NULL;
	}

    entity_kill(slot);
}