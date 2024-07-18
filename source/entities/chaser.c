#include "chaser.h"

entity_chaser_t* entity_chaser_new() {
	// Allocate memory for the entity
	entity_chaser_t* entity = (entity_chaser_t*)&entity_pool[entity_alloc(ENTITY_CHASER) * entity_pool_stride];
	entity->entity_header.position = (vec3_t){ 0, 0, 0 };
	entity->entity_header.rotation = (vec3_t){ 0, 0, 0 };
	entity->entity_header.scale = (vec3_t){ ONE, ONE, ONE };
	entity->entity_header.mesh = model_find_mesh(entity_models, "19_enemy_chaser_idle");

	return entity;
}

void entity_chaser_update(int slot, player_t* player, int dt) {
	entity_chaser_t* chaser = (entity_chaser_t*)&entity_pool[slot * entity_pool_stride];
	vec3_t chaser_pos = chaser->entity_header.position;

	// todo: switch meshes to make animation
	// todo: register body hitbox
	// todo: register head hitbox
	// todo: implement behaviour
	
	// Render
	transform_t render_transform;
	render_transform.position.x = -chaser_pos.x / COL_SCALE;
	render_transform.position.y = -chaser_pos.y / COL_SCALE;
	render_transform.position.z = -chaser_pos.z / COL_SCALE;
	render_transform.rotation.x = -chaser->entity_header.rotation.x;
	render_transform.rotation.y = -chaser->entity_header.rotation.y;
	render_transform.rotation.z = -chaser->entity_header.rotation.z;
	render_transform.scale.x = chaser->entity_header.scale.x;
	render_transform.scale.y = chaser->entity_header.scale.x;
	render_transform.scale.z = chaser->entity_header.scale.x;
#ifdef _LEVEL_EDITOR
	drawing_entity_id = slot;
#endif
	renderer_draw_mesh_shaded_offset(chaser->entity_header.mesh, &render_transform, tex_entity_start);
}

void entity_chaser_on_hit(int slot, int hitbox_index) {
	// todo: handle body hits
	// todo: handle head hits
}