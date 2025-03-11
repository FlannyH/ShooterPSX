#include "crate.h"

#include "pickup.h"

entity_crate_t* entity_crate_new(void) {
	// Allocate memory for the entity
	entity_crate_t* entity = (entity_crate_t*)entity_get_header(entity_alloc(ENTITY_CRATE));
	entity->entity_header.position = (vec3_t){0, 0, 0};
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = model_find_mesh(entity_get_models(), "28_crate");
    entity->pickup_to_spawn = PICKUP_TYPE_NONE;
	return entity;
}

void entity_crate_update(int slot, player_t* player, int dt) {
	PANIC_IF("entity_models is null!", entity_get_models() == NULL);
	PANIC_IF("entity_models->meshes is null!", entity_get_models()->meshes == NULL);
	PANIC_IF("entity_models does not contain enough meshes!", entity_get_models()->n_meshes < N_ENTITY_MESH_IDS);

	(void)dt;
	(void)player;
	entity_crate_t* crate = (entity_crate_t*)entity_get_header(slot);
	const vec3_t crate_pos = crate->entity_header.position;

	// Register collision
	aabb_t bounds = entity_get_models()->meshes[ENTITY_MESH_CRATE].bounds;
	bounds.min.x *= COL_SCALE; bounds.min.y *= COL_SCALE; bounds.min.z *= COL_SCALE; 
	bounds.max.x *= COL_SCALE; bounds.max.y *= COL_SCALE; bounds.max.z *= COL_SCALE; 
	const aabb_t collision_box = {
		.min = vec3_sub(crate_pos, bounds.max),
		.max = vec3_sub(crate_pos, bounds.min)
	};
	const entity_collision_box_t box = {
		.aabb = collision_box,
		.box_index = 0,
		.entity_index = slot,
		.is_solid = 1,
		.is_trigger = 0,
	};
	entity_register_collision_box(&box);

	// Render
	transform_t render_transform;
    render_transform.position.x = -crate_pos.x / COL_SCALE;
    render_transform.position.y = -crate_pos.y / COL_SCALE;
    render_transform.position.z = -crate_pos.z / COL_SCALE;
    render_transform.rotation.x = -crate->entity_header.rotation.x;
    render_transform.rotation.y = -crate->entity_header.rotation.y;
    render_transform.rotation.z = -crate->entity_header.rotation.z;
	render_transform.scale.x = crate->entity_header.scale.x;
	render_transform.scale.y = crate->entity_header.scale.x;
	render_transform.scale.z = crate->entity_header.scale.x;
#ifdef _LEVEL_EDITOR
	renderer_set_drawing_id(slot, 1);
#endif
	if (crate->entity_header.mesh == NULL) {
		crate->entity_header.mesh = model_find_mesh(entity_get_models(), "28_crate");
	}
	renderer_draw_mesh_shaded(crate->entity_header.mesh, &render_transform, 0, 0, tex_entity_start);
}

void entity_crate_on_hit(int slot, int hitbox_index) {
	(void)hitbox_index;
	entity_crate_t* crate = (entity_crate_t*)entity_get_header(slot);
	crate->entity_header.mesh = NULL; // Mark the mesh for refreshing
	entity_set_type(slot, (uint8_t)ENTITY_PICKUP); // pickup struct is practically identical to crate struct so this works :D
}

void entity_crate_player_intersect(int slot, player_t *player) {
	(void)slot;
	(void)player;
}
