#include "chaser.h"

#include "../random.h"
#include "../main.h"
extern state_vars_t state;
#define CHASER_BEHAVIOUR_PERIOD 16
#define CHASER_REACTION_TIME_MIN 200 // in milliseconds
#define CHASER_REACTION_TIME_MAX 400 
#define CHASER_AGGRO_DISTANCE_SQUARED (320000*1000)
#define CHASER_STRAFE_DISTANCE_SQUARED (80000*1000)
#define CHASER_FLEE_DISTANCE_SQUARED (64000*1000)
#define CHASER_NODE_REACH_DISTANCE_SQUARED (800*1000)

// Slightly cursed but it makes the rest of the code more readable so eh
#define n_nav_graph_nodes state.in_game.level.collision_bvh.n_nav_graph_nodes
#define nav_graph_nodes state.in_game.level.collision_bvh.nav_graph_nodes

entity_chaser_t* entity_chaser_new(void) {
	// Allocate memory for the entity
	entity_chaser_t* entity = (entity_chaser_t*)&entity_pool[entity_alloc(ENTITY_CHASER) * entity_pool_stride];
	entity->entity_header.position = (vec3_t){ 0, 0, 0 };
	entity->entity_header.rotation = (vec3_t){ 0, 0, 0 };
	entity->entity_header.scale = (vec3_t){ ONE, ONE, ONE };
	entity->entity_header.mesh = model_find_mesh(entity_models, "19_enemy_chaser_idle");
	entity->curr_navmesh_node = -1;
	entity->target_navmesh_node = -1;
	entity->state = CHASER_WAIT;
	entity->velocity = vec3_from_scalar(0);
	entity->behavior_timer = 0;
	entity->last_known_player_pos = vec3_from_scalar(0);

	return entity;
}

void chaser_wait(entity_chaser_t* chaser, player_t* player, int dt) {
	(void)dt;
	const vec3_t chaser_pos = chaser->entity_header.position;

	// Wait for the entity behavior timer to reach the desired value
	if (chaser->behavior_timer <= 0) {
		// Set timer to random value in milliseconds - mostly to reduce lag lol
		// Random timers for each enemy means the raycasts are more spread out
		// It also makes their reaction times more realistic so there's that too
		chaser->behavior_timer = random_range(CHASER_REACTION_TIME_MIN, CHASER_REACTION_TIME_MAX);

		int player_visible = 0;

		// Find distance to player
		const vec3_t chaser_to_player = vec3_sub(player->position, chaser_pos);
		const scalar_t dist_chaser_to_player_squared = vec3_magnitude_squared(chaser_to_player);

		// If the player is too far away, ignore them, and don't bother with a raycast
		if (dist_chaser_to_player_squared < CHASER_AGGRO_DISTANCE_SQUARED) {
			ray_t ray = {
				.position = chaser_pos,
				.direction = vec3_normalize(chaser_to_player),
				.length = INT32_MAX,
			};
			ray.inv_direction = vec3_div((vec3_t){ONE, ONE, ONE}, ray.direction);

			rayhit_t hit = {0};
			bvh_intersect_ray(&state.in_game.level.collision_bvh, ray, &hit);

			// If the ray didn't hit anything, the player is visible
			if (is_infinity(hit.distance)) 
				player_visible = 1;

			// If the ray hit something that's further away from the enemy than the player is, the player is visible
			else if (scalar_mul(hit.distance, hit.distance) > dist_chaser_to_player_squared)
				player_visible = 1;
		}

		if (player_visible) {
			chaser->last_known_player_pos = player->position;

			// If player is a bit far away, chase them
			chaser->behavior_timer = 0; // Act immediately after switching state
				// printf("chase\n");
			if (dist_chaser_to_player_squared < CHASER_AGGRO_DISTANCE_SQUARED) {
				chaser->state = CHASER_CHASE_PLAYER;
			}
			// If player is close enough, start strafing
			else if (dist_chaser_to_player_squared < CHASER_STRAFE_DISTANCE_SQUARED) {
				printf("chase\n");
				chaser->state = CHASER_STRAFE;
			}
			// If the player is too close, retreat
			else if (dist_chaser_to_player_squared < CHASER_FLEE_PLAYER) {
				printf("flee\n");
				chaser->state = CHASER_FLEE_PLAYER;
			}
		}
	}
}

