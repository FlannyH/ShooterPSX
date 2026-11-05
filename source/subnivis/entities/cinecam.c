#include "cinecam.h"
#include "engine/renderer.h"

#ifdef _LEVEL_EDITOR
#include "editor/editor.h"

int entity_cinecam_inspect(int slot) {
    entity_cinecam_t* cinecam = (entity_cinecam_t*)entity_get_header(slot);

    int result = 0;

    if (inspect_tree_push("Nodes")) {
        for (int i = 0; i < cinecam->n_nodes; ++i) {
            renderer_set_drawing_id(0, 0);
            renderer_set_depth_bias(0);

            editor_draw_sphere((transform_t) {
                .position = editor_selected_vertex_position(),
                .rotation = vec3_from_scalar(0),
                .scale = vec3_from_scalar(SCALAR(32)),
            });

            if ((i > 0) && (cinecam->n_nodes > 1)) {
                renderer_debug_draw_line(cinecam->nodes[i-1].position, cinecam->nodes[i].position, white, &id_transform);
            }

            char node_name[] = "Node 9999\n";
            snprintf(node_name, sizeof(node_name), "Node %i", i);
            if (!inspect_tree_push(node_name)) continue;

            inspect_index_push(i);
            if (!cinecam->nodes) break;

            result |= inspect_vec3(&cinecam->nodes[i].position, "Position");
            result |= inspect_vec3(&cinecam->nodes[i].rotation, "Rotation");
            result |= inspect_scalar(&cinecam->nodes[i].time_to_next, "Seconds to next node");
            if (inspect_button("Move viewport to node")) {
                editor_set_viewport_position(cinecam->nodes[i].position);
                editor_set_viewport_rotation(cinecam->nodes[i].rotation);
                result |= 1;
            }
            if (inspect_button("Move node to viewport")) {
                cinecam->nodes[i].position = editor_get_viewport_position();
                cinecam->nodes[i].rotation = editor_get_viewport_rotation();
                result |= 1;
            }
            if (inspect_button("Remove node")) {
                cinecam->n_nodes--;
                for (int j = i; j < cinecam->n_nodes; ++j) {
                    cinecam->nodes[j] = cinecam->nodes[j + 1];
                }
                i--;
                result |= 1;
            }
            inspect_index_pop();
            inspect_tree_pop();
        }
        inspect_tree_pop();

    }
    inspect_bool(&cinecam->active, "Active");
    inspect_scalar(&cinecam->progress, "Progress");

    if (cinecam->progress < 0) cinecam->progress = 0;
    if (cinecam->progress > SCALAR(cinecam->n_nodes)) cinecam->progress = SCALAR(cinecam->n_nodes);

    if (inspect_button("Add")) {
        cinecam->nodes[cinecam->n_nodes].position = editor_get_viewport_position();
        cinecam->nodes[cinecam->n_nodes++].rotation = editor_get_viewport_rotation();
        result |= 1;
    }

    return result;
}
#endif

entity_header_t* entity_cinecam_new(vec3_t spawn_pos) {
	entity_cinecam_t* entity = (entity_cinecam_t*)entity_get_header(entity_alloc(ENTITY_CINECAM));
	entity->entity_header.position = spawn_pos;
	entity->entity_header.rotation = (vec3_t){0, 0, 0};
	entity->entity_header.scale = (vec3_t){ONE, ONE, ONE};
	entity->entity_header.mesh = NULL;
	entity->n_nodes = 1;
	entity->nodes = mem_alloc(PATH_CAMERA_MAX_NODES * sizeof(*entity->nodes), MEM_CAT_UNDEFINED);
	entity->progress = 0;
	entity->active = 0;
	return (entity_header_t*)entity;
}

size_t entity_cinecam_serialize(int slot, uint8_t* data) {
    entity_cinecam_t* cinecam_source = (entity_cinecam_t*)entity_get_header(slot);
    size_t cursor = sizeof(vec3_t) * 3;

    // metadata
    *(int32_t*)(data + cursor) = cinecam_source->n_nodes; cursor += sizeof(int32_t);
    cursor += serialize_scalar(data + cursor, cinecam_source->progress);;
    *(int32_t*)(data + cursor) = cinecam_source->active; cursor += sizeof(int32_t);
    *(int32_t*)(data + cursor) = cinecam_source->signal_id; cursor += sizeof(int32_t);

    // path
    for (int i = 0; i < cinecam_source->n_nodes; ++i) {
        const cinecam_node_t* source = &cinecam_source->nodes[i];
        cursor += serialize_vec3(data + cursor, source->position);
        cursor += serialize_vec3(data + cursor, source->rotation);
        cursor += serialize_scalar(data + cursor, source->time_to_next);
    }

    return cursor - sizeof(vec3_t) * 3;
}