void chaser_chase_player(entity_chaser_t* chaser, player_t* player, int dt) {
	if (chaser->curr_navmesh_node < 0 || chaser->curr_navmesh_node >= n_nav_graph_nodes) 
		return;
	const vec3_t chaser_pos = chaser->entity_header.position;

	// Find neighbor node that's closest to the player
	if (chaser->behavior_timer <= 0) {
		// Find closest node to player
		chaser->behavior_timer = random_range(CHASER_REACTION_TIME_MIN, CHASER_REACTION_TIME_MAX);
		if ((chaser->target_navmesh_node == -1) || (chaser->target_navmesh_node == chaser->curr_navmesh_node)) {
			scalar_t closest_distance_squared = INT32_MAX;
			for (size_t i = 0; i < 4; ++i) {
				const uint16_t neighbor_id = nav_graph_nodes[chaser->curr_navmesh_node].neighbor_ids[i];
				if (
					neighbor_id == 0xFFFF || 
					neighbor_id >= n_nav_graph_nodes || 
					neighbor_id == chaser->curr_navmesh_node
				) continue;
				const vec3_t node_position = vec3_from_svec3(nav_graph_nodes[neighbor_id].position);
				const scalar_t distance_squared = vec3_magnitude_squared(vec3_sub(node_position, player->position));
				if (distance_squared < closest_distance_squared) {
					closest_distance_squared = distance_squared;
					chaser->target_navmesh_node = neighbor_id;
				}
			}
		}

		// If we're close enough to shoot, switch to that state
		const scalar_t distance_to_player_squared = vec3_magnitude_squared(vec3_sub(chaser_pos, player->position));
		if (distance_to_player_squared < CHASER_FLEE_DISTANCE_SQUARED) {
			// printf("shoot\n");
			chaser->state = CHASER_SHOOT;
		}
	}
}

void chaser_flee_player(entity_chaser_t* chaser, player_t* player, int dt) {
	if (chaser->behavior_timer <= 0) {
		chaser->state = CHASER_WAIT;
		chaser->behavior_timer = random_range(CHASER_REACTION_TIME_MIN, CHASER_REACTION_TIME_MAX);
	}
}

void chaser_strafe(entity_chaser_t* chaser, player_t* player, int dt) {
	if (chaser->behavior_timer <= 0) {
		chaser->state = CHASER_WAIT;
		chaser->behavior_timer = random_range(CHASER_REACTION_TIME_MIN, CHASER_REACTION_TIME_MAX);
	}
}

void chaser_shoot(entity_chaser_t* chaser, player_t* player, int dt) {
	if (chaser->behavior_timer <= 0) {
		chaser->state = CHASER_WAIT;
		chaser->behavior_timer = random_range(CHASER_REACTION_TIME_MIN, CHASER_REACTION_TIME_MAX);
	}
}