size_t entity_cinecam_deserialize(int slot, const uint8_t* data) {
    entity_cinecam_t* cinecam_dest = (entity_cinecam_t*)entity_get_header(slot);
    size_t cursor = sizeof(vec3_t) * 3;

    // metadata
    cinecam_dest->n_nodes = *(int32_t*)(data + cursor); cursor += sizeof(int32_t);
    cursor += deserialize_scalar(data + cursor, &cinecam_dest->progress);
    cinecam_dest->active = *(int32_t*)(data + cursor); cursor += sizeof(int32_t);
    cinecam_dest->signal_id = *(int32_t*)(data + cursor); cursor += sizeof(int32_t);

    // path
    #ifdef _LEVEL_EDITOR
	cinecam_dest->nodes = mem_alloc(PATH_CAMERA_MAX_NODES * sizeof(cinecam_node_t), MEM_CAT_UNDEFINED);
	#else
	cinecam_dest->nodes = mem_stack_alloc(cinecam_dest->n_nodes * sizeof(cinecam_node_t), STACK_ENTITY);
	#endif
	for (int i = 0; i < cinecam_dest->n_nodes; ++i) {
        cinecam_node_t* dest = &cinecam_dest->nodes[i];
        cursor += deserialize_vec3(data + cursor, &dest->position);
        cursor += deserialize_vec3(data + cursor, &dest->rotation);
        cursor += deserialize_scalar(data + cursor, &dest->time_to_next);
	}

	cinecam_dest->progress = 0;

    return cursor - sizeof(vec3_t) * 3;
}

scalar_t catmull_rom(scalar_t p0, scalar_t p1, scalar_t p2, scalar_t p3, scalar_t t) {
    p0 = scalar_shift_right(p0, 4);
    p1 = scalar_shift_right(p1, 4);
    p2 = scalar_shift_right(p2, 4);
    p3 = scalar_shift_right(p3, 4);
    scalar_t t2 = scalar_mul(t, t);
    scalar_t t3 = scalar_mul(t2, t);
    scalar_t a = scalar_mul(2 * ONE, p1);
    scalar_t b = scalar_mul(-p0 + p2, t);
    scalar_t c = scalar_mul((2*p0) - (5*p1) + (4*p2) - p3, t2);
    scalar_t d = scalar_mul(-p0 + (3*p1) - (3*p2) + p3, t3);
    return scalar_shift_left((a+b+c+d)/2, 4);
}

vec3_t spline_point(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3, scalar_t t) {
    return (vec3_t) {
        .x = catmull_rom(p0.x, p1.x, p2.x, p3.x, t),
        .y = catmull_rom(p0.y, p1.y, p2.y, p3.y, t),
        .z = catmull_rom(p0.z, p1.z, p2.z, p3.z, t),
    };
}

void entity_cinecam_update(int slot, player_t* player, scalar_t dt, level_t* level) {
    (void)player;
    (void)level;
    entity_cinecam_t* cinecam = (entity_cinecam_t*)entity_get_header(slot);

    if (entity_get_signal(cinecam->signal_id)) {
        cinecam->active = 1;
    }

    if (!cinecam->active) return;

    // interpolate
    int prev_index = scalar_clamp((cinecam->progress / ONE), 0, cinecam->n_nodes - 2);
    int target_index = prev_index + 1;
    scalar_t t = cinecam->progress % ONE;

    #if 0 // linear interpolation
    cinecam->entity_header.position = vec3_lerp(cinecam->nodes[prev_index].position, cinecam->nodes[target_index].position, t);
    cinecam->entity_header.rotation = vec3_lerp(cinecam->nodes[prev_index].rotation, cinecam->nodes[target_index].rotation, t);
    scalar_t time_to_next = scalar_lerp(cinecam->nodes[prev_index].time_to_next, cinecam->nodes[target_index].time_to_next, t);
    #endif

    #if 1 // catmull-rom interpolation
    cinecam_node_t* n0 = &cinecam->nodes[scalar_clamp(prev_index-1, 0, cinecam->n_nodes - 1)];
    cinecam_node_t* n1 = &cinecam->nodes[scalar_clamp(prev_index, 0, cinecam->n_nodes - 1)];
    cinecam_node_t* n2 = &cinecam->nodes[scalar_clamp(target_index, 0, cinecam->n_nodes - 1)];
    cinecam_node_t* n3 = &cinecam->nodes[scalar_clamp(target_index+1, 0, cinecam->n_nodes - 1)];
    cinecam->entity_header.position = spline_point(n0->position, n1->position, n2->position, n3->position, t);
    cinecam->entity_header.rotation = spline_point(n0->rotation, n1->rotation, n2->rotation, n3->rotation, t);
    scalar_t time_to_next = catmull_rom(n0->time_to_next, n1->time_to_next, n2->time_to_next, n3->time_to_next, t);
    #endif

    if (cinecam->n_nodes > 1) cinecam->progress += scalar_div(dt, time_to_next);
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
    (void)slot;
    (void)hitbox_index;
}

void entity_cinecam_player_intersect(int slot, player_t* player) {
    (void)slot;
    (void)player;

}