void entity_chaser_update(int slot, player_t* player, int dt) {
	entity_chaser_t* chaser = (entity_chaser_t*)&entity_pool[slot * entity_pool_stride];
	vec3_t chaser_pos = chaser->entity_header.position;

	// Register hitboxes
	aabb_t bounds_body = (aabb_t){
		.min = (vec3_t){
			chaser_pos.x - (-69 * COL_SCALE), 
			chaser_pos.y - (-230 * COL_SCALE), 
			chaser_pos.z - (-69 * COL_SCALE )},
		.max = (vec3_t){ 
			chaser_pos.x - (69 * COL_SCALE), 
			chaser_pos.y - (0 * COL_SCALE), 
			chaser_pos.z - (69 * COL_SCALE)
		},
	};
	aabb_t bounds_head = (aabb_t){
		.min = (vec3_t){
			chaser_pos.x - (-35 * COL_SCALE), 
			chaser_pos.y - (-340 * COL_SCALE), 
			chaser_pos.z - (-35 * COL_SCALE )},
		.max = (vec3_t){ 
			chaser_pos.x - (25 * COL_SCALE), 
			chaser_pos.y - (-230 * COL_SCALE), 
			chaser_pos.z - (35 * COL_SCALE)
		},
	};
	entity_collision_box_t box_body = {
		.aabb = bounds_body,
		.box_index = 0,
		.entity_index = slot,
		.is_solid = 0,
		.is_trigger = 0,
	};
	entity_collision_box_t box_head = {
		.aabb = bounds_head,
		.box_index = 1,
		.entity_index = slot,
		.is_solid = 0,
		.is_trigger = 0,
	};
	entity_register_collision_box(&box_body);
	entity_register_collision_box(&box_head);

	// If the enemy doesn't know where it is, figure that out
	if (chaser->curr_navmesh_node < 0 || chaser->curr_navmesh_node >= n_nav_graph_nodes) {
		scalar_t curr_min_distance = INT32_MAX;

		for (int i = 0; i < n_nav_graph_nodes; ++i) {
			const vec3_t node_position = vec3_from_svec3(nav_graph_nodes[i].position);
			scalar_t distance_squared = vec3_magnitude_squared(vec3_sub(node_position, chaser_pos));
			if (distance_squared >= 0 && distance_squared < curr_min_distance) {
				curr_min_distance = distance_squared;
				chaser->curr_navmesh_node = i;
			}
		}
	}

	if (chaser->behavior_timer > 0) chaser->behavior_timer -= dt;
	switch (chaser->state) {
		case CHASER_WAIT: 			chaser_wait(chaser, player, dt);			break;
		case CHASER_CHASE_PLAYER: 	chaser_chase_player(chaser, player, dt);	break;
		case CHASER_FLEE_PLAYER: 	chaser_flee_player(chaser, player, dt);		break;
		case CHASER_STRAFE: 		chaser_strafe(chaser, player, dt);			break;
		case CHASER_SHOOT: 			chaser_shoot(chaser, player, dt);			break;
		default: 					chaser->state = CHASER_WAIT; /* Failsafe */	break;
	}

	// Handle velocity
	scalar_t velocity_scalar = scalar_sqrt(vec3_magnitude_squared(chaser->velocity));
	if (velocity_scalar > 0) {
		vec3_t velocity_normalized = vec3_divs(chaser->velocity, velocity_scalar);

		// fallback because for some reason my level editor isnt exporting the right values
		if (velocity_scalar > 1000000) {
			velocity_normalized = vec3_from_scalar(0);
			velocity_scalar = 0;
		}
		if (velocity_scalar > chaser_max_speed) {
			velocity_scalar = chaser_max_speed;
		}
		if (velocity_scalar > chaser_drag * dt) {
			velocity_scalar -= chaser_drag * dt;
		}
		chaser->velocity = vec3_muls(velocity_normalized, velocity_scalar);

		chaser->entity_header.position.x += (chaser->velocity.x / 256) * dt;
		chaser->entity_header.position.y += (chaser->velocity.y / 256) * dt;
		chaser->entity_header.position.z += (chaser->velocity.z / 256) * dt;
	}

	// Add a force towards the target node
	if (chaser->target_navmesh_node >= 0 && chaser->target_navmesh_node < n_nav_graph_nodes) {
		const vec3_t target_pos = vec3_from_svec3(nav_graph_nodes[chaser->target_navmesh_node].position);
		const vec3_t target_dir = vec3_normalize(vec3_sub(target_pos, chaser_pos));
		chaser->velocity = vec3_add(chaser->velocity, vec3_muls(target_dir, chaser_acceleration * dt));

		// If we're close to the target node, set that node as the current node
		const scalar_t distance_to_target_node_squared = vec3_magnitude_squared(vec3_sub(chaser_pos, target_pos));
		if (distance_to_target_node_squared < CHASER_NODE_REACH_DISTANCE_SQUARED) {
			chaser->curr_navmesh_node = chaser->target_navmesh_node;
		}
	}

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
	if (chaser->entity_header.mesh == NULL) {
		chaser->entity_header.mesh = model_find_mesh(entity_models, "19_enemy_chaser_idle");
	}
	renderer_draw_mesh_shaded(chaser->entity_header.mesh, &render_transform, 0, tex_entity_start);
}

void entity_chaser_on_hit(int slot, int hitbox_index) {
	(void)slot;
	if (hitbox_index == 0) {
		// todo: handle body hits
		printf("hit enemy body\n");
	}
	if (hitbox_index == 1) {
		// todo: handle head hits
		printf("hit enemy head\n");
	}
}