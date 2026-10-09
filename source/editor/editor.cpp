#include "editor.h"

#include "../subnivis/main.h"

// todo: figure this crap out
extern "C" {
    int widescreen = 0;
    state_vars_t state;
    state_t current_state = STATE_NONE;
    state_t prev_state = STATE_NONE;
    void set_current_state(state_t state) { current_state = state; }
    state_t get_current_state(void) { return current_state; }
    state_t get_prev_state(void) { return prev_state; }
}

#include "engine/entity.h"
#include "camera.h"
#include "imgui.h"

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <GL/gl3w.h>

#include "engine/texture.h"

#ifdef _LEVEL_EDITOR
#include "entities/platform.h"
#include "entities/trigger.h"
#include "entities/pickup.h"
#include "entities/chaser.h"
#include "entities/crate.h"
#include "entities/door.h"
#include "editor/input_map.h"
#include "engine/input_mapping.h"
#include "engine/renderer.h"
#include "engine/mesh.h"
#include "engine/file.h"

#include <ImGuizmo.h>
#include <imfilebrowser.h>
#include <format>
#endif

// todo(debug_renderer_fix): desc: fix debug renderers for collision, nav, etc

struct editor_state_t {
    bool initialized;
    struct editor_level_state_t {
        char path[256];
        char path_music[256];
        char path_bank[256];
        char path_textures[256];
        char path_collision[256];
        char path_vislist[256];
        char path_model[256];
        char path_model_lod[256];
        char name[256];
        vec3_t player_spawn_position;
        vec3_t player_spawn_rotation;
        level_t lvl;
    } level;
    struct editor_render_state_t {
        bool graphics;
        bool collision;
        bool vislist;
        bool level_bvh;
        bool level_nav_graph;
        bool level_vislist_regions;
        int level_bvh_start_depth;
        int level_bvh_end_depth;
        int hull_build_set_cap;
    } render;
    struct editor_resources_t {
        texture_cpu_t* gizmo_textures;
        model_t* gizmos;
        mesh_t* specialized_meshes[MAX_SHAPE_COUNT];
    } resources;
    struct editor_misc_t {
        player_t player;
        debug_camera_t camera;
        int selected_entity;
        int selected_light;
        int selected_shape;
        bool vertex_selected;
        vec3_t selected_vertex_position;
        int mouse_over_viewport;
        std::vector<size_t> defer_remove_shape;
        ImGuizmo::OPERATION gizmode = ImGuizmo::TRANSLATE;
    } misc;
} editor;

static ImGui::FileBrowser file_dialog(ImGuiFileBrowserFlags_EnterNewFilename);

void debug_layer_init(GLFWwindow* window) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    ImGui::LoadIniSettingsFromDisk("imgui_layout.ini");
}

void debug_layer_begin(void) {
    // Begin a new frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    #ifdef _LEVEL_EDITOR
    ImGuizmo::BeginFrame();
    #endif
}

void debug_layer_end(void) {
	// Clear screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render to window
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#ifdef _LEVEL_EDITOR
#include <cglm/types.h>
#include <cglm/affine.h>
#include "engine/level.h"
#include "engine/file.h"
extern "C" {
    // todo(debug_layer_extern_fb): desc: remove extern from debug layer framebuffer
    extern GLuint fb_texture;
    extern GLuint fbo;
}

// todo(debug_layer_extern_entity_names): desc: maybe dont use extern for entity_names
extern const char* entity_names[];
const char* light_type_names[] = {
    "None",
    "Directional light",
    "Point light",
    NULL
};
const char* shape_type_names[] = {
    "None",
    "Sphere",
    "Capsule (wip)",
    "Triangle (wip)",
    "AABB",
    "Convex Hull",
    NULL
};

void shape_defragment() {
    // find last valid entry
    size_t last_valid = 0;
    for (size_t i = 0; i < MAX_SHAPE_COUNT; ++i) {
        if (editor.level.lvl.shapes[i].type != SHAPE_NONE) last_valid = i;
    }
    editor.level.lvl.n_shapes = last_valid + 1;

    // defragment up to last valid entry
    size_t i = 0;
    while (i < editor.level.lvl.n_shapes) {
        if (editor.level.lvl.shapes[i].type != SHAPE_NONE) {
            ++i;
            continue;
        }

        --editor.level.lvl.n_shapes;
        for (size_t j = i; j < editor.level.lvl.n_shapes; ++j) {
            editor.level.lvl.shapes[j] = editor.level.lvl.shapes[j + 1];
        }
        memset(&editor.level.lvl.shapes[editor.level.lvl.n_shapes], 0, sizeof(shape_t));
    }
}

float scalar_to_float(scalar_t a) {
    return (float)a / (float)ONE;
}

// returns whether data changed
bool inspect_vec3(vec3_t* vec, const char* label) {
    float vec_float[] =  {
        scalar_to_float(vec->x),
        scalar_to_float(vec->y),
        scalar_to_float(vec->z),
    };

    if (ImGui::DragFloat3(label, vec_float)) {
        *vec = vec3_from_floats(vec_float[0], vec_float[1], vec_float[2]);
        return true;
    }
    return false;
}

// returns whether data changed
bool inspect_svec3_4_12(svec3_t* vec, const char* label) {
    float vec_float[] =  {
        (float)vec->x,
        (float)vec->y,
        (float)vec->z,
    };

    if (ImGui::DragFloat3(label, vec_float)) {
        vec->x = (int16_t)vec_float[0];
        vec->y = (int16_t)vec_float[1];
        vec->z = (int16_t)vec_float[2];
        return true;
    }
    return false;
}

// returns whether data changed
bool inspect_scalar(scalar_t* scalar, const char* label) {
    float scalar_float = SCALAR(*scalar);

    if (ImGui::DragFloat(label, &scalar_float)) {
        *scalar = SCALAR(scalar_float);
        return true;
    }
    return false;
}

size_t inspect_enum(size_t value, const char** names, const char* label) {
    size_t new_value = value;
    if (ImGui::BeginCombo(label, names[value]))
    {
        size_t i = 1;
        while (names[i]) {
            if (ImGui::Selectable(names[i], i == value)) {
                new_value = i;
            }
            ++i;
        }
        ImGui::EndCombo();
    }
    return new_value;
}

// returns whether data changed
bool inspect_entity(size_t entity_id) {
    const uint8_t entity_type = entity_get_type(entity_id);
    if (entity_type == ENTITY_NONE) return false;

    entity_header_t* entity_data = entity_get_header(entity_id);

    bool result = false;

    if (ImGui::TreeNodeEx("Entity Header", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (entity_data->mesh) ImGui::Text("Mesh: %s", entity_data->mesh->name);
        result |= inspect_vec3(&entity_data->position, "Position");
        result |= inspect_vec3(&entity_data->rotation, "Rotation");
        result |= inspect_vec3(&entity_data->scale, "Scale");
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Entity Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (entity_type == ENTITY_DOOR) {
            entity_door_t* door = (entity_door_t*)entity_data;
            inspect_vec3(&door->open_offset, "Open offset");
            ImGui::InputInt("Signal ID", &door->signal_id, 0, ENTITY_SIGNAL_COUNT);
            bool is_locked = (bool)door->is_locked;
            bool is_big_door = (bool)door->is_big_door;
            bool is_rotated = (bool)door->is_rotated;
            bool open_by_signal = (bool)door->open_by_signal;
            if (ImGui::Checkbox("Locked", &is_locked)) { door->is_locked = (int)is_locked; door->state_changed = 1; result |= true;}
            if (ImGui::Checkbox("Big door", &is_big_door)) { door->is_big_door = (int)is_big_door; door->state_changed = 1; result |= true;}
            if (ImGui::Checkbox("Rotated", &is_rotated)) { door->is_rotated = (int)is_rotated; door->state_changed = 1; result |= true;}
            if (ImGui::Checkbox("Open by signal", &open_by_signal)) { door->open_by_signal = (int)open_by_signal; door->state_changed = 1; result |= true;}
        }

        else if (entity_type == ENTITY_PICKUP) {
            entity_pickup_t* pickup = (entity_pickup_t*)entity_data;
            const size_t old_type = (size_t)pickup->type;
            const size_t new_type = inspect_enum((size_t)pickup->type, pickup_names, "Pickup type");
            if (old_type != new_type) {
                pickup->type = new_type;
                pickup->entity_header.mesh = NULL; // force refresh mesh data
                result |= 1;
            }
        }

        else if (entity_type == ENTITY_CRATE) {
            entity_crate_t* crate = (entity_crate_t*)entity_data;
            const uint8_t old_val = crate->pickup_to_spawn;
            crate->pickup_to_spawn = inspect_enum((size_t)crate->pickup_to_spawn, pickup_names, "Pickup type to spawn");
            const uint8_t new_val = crate->pickup_to_spawn;
            if (old_val != new_val) {
                result |= 1;
            }
        }

        else if (entity_type == ENTITY_PLATFORM) {
            entity_platform_t* platform = (entity_platform_t*)entity_data;
            result |= inspect_vec3(&platform->position_start, "Start position");
            result |= inspect_vec3(&platform->position_end, "End position");
            result |= inspect_scalar(&platform->velocity, "Velocity");
            result |= ImGui::DragInt("Current timer value (ms)", &platform->curr_timer_value);
            result |= ImGui::DragInt("Auto start to end timer (ms)", &platform->auto_start_timer);
            result |= ImGui::DragInt("Auto end to start timer (ms)", &platform->auto_return_timer);
            result |= ImGui::DragInt("Signal ID", &platform->signal_id);
            bool listen_to_signal = (bool)platform->listen_to_signal;
            bool target_is_end = (bool)platform->target_is_end;
            bool auto_start = (bool)platform->auto_start;
            bool auto_return = (bool)platform->auto_return;
            bool move_on_player_collision = (bool)platform->move_on_player_collision;
            if (ImGui::Checkbox("Listen to signal", &listen_to_signal)) { platform->listen_to_signal = (int)listen_to_signal; result |= 1; }
            if (ImGui::Checkbox("Target is end position", &target_is_end)) { platform->target_is_end = (int)target_is_end; result |= 1; }
            if (ImGui::Checkbox("Start automatically", &auto_start)) { platform->auto_start = (int)auto_start; result |= 1; }
            if (ImGui::Checkbox("Return automatically", &auto_return)) { platform->auto_return = (int)auto_return; result |= 1; }
            if (ImGui::Checkbox("Move on player collision", &move_on_player_collision)) { platform->move_on_player_collision = (int)move_on_player_collision; result |= 1; }

            const aabb_t start_pos_debug = (aabb_t) {
                .min = vec3_sub(platform->position_start, vec3_from_scalar(ONE / 2)),
                .max = vec3_add(platform->position_start, vec3_from_scalar(ONE / 2)),
            };
            const aabb_t end_pos_debug = (aabb_t) {
                .min = vec3_sub(platform->position_end, vec3_from_scalar(ONE / 2)),
                .max = vec3_add(platform->position_end, vec3_from_scalar(ONE / 2)),
            };
            renderer_debug_draw_aabb(&start_pos_debug, red, &id_transform);
            renderer_debug_draw_aabb(&end_pos_debug, green, &id_transform);
            renderer_debug_draw_line(platform->position_start, platform->position_end, blue, &id_transform);
        }

        else if (entity_type == ENTITY_TRIGGER) {
            entity_trigger_t* trigger = (entity_trigger_t*)entity_data;
            bool destroy_on_player_intersect = (bool)trigger->destroy_on_player_intersect;
            if (ImGui::Checkbox("Destroy on player intersect", &destroy_on_player_intersect)) { trigger->destroy_on_player_intersect = (int)destroy_on_player_intersect; }

            trigger->trigger_type = (uint8_t)inspect_enum((int)trigger->trigger_type, entity_trigger_type_names, "Trigger type");
            if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TEXT) {
                float rgb[3] = {
                    (float)trigger->data_text.color.r / 255.f,
                    (float)trigger->data_text.color.g / 255.f,
                    (float)trigger->data_text.color.b / 255.f,
                };
                if (ImGui::ColorEdit3("Text color", rgb)) {
                    trigger->data_text.color.r = (uint8_t)(rgb[0] * 255.f);
                    trigger->data_text.color.g = (uint8_t)(rgb[1] * 255.f);
                    trigger->data_text.color.b = (uint8_t)(rgb[2] * 255.f);
                }
                result |= ImGui::InputInt("Text entry ID", &trigger->data_text.id, 1, 5);
                result |= inspect_scalar(&trigger->data_text.total_display_time, "Display time");
            }
            else if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_SIGNAL) {
                result |= ImGui::InputInt("Signal ID", &trigger->signal.id, 1, 5);
                result |= ImGui::InputInt("Value to send", &trigger->signal.value_to_send, 1, 100);
            }
            else if (trigger->trigger_type == ENTITY_TRIGGER_TYPE_TELEPORT) {
                result |= inspect_vec3(&trigger->teleport.destination_pos, "Destination position");
                result |= inspect_vec3(&trigger->teleport.destination_rotation_offset, "Destination rotation offset");
            }
        }

        ImGui::TreePop();
    }

    if (ImGui::Button("Delete")) {
        entity_kill(entity_id);
        return true;
    }

    // Render its registered bounding boxes
    const size_t n_active_aabb = entity_get_n_active_aabb();
    for (size_t i = 0; i < n_active_aabb; ++i) {
        const entity_collision_box_t* const box = entity_get_aabb_queue_entry(i);
        if (box->entity_index != entity_id) continue;
        const pixel32_t color = (box->is_solid) ? (red) : ((box->is_trigger) ? green : blue);
        renderer_debug_draw_aabb(&box->aabb, color, &id_transform);
    }

    return result;
}

// returns whether data changed
bool inspect_light(size_t light_id) {
    const uint8_t light_type = editor.level.lvl.lights[light_id].type;
    if (light_type == LIGHT_NONE) return false;

    bool result = false;

    inspect_svec3_4_12(&editor.level.lvl.lights[light_id].direction_position, (light_type == LIGHT_DIRECTIONAL)? "Direction" : "Position");
    float intensity = (editor.level.lvl.lights[light_id].intensity) / 256.0f;
    if (ImGui::DragFloat("Intensity", &intensity, 0.05, 0.0f, 127.0f)) {
        editor.level.lvl.lights[light_id].intensity = (int16_t)(intensity * 256.0);
        result |= true;
    }
    float color[3] = {
        ((float)editor.level.lvl.lights[light_id].color_r) / 255.0f,
        ((float)editor.level.lvl.lights[light_id].color_g) / 255.0f,
        ((float)editor.level.lvl.lights[light_id].color_b) / 255.0f,
    };
    if (ImGui::ColorPicker3("Color", color)) {
        editor.level.lvl.lights[light_id].color_r = (color[0] * 255.0f);
        editor.level.lvl.lights[light_id].color_g = (color[1] * 255.0f);
        editor.level.lvl.lights[light_id].color_b = (color[2] * 255.0f);
        result |= true;
    }

    if (ImGui::Button("Delete")) {
        editor.level.lvl.lights[light_id].type = LIGHT_NONE;
        result |= true;
    }

    return result;
}

// returns if the shape changed
bool inspect_shape(size_t shape_id, int& render_hull_build_set_cap) {
    bool result = false;

    const uint8_t shape_type = editor.level.lvl.shapes[shape_id].type;
    if (shape_type == SHAPE_NONE) return false;
    else if (shape_type == SHAPE_SPHERE) {
        inspect_vec3(&editor.level.lvl.shapes[shape_id].sphere.center, "Center");
        ImGui::SameLine();
        if (ImGui::Button("Center to selected vtx")) {
            editor.level.lvl.shapes[shape_id].sphere.center = editor.misc.selected_vertex_position;
            result |= true;
        }

        result |= inspect_scalar(&editor.level.lvl.shapes[shape_id].sphere.radius, "Radius");
        ImGui::SameLine();
        if (ImGui::Button("Fit to selected vtx")) {
            editor.level.lvl.shapes[shape_id].sphere.radius = vec3_magnitude(vec3_sub(editor.misc.selected_vertex_position, editor.level.lvl.shapes[shape_id].sphere.center));
        }
        const aabb_t aabb = {
            .min = vec3_sub(editor.level.lvl.shapes[shape_id].sphere.center, vec3_from_scalar(editor.level.lvl.shapes[shape_id].sphere.radius)),
            .max = vec3_add(editor.level.lvl.shapes[shape_id].sphere.center, vec3_from_scalar(editor.level.lvl.shapes[shape_id].sphere.radius)),
        };
        renderer_debug_draw_aabb(&aabb, {255, 255, 127, 255}, &id_transform);
    }
    else if (shape_type == SHAPE_AABB) {
        inspect_vec3(&editor.level.lvl.shapes[shape_id].aabb.min, "Min");
        ImGui::SameLine();
        if (ImGui::Button("Min to selected vtx")) {
            editor.level.lvl.shapes[shape_id].aabb.min = editor.misc.selected_vertex_position;
            result |= true;
        }

        result |= inspect_vec3(&editor.level.lvl.shapes[shape_id].aabb.max, "Max");
        ImGui::SameLine();
        if (ImGui::Button("Max to selected vtx")) {
            editor.level.lvl.shapes[shape_id].aabb.max = editor.misc.selected_vertex_position;
            result |= true;
        }

        renderer_debug_draw_aabb(&editor.level.lvl.shapes[shape_id].aabb, {255, 255, 127, 255}, &id_transform);
    }
    else if (shape_type == SHAPE_CONVEX_HULL) {
        if (ImGui::TreeNode("Points")) {
            for (size_t i = 0; i < editor.level.lvl.shapes[shape_id].convex_hull.n_points; ++i) {
                ImGui::PushID((int)i);
                if (ImGui::Button("Remove")) {
                    editor.level.lvl.shapes[shape_id].convex_hull.n_points--;
                    for (size_t si = i; si < editor.level.lvl.shapes[shape_id].convex_hull.n_points; ++si) {
                        editor.level.lvl.shapes[shape_id].convex_hull.points[si] = editor.level.lvl.shapes[shape_id].convex_hull.points[si+1];
                    }
                    result |= true;
                }
                ImGui::SameLine();
                if (ImGui::Button("To selected vtx")) {
                    editor.level.lvl.shapes[shape_id].convex_hull.points[i] = editor.misc.selected_vertex_position;
                    result |= true;
                }
                ImGui::SameLine();
                if (inspect_vec3(&editor.level.lvl.shapes[shape_id].convex_hull.points[i], "Pos")) {
                    mem_free(editor.resources.specialized_meshes[shape_id]);
                    editor.resources.specialized_meshes[shape_id] = create_convex_hull_from_point_cloud(
                        editor.level.lvl.shapes[shape_id].convex_hull.points,
                        editor.level.lvl.shapes[shape_id].convex_hull.n_points,
                        render_hull_build_set_cap
                    );
                    result |= true;
                }
                ImGui::PopID();
            }
            ImGui::TreePop();

            mem_free(editor.resources.specialized_meshes[shape_id]);
            editor.resources.specialized_meshes[shape_id] = create_convex_hull_from_point_cloud(
                editor.level.lvl.shapes[shape_id].convex_hull.points,
                editor.level.lvl.shapes[shape_id].convex_hull.n_points,
                render_hull_build_set_cap
            );
            result |= true;
        }

        const vec3_t initial_point = editor.level.lvl.shapes[shape_id].convex_hull.points[0];
        aabb_t aabb = (aabb_t){initial_point, initial_point};
        for (size_t i = 1; i < editor.level.lvl.shapes[shape_id].convex_hull.n_points; ++i) {
            const vec3_t point = editor.level.lvl.shapes[shape_id].convex_hull.points[i];

            // expand aabb
            aabb.min = vec3_min(aabb.min, point);
            aabb.max = vec3_max(aabb.max, point);

            // render point
            transform_t trans = {
                .position = point,
                .rotation = vec3_from_scalar(0),
                .scale = vec3_from_scalar(SCALAR(16.0 / 1024)), // 1024 because the model is scaled by 1024 for precision
            };
            renderer_set_drawing_id(0, 0);
            renderer_draw_mesh_shaded(&editor.resources.gizmos->meshes[2], &trans, 0, 0);
        }

        if (editor.resources.specialized_meshes[shape_id]) {
            for (size_t i = 0; i < editor.resources.specialized_meshes[shape_id]->n_triangles; ++i) {
                assert(editor.resources.specialized_meshes[shape_id]->vertices != NULL);
                assert(editor.resources.specialized_meshes[shape_id]->normals != NULL);
                const vertex_3d_t* vertices = &editor.resources.specialized_meshes[shape_id]->vertices[3*i + 0];
                const vec3_t a = vec3_from_svec3(*(svec3_t*)&vertices[0]);
                const vec3_t b = vec3_from_svec3(*(svec3_t*)&vertices[1]);
                const vec3_t c = vec3_from_svec3(*(svec3_t*)&vertices[2]);
                const vec3_t ab = vec3_sub(b, a);
                const vec3_t ac = vec3_sub(c, a);
                const vec3_t center = vec3_add(a, vec3_divs(vec3_add(ab, ac), SCALAR(3.0)));
                const vec3_t normal = (vec3_t) {
                    .x = (scalar_t)editor.resources.specialized_meshes[shape_id]->normals[3*i].x * (ONE / 127),
                    .y = (scalar_t)editor.resources.specialized_meshes[shape_id]->normals[3*i].y * (ONE / 127),
                    .z = (scalar_t)editor.resources.specialized_meshes[shape_id]->normals[3*i].z * (ONE / 127),
                };
                // todo: was i gonna plan something here?
            }
        }

        renderer_debug_draw_aabb(&aabb, {255, 255, 127, 255}, &id_transform);

        if (ImGui::Button("Add point") || input_mapping_pressed(IM_SHAPE_ADD_POINT, 0)) {
            size_t index = editor.level.lvl.shapes[shape_id].convex_hull.n_points++;
            editor.level.lvl.shapes[shape_id].convex_hull.points[index] = editor.misc.selected_vertex_position;
        }
        if (ImGui::Button("Delete")) {
            editor.misc.defer_remove_shape.push_back(shape_id);
        }
    }

    return result;
}

void draw_texture_category(const char* name, texture_category_t category, bool show_detail) {
    const float cell_width = 80 + (show_detail * 152);
    int items_per_row = ImGui::GetContentRegionAvail().x / cell_width;
    if (items_per_row < 1) items_per_row = 1;
    int column = 0;
    if (ImGui::TreeNode(name)) {
        for (size_t i = 0; i < MAX_TEXTURE_COUNT; ++i) {
            texture_entry_t* entry = renderer_get_texture_entry(category, i);
            if (!entry->allocated) continue;

            float u0 = entry->pool_offset_u;
            float v0 = entry->pool_offset_v;
            float u1 = u0 + entry->width;
            float v1 = v0 + entry->height;
            u0 /= 2048;
            u1 /= 2048;
            v0 /= 2048;
            v1 /= 2048;

            if ((column++ % items_per_row) != 0) {
                ImGui::SameLine();
            }
            ImGui::Image(
                reinterpret_cast<ImTextureID>(renderer_debug_fetch_atlas()),
                ImVec2(64, 64),
                ImVec2(u0, v0),
                ImVec2(u1, v1)
            );

            if (show_detail) {
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("Resolution: %i x %i", entry->width ? entry->width : 256, entry->height ? entry->height : 256);
                ImGui::Text("Pool offset: (%i, %i)", entry->pool_offset_u, entry->pool_offset_v);
                ImGui::Text("Pool %i", entry->texture_pool_id);
                ImGui::Text("Entry %i", entry->texture_entry_id);
                ImGui::EndGroup();
            }
        }
    ImGui::TreePop();
    }
}

#define PI 3.14159265358979f

void render_3d_debug_stuff() {
    if (editor.render.graphics) {
        renderer_draw_model_shaded(editor.level.lvl.graphics, &editor.level.lvl.transform, NULL);
    }

    // todo(editor_render_level_bvh): desc: render level bvh and nav graph in the editor
    // if (render_level_bvh) bvh_debug_draw(&editor.level.lvl.collision_bvh, render_level_bvh_start_depth, render_level_bvh_end_depth, (pixel32_t){ .r = 160, .g = 240, .b = 80, .a = 255 });
    // if (render_level_nav_graph) bvh_debug_draw_nav_graph(&editor.level.lvl.collision_bvh);

    if (editor.render.vislist) {
        uint32_t node_stack[2048] = {0};
        uint32_t node_handle_ptr = 0;
        uint32_t node_add_ptr = 1;
        while (node_handle_ptr != node_add_ptr) {
            // check a node
            visbvh_node_t* node = &editor.level.lvl.vislist.bvh_root[node_stack[node_handle_ptr]];

            aabb_t aabb;
            aabb.min = vec3_shift_right(vec3_from_svec3(node->min), 3);
            aabb.max = vec3_shift_right(vec3_from_svec3(node->max), 3);

            // If the node is an interior node
            if ((node->child_or_vis_index & 0x80000000) == 0) {
                // Add the 2 children to the stack
                node_stack[node_add_ptr] = node->child_or_vis_index;
                node_add_ptr = (node_add_ptr + 1) % 2048;
                node_stack[node_add_ptr] = node->child_or_vis_index + 1;
                node_add_ptr = (node_add_ptr + 1) % 2048;
            }
            else {
                // Draw
                const static transform_t trans = { {0,0,0},{0,0,0}, {ONE, ONE, ONE} };
                renderer_debug_draw_aabb(&aabb, green, &trans);
            }

            node_handle_ptr = (node_handle_ptr + 1) % 2048;
        }
    }

    // Draw sphere at selected vertex to show it's selected
    if (editor.misc.vertex_selected) {
        transform_t trans = {
            .position = editor.misc.selected_vertex_position,
            .rotation = vec3_from_scalar(0),
            .scale = vec3_from_scalar(SCALAR(16.0 / 1024)), // 1024 because the model is scaled by 1024 for precision
        };
        renderer_set_drawing_id(0, 0);
        renderer_draw_mesh_shaded(&editor.resources.gizmos->meshes[2], &trans, 0, 0);
    }
}

void level_metadata() {
    ImGui::Begin("Level Metadata");
    auto load = []() {
        mem_debug();
        // todo: how is this gonna work? we'll find out
        if (editor.level.lvl.graphics) {
            for (uint32_t i = 0; i < editor.level.lvl.graphics->n_meshes; ++i) {
                mem_free(editor.level.lvl.graphics->meshes[i].vertices);
            }
        }
        if (editor.level.lvl.collision_mesh_debug) {
            if (editor.level.lvl.collision_mesh_debug->meshes) {
                for (uint32_t i = 0; i < editor.level.lvl.collision_mesh_debug->n_meshes; ++i) {
                    mem_free(editor.level.lvl.collision_mesh_debug->meshes[i].vertices);
                }
                mem_free(editor.level.lvl.collision_mesh_debug->meshes);
            }
            mem_free(editor.level.lvl.collision_mesh_debug);
        }
        mem_free(editor.level.lvl.lights);
        mem_debug();

        uint32_t* data;
        size_t size;
        if (file_read(editor.level.path, &data, &size, 1, STACK_TEMP) == 0) {
            printf("Failed to load level '%s'\n", editor.level.path);
        }
        level_header_t* header = (level_header_t*)data;
        char* binary_section = (char*)(&header[1]);

        strncpy(editor.level.path_music, binary_section + header->path_music_offset, 255);
        strncpy(editor.level.path_bank, binary_section + header->path_bank_offset, 255);
        strncpy(editor.level.path_textures, binary_section + header->path_texture_offset, 255);
        strncpy(editor.level.path_collision, binary_section + header->path_collision_offset, 255);
        strncpy(editor.level.path_vislist, binary_section + header->path_vislist_offset, 255);
        strncpy(editor.level.path_model, binary_section + header->path_model_offset, 255);
        strncpy(editor.level.path_model_lod, binary_section + header->path_model_lod_offset, 255);
        strncpy(editor.level.name, binary_section + header->level_name_offset, 255);

        entity_init();
        editor.level.lvl = level_load(editor.level.path, LEVEL_LOAD_ALL);
        editor.level.player_spawn_position = vec3_from_svec3(editor.level.lvl.player_spawn_position);
        editor.level.player_spawn_rotation = editor.level.lvl.player_spawn_rotation;
        player_init(&editor.misc.player, editor.level.player_spawn_position, editor.level.player_spawn_rotation, 40, 0, 0);
        editor.misc.camera.transform.position = editor.level.player_spawn_position;
        editor.misc.camera.transform.rotation = editor.level.player_spawn_rotation;
    };

    auto save = []() {
        std::vector<uint8_t> binary_section;

        auto write_text_and_get_offset = [](std::vector<uint8_t>& output, char* string) {
            const auto string_offset_in_file = output.size();
            intptr_t offset = 0;
            do {
                output.push_back(string[offset]);
            } while (string[offset++] != 0);
            return string_offset_in_file;
        };

        auto write_data_and_get_offset = [](std::vector<uint8_t>& output, const void* data, size_t size_in_bytes) {
            const uint8_t* ptr = (uint8_t*)data;

            // align to 4 bytes
            while ((output.size() % 4) != 0) output.push_back(0);

            const auto offset_in_file = output.size();
            uintptr_t offset = 0;
            if (data) {
                do {
                    output.push_back(ptr[offset]);
                } while (++offset < size_in_bytes);
            }
            return offset_in_file;
        };

        // Construct level header and write into binary section as we go along
        entity_defragment(); // make entity allocations contiguous and compact so we can save space...
        const int n_entities = entity_how_many_active(); // ...and reuse this function

        // Serialize the entities, removing all pointers in the process
        std::vector<uint8_t> entity_data_serialized;
        for (intptr_t i = 0; i < n_entities; ++i) {
            // Get header
            const entity_header_t header = *entity_get_header(i);

            // Write entity header
            write_data_and_get_offset(entity_data_serialized, &header.position, sizeof(vec3_t));
            write_data_and_get_offset(entity_data_serialized, &header.rotation, sizeof(vec3_t));
            write_data_and_get_offset(entity_data_serialized, &header.scale, sizeof(vec3_t));

            // Write entity data
            const uint8_t* entity_data = ((const uint8_t*)entity_get_header(i)) + sizeof(entity_header_t);
            const size_t size = entity_get_pool_stride() - sizeof(entity_header_t);
            write_data_and_get_offset(entity_data_serialized, entity_data, size);
        }

        // Serialize text
        std::vector<uint8_t> text_data_serialized;
        for (int i = 0; i < editor.level.lvl.n_text_entries; ++i) {
            int n_chars = 0;
            while (editor.level.lvl.text_entries[i][n_chars] != 0 && editor.level.lvl.text_entries[i][n_chars] != 127) {
                ++n_chars;
            }
            text_data_serialized.push_back((uint8_t)n_chars);
            for (int j = 0; j < n_chars; ++j) {
                text_data_serialized.push_back(*(uint8_t*)&editor.level.lvl.text_entries[i][j]);
            }
        }

        const size_t n_entities_padded = (n_entities + 3) & ~0x03;
        const size_t n_extra_values = n_entities_padded - n_entities;
        uint8_t* entity_types = (uint8_t*)mem_alloc(n_entities_padded, MEM_CAT_UNDEFINED);
        for (int i = 0; i < n_entities; ++i) {
            entity_types[i] = entity_get_type(i);
        }
        for (size_t i = 0; i < n_extra_values; ++i) {
            entity_types[i + n_entities] = 0;
        }

        light_t lights[MAX_LIGHT_COUNT];
        memset(lights, 0, sizeof(lights));

        int n_lights = 0;
        for (int i = 0; i < MAX_LIGHT_COUNT; ++i) {
            if (editor.level.lvl.lights[i].type != LIGHT_NONE) {
                lights[n_lights++] = editor.level.lvl.lights[i];
            }
        }

        // serialize shapes
        uint8_t* shapes = (uint8_t*)mem_alloc(1 * MiB, MEM_CAT_UNDEFINED); // 1 MB should be overkill
        size_t shape_cursor = 0;
        size_t n_shapes = 0;
        for (int i = 0; i < MAX_SHAPE_COUNT; ++i) {
            if (editor.level.lvl.shapes[i].type == SHAPE_NONE) continue;
            serialize_shape(shapes, &shape_cursor, &editor.level.lvl.shapes[i]);
            ++n_shapes;
        }

        level_header_t header = {
            .file_magic = MAGIC_FLVL,
            .path_music_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_music),
            .path_bank_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_bank),
            .path_texture_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_textures),
            .path_collision_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_collision),
            .path_vislist_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_vislist),
            .path_model_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_model),
            .path_model_lod_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.path_model_lod),
            .entity_types_offset = (uint32_t)write_data_and_get_offset(binary_section, entity_types, (n_entities + 3) & ~0x03), // 4-byte padding
            .entity_pool_offset = (uint32_t)write_data_and_get_offset(binary_section, entity_data_serialized.data(), entity_data_serialized.size() * sizeof(entity_data_serialized[0])),
            .light_data_offset =  (uint32_t)write_data_and_get_offset(binary_section, lights, n_lights * sizeof(light_t)),
            .shape_data_offset =  (uint32_t)write_data_and_get_offset(binary_section, shapes, shape_cursor),
            .level_name_offset = (uint32_t)write_text_and_get_offset(binary_section, editor.level.name),
            .text_offset = (uint32_t)write_data_and_get_offset(binary_section, text_data_serialized.data(), text_data_serialized.size()),
            .n_text_entries = (uint32_t)editor.level.lvl.n_text_entries,
            .player_spawn_position = svec3_from_vec3(editor.level.player_spawn_position),
            .player_spawn_rotation = editor.level.player_spawn_rotation,
            .n_entities = (uint16_t)n_entities,
            .n_lights = (uint16_t)n_lights,
            .n_shapes = (uint16_t)n_shapes,
        };

        mem_free(entity_types);

        FILE* file = fopen(editor.level.path, "wb");
        if (file == nullptr) {
            // todo(error_dialog): desc: message boxes for errors?
            printf("Error saving file '%s': could not open file\n", editor.level.path);
            return;
        }
        fwrite(&header, sizeof(header), 1, file);
        fwrite(binary_section.data(), sizeof(binary_section[0]), binary_section.size(), file);
        fclose(file);
    };

    ImGui::SeparatorText("Level File");
    ImGui::InputText("Level File Path", editor.level.path, 255);

    // Browse button
    ImGui::SameLine();
    if (ImGui::Button("...")) {
        file_dialog.SetTitle("Open level file");
        file_dialog.SetTypeFilters({ ".lvl" });
        file_dialog.Open();
    }
    file_dialog.Display();

    static bool load_after_select = false;
    static bool save_after_select = false;

    if (file_dialog.HasSelected()) {
        strcpy(editor.level.path, file_dialog.GetSelected().string().c_str());
        file_dialog.ClearSelected();

        if (load_after_select) load();
        if (save_after_select) save();

        load_after_select = false;
        save_after_select = false;
    }

    // Load button
    if (ImGui::Button("Load")) {
        // If the level path is empty, open file dialog
        if (editor.level.path[0] == '\0' || editor.level.path[0] == ' ') {
            load_after_select = true;
            file_dialog.SetTitle("Open level file");
            file_dialog.SetTypeFilters({ ".lvl" });
            file_dialog.Open();
        }
        else {
            load();
        }
    }

    // Save button
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        // If the level path is empty, open file dialog
        if (editor.level.path[0] == '\0' || editor.level.path[0] == ' ') {
            save_after_select = true;
            file_dialog.SetTitle("Open level file");
            file_dialog.SetTypeFilters({ ".lvl" });
            file_dialog.Open();
        }
        else {
            save();
        }
    }

    // Save as button
    ImGui::SameLine();
    if (ImGui::Button("Save as")) {
        // Open file dialog
        save_after_select = true;
        file_dialog.SetTitle("Open level file");
        file_dialog.SetTypeFilters({ ".lvl" });
        file_dialog.Open();
    }

    ImGui::SeparatorText("Level Header");
    ImGui::InputText("Music Sequence Path", editor.level.path_music, 255);
    ImGui::InputText("Music Soundbank Path", editor.level.path_bank, 255);
    ImGui::InputText("Texture Collection Path", editor.level.path_textures, 255);
    ImGui::InputText("Collision Path", editor.level.path_collision, 255);
    ImGui::InputText("Visility List Path", editor.level.path_vislist, 255);
    ImGui::InputText("Model Path", editor.level.path_model, 255);
    ImGui::InputText("Model LOD Path", editor.level.path_model_lod, 255);
    ImGui::InputText("Level Name", editor.level.name, 255);
    inspect_vec3(&editor.level.player_spawn_position, "Player Spawn Position");
    inspect_vec3(&editor.level.player_spawn_rotation, "Player Spawn Rotation");

    if (ImGui::Button("Hot reload")) {
        // todo(debug_level_load_reuse_code): desc: reuse level_load() in hot reload
        mem_stack_release(STACK_TEMP);
        mem_stack_release(STACK_LEVEL);
        mem_stack_release(STACK_ENTITY);
        entity_init();

        // Load level textures
        texture_cpu_t *tex_level;
        auto n_level_textures = texture_collection_load(editor.level.path_textures, &tex_level, 1, STACK_TEMP);
        for (uint8_t i = 0; i < n_level_textures; ++i) {
            renderer_upload_texture(&tex_level[i], i, TEX_CAT_LEVEL);
        }

        // Load graphics and collision data
        editor.level.lvl.graphics = model_load(editor.level.path_model, 1, STACK_LEVEL, TEX_CAT_LEVEL, 1);
        editor.level.lvl.collision_mesh_debug = model_load_collision_debug(editor.level.path_collision, 0, (stack_t)0);
        editor.level.lvl.collision_mesh = model_load_collision(editor.level.path_collision, 1, STACK_LEVEL);
        editor.level.lvl.transform = { {0, 0, 0}, {0, 0, 0}, {ONE, ONE, ONE} };
        editor.level.lvl.vislist = vislist_load(editor.level.path_vislist, 1, STACK_LEVEL);

        // todo(editor_collision_bvh_load): desc: update editor to new collision system
        // editor.level.lvl.collision_bvh = bvh_from_file(path_collision, 1, STACK_LEVEL);
        // memset(&editor.level.collision_bvh, 0, sizeof(editor.level.collision_bvh));

        editor.misc.player.transform.position = editor.level.player_spawn_position;
        editor.misc.player.transform.rotation = editor.level.player_spawn_rotation;
        editor.misc.camera.transform.position = editor.level.player_spawn_position;
        editor.misc.camera.transform.rotation = editor.level.player_spawn_position;
        player_update(&editor.misc.player, &editor.level.lvl, 0, 0); // Tick the player with 0 delta time to update the camera transform
    }
    ImGui::End();
}

void initialize() {
    if (editor.initialized) return;

    memset(&editor, 0, sizeof(editor));

    editor.resources.gizmos = model_load("editor/gizmos.msh", 0, (stack_t)0, TEX_CAT_PERSISTENT, 0);
    uint32_t n_tex = texture_collection_load("editor/gizmos.txc", &editor.resources.gizmo_textures, 1, STACK_TEMP);
    for (uint32_t i = 0; i < n_tex; ++i) {
        renderer_upload_texture(&editor.resources.gizmo_textures[i], i,  TEX_CAT_PERSISTENT);
    }

    editor.level.lvl.lights = (light_t*)mem_alloc(256 * sizeof(light_t), MEM_CAT_UNDEFINED);
    editor.level.lvl.shapes = (shape_t*)mem_alloc(256 * sizeof(shape_t), MEM_CAT_UNDEFINED);
    memset(editor.level.lvl.lights, 0, 256 * sizeof(light_t));
    memset(editor.level.lvl.shapes, 0, 256 * sizeof(shape_t));

    editor.render.graphics = 1;
    editor.misc.selected_entity = -1;
    editor.misc.selected_light = -1;
    editor.misc.selected_shape = -1;
    editor.misc.gizmode = ImGuizmo::TRANSLATE;
    editor.initialized = true;
}

void cleanup() {
    if (!editor.misc.defer_remove_shape.empty()) {
        // remove deferred shapes
        for (const size_t shape_id : editor.misc.defer_remove_shape) {
            memset(&editor.level.lvl.shapes[shape_id], 0, sizeof(shape_t));
        }
        editor.misc.defer_remove_shape.clear();

        shape_defragment();
    }
}

void general_info() {
    // General info
    ImGui::Begin("Info");
    if (ImGui::TreeNodeEx("Camera Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        float vec_float[] =  {
            scalar_to_float(editor.misc.camera.transform.position.x),
            scalar_to_float(editor.misc.camera.transform.position.y),
            scalar_to_float(editor.misc.camera.transform.position.z),
        };

        if (ImGui::DragFloat3("Position", vec_float)) {
            editor.misc.camera.transform.position = vec3_from_floats(vec_float[0], vec_float[1], vec_float[2]);
        }
        inspect_vec3(&editor.misc.camera.transform.rotation, "Rotation");
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Debug Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Render level graphics", &editor.render.graphics);
        ImGui::Checkbox("Render level collision", &editor.render.collision);
        ImGui::Checkbox("Render level BVH", &editor.render.level_bvh);
        ImGui::Checkbox("Render level vislist regions", &editor.render.level_vislist_regions);
        ImGui::Checkbox("Render Level navgraph", &editor.render.level_nav_graph);
        if (ImGui::Button("-")) editor.render.hull_build_set_cap--;
        ImGui::SameLine();
        if (ImGui::Button("+")) editor.render.hull_build_set_cap++;
        ImGui::SameLine();
        ImGui::DragInt("Hull build step cap", &editor.render.hull_build_set_cap);
        ImGui::DragInt("BVH min level", &editor.render.level_bvh_start_depth);
        ImGui::DragInt("BVH max level", &editor.render.level_bvh_end_depth);
        ImGui::TreePop();
    }
    ImGui::End();
}

void entity_stuff() {
    // Entity spawn menu
    ImGui::Begin("Entity spawning");
    // Entity count:
    int entity_count = 0;
    for (size_t i = 0; i < ENTITY_LIST_LENGTH; ++i) {
        if (entity_get_type(i) != ENTITY_NONE) {
            ++entity_count;
        }
    }

    ImGui::Text("Entities: %i / %i", entity_count, ENTITY_LIST_LENGTH);

    // Entity select dropdown
    static size_t curr_selected_entity_type = 1;
    curr_selected_entity_type = inspect_enum(curr_selected_entity_type, entity_names, "Entity type");

    if (ImGui::Button("Spawn")) {
        // Figure out where to spawn - in front of the camera
        const vec3_t forward = renderer_get_forward_vector();
        const vec3_t spawn_pos = vec3_add((editor.misc.vertex_selected) ? (editor.misc.selected_vertex_position) : (editor.misc.camera.transform.position), vec3_muls(forward, 80 * ONE));

        entity_header_t* entity;

        switch (curr_selected_entity_type) {
            case ENTITY_DOOR:
                entity = (entity_header_t*)entity_door_new();
                entity->position = spawn_pos;
                break;
            case ENTITY_PICKUP:
                entity = (entity_header_t*)entity_pickup_new();
                entity->position = spawn_pos;
                break;
            case ENTITY_CRATE:
                entity = (entity_header_t*)entity_crate_new();
                entity->position = spawn_pos;
                break;
            case ENTITY_CHASER:
                entity = (entity_header_t*)entity_chaser_new();
                entity->position = spawn_pos;
                break;
            case ENTITY_PLATFORM:
                entity = (entity_header_t*)entity_platform_new();
                entity->position = spawn_pos;
                ((entity_platform_t*)entity)->position_start = spawn_pos;
                ((entity_platform_t*)entity)->position_end = vec3_add(spawn_pos, vec3_from_scalars(0, ONE * 16, 0));
                break;
            case ENTITY_TRIGGER:
                entity = (entity_header_t*)entity_trigger_new();
                entity->position = spawn_pos;
                break;
        }
    }

    if (ImGui::Button("Defragment")) {
        entity_defragment();
    }
    ImGui::End();

    // Entity inspector menu
    ImGui::Begin("Entity Inspector", NULL, ImGuiWindowFlags_None);
    {
        if (editor.misc.selected_entity >= 0) {
            ImGui::Text("Selected entity");
            ImGui::Spacing();
            inspect_entity(editor.misc.selected_entity);

            // If deleted, deselect it
            if (entity_get_type(editor.misc.selected_entity) == ENTITY_NONE) {
                editor.misc.selected_entity = -1;
            }
        }
        ImGui::Spacing();
        if (ImGui::TreeNode("All entities")) {
            for (size_t i = 0; i < ENTITY_LIST_LENGTH; ++i) {
                if (entity_get_type((int)i) != ENTITY_NONE) {
                    static std::string tree_nodes[ENTITY_LIST_LENGTH];
                    tree_nodes[i] = std::format("{} - {}", i, entity_names[entity_get_type((int)i)]);
                    if (ImGui::TreeNode(tree_nodes[i].c_str())) {
                        inspect_entity(i);
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void light_stuff() {
    // Light spawn menu
    ImGui::Begin("Light spawning");
    int light_count = 0;
    for (size_t i = 0; i < MAX_LIGHT_COUNT; ++i) {
        if (editor.level.lvl.lights[i].type != LIGHT_NONE) {
            ++light_count;
        }
    }

    ImGui::Text("Lights: %i / %i", light_count, MAX_LIGHT_COUNT);

    // Light select dropdown
    static size_t curr_selected_light_type = 1;
    curr_selected_light_type = inspect_enum(curr_selected_light_type, light_type_names, "Light type");

    if (ImGui::Button("Spawn")) {
        // Figure out where to spawn - in front of the camera
        const vec3_t forward = renderer_get_forward_vector();
        const vec3_t spawn_pos = vec3_add((editor.misc.vertex_selected) ? (editor.misc.selected_vertex_position) : (editor.misc.camera.transform.position), vec3_muls(forward, 80 * ONE));

        for (size_t i = 0; i < MAX_LIGHT_COUNT; ++i) {
            if (editor.level.lvl.lights[i].type == LIGHT_NONE) {
                if (curr_selected_light_type == LIGHT_DIRECTIONAL) {
                    editor.level.lvl.lights[i].direction_position = { (int16_t)forward.x, (int16_t)forward.y, (int16_t)forward.z };
                }
                else if (curr_selected_light_type == LIGHT_POINT) {
                    editor.level.lvl.lights[i].direction_position = svec3_from_vec3(spawn_pos);
                }

                editor.level.lvl.lights[i].intensity = 1 << 8; // 1.0
                editor.level.lvl.lights[i].color_r = 255;
                editor.level.lvl.lights[i].color_g = 255;
                editor.level.lvl.lights[i].color_b = 255;
                editor.level.lvl.lights[i].type = (uint8_t)curr_selected_light_type;
                break;
            }
        }
    }

    if (ImGui::Button("Defragment")) {
        // todo(debug_light_defragment): desc: light_defragment();
    }
    ImGui::End();

        // Light inspector menu
    ImGui::Begin("Light Inspector", NULL, ImGuiWindowFlags_None);
    if (editor.misc.selected_light >= 0) {
        ImGui::Text("Selected light");
        ImGui::Spacing();
        inspect_light(editor.misc.selected_light);

        // If deleted, deselect it
        if (editor.level.lvl.lights[editor.misc.selected_light].type == LIGHT_NONE) {
            editor.misc.selected_light = -1;
        }
    }
    ImGui::Spacing();
    if (ImGui::TreeNode("All lights")) {
        for (size_t i = 0; i < MAX_LIGHT_COUNT && editor.level.lvl.lights; ++i) {
            if (editor.level.lvl.lights[i].type != LIGHT_NONE) {
                static std::string tree_nodes[MAX_LIGHT_COUNT];
                tree_nodes[i] = std::format("{} - {}", i, light_type_names[editor.level.lvl.lights[i].type]);
                if (ImGui::TreeNode(tree_nodes[i].c_str())) {
                    inspect_light(i);
                    ImGui::TreePop();
                }
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();

    for (int i = 0; i < MAX_LIGHT_COUNT && editor.level.lvl.lights; ++i) {
        if (editor.level.lvl.lights[i].type == LIGHT_POINT) {
            transform_t trans = {
                .position = vec3_from_svec3(editor.level.lvl.lights[i].direction_position),
                .rotation = vec3_from_scalar(0),
                .scale = vec3_from_scalar(ONE),
            };
            if ((int)i == editor.misc.selected_light) {
                aabb_t aabb = {
                    .min = vec3_sub(trans.position, vec3_from_scalar(6*ONE)),
                    .max = vec3_add(trans.position, vec3_from_scalar(6*ONE)),
                };
                renderer_debug_draw_aabb(&aabb, red, &id_transform);
            }
            renderer_set_drawing_id(i, 2);
            renderer_draw_mesh_shaded(&editor.resources.gizmos->meshes[(size_t)(editor.level.lvl.lights[i].type-1)], &trans, 0, 1);
        }
    }
    renderer_update_lights(editor.level.lvl.lights);
}

void collision_stuff() {
    ImGui::Begin("Collision spawning");
    int shape_count = 0;
    for (size_t i = 0; i < MAX_SHAPE_COUNT; ++i) {
        if (!editor.level.lvl.shapes) break;
        if (editor.level.lvl.shapes[i].type != SHAPE_NONE) {
            ++shape_count;
        }
    }

    ImGui::Text("Shapes: %i / %i", shape_count, MAX_SHAPE_COUNT);

    // Shape select dropdown
    static size_t curr_selected_shape_type = 1;
    curr_selected_shape_type = inspect_enum(curr_selected_shape_type, shape_type_names, "Shape type");

    if (ImGui::Button("Spawn")) {
        // Figure out where to spawn - in front of the camera
        const vec3_t forward = renderer_get_forward_vector();
        const vec3_t spawn_pos = vec3_add((editor.misc.vertex_selected) ? (editor.misc.selected_vertex_position) : (editor.misc.camera.transform.position), vec3_muls(forward, 80 * ONE));

        for (size_t i = 0; i < MAX_SHAPE_COUNT; ++i) {
            if (editor.level.lvl.shapes[i].type == SHAPE_NONE) {
                if (curr_selected_shape_type == SHAPE_SPHERE) {
                    editor.level.lvl.shapes[i].sphere.center = spawn_pos;
                    editor.level.lvl.shapes[i].sphere.radius = SCALAR(100.0);
                }
                else if (curr_selected_shape_type == SHAPE_AABB) {
                    editor.level.lvl.shapes[i].aabb.min = spawn_pos;
                    editor.level.lvl.shapes[i].aabb.max = vec3_add(spawn_pos, vec3_from_scalar(SCALAR(250.0)));
                }
                else if (curr_selected_shape_type == SHAPE_CONVEX_HULL) {
                    editor.level.lvl.shapes[i].convex_hull.points = (vec3_t*)mem_alloc(256 * sizeof(vec3_t), MEM_CAT_MESH);
                    editor.level.lvl.shapes[i].convex_hull.n_points = 0;
                }
                editor.level.lvl.shapes[i].type = curr_selected_shape_type;
                break;
            }
        }

        shape_defragment();
    }

    if (ImGui::Button("Defragment")) {
        shape_defragment();
    }
    ImGui::End();

    // Collision inspector menu
    ImGui::Begin("Collision Inspector", NULL, ImGuiWindowFlags_None);
    if (editor.misc.selected_shape >= 0) {
        ImGui::Text("Selected shape");
        ImGui::Spacing();
        inspect_shape(editor.misc.selected_shape, editor.render.hull_build_set_cap);

        // If deleted, deselect it
        if (!editor.level.lvl.shapes || editor.level.lvl.shapes[editor.misc.selected_shape].type == SHAPE_NONE) {
            editor.misc.selected_shape = -1;
        }
    }
    ImGui::Spacing();
    if (ImGui::TreeNode("All shapes")) {
        for (int i = 0; i < editor.level.lvl.n_shapes && editor.level.lvl.shapes; ++i) {
            if (editor.level.lvl.shapes[i].type == SHAPE_NONE) continue;

            static std::string tree_nodes[MAX_SHAPE_COUNT];
            tree_nodes[i] = std::format("{} - {}", i, shape_type_names[editor.level.lvl.shapes[i].type]);
            if (ImGui::TreeNode(tree_nodes[i].c_str())) {
                inspect_shape(i, editor.render.hull_build_set_cap);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::End();

    static int counter = 0;
    counter += 1;

    if (editor.render.collision) {
        for (int i = 0; i < MAX_SHAPE_COUNT && editor.level.lvl.shapes; ++i) {
            if (editor.level.lvl.shapes[i].type == SHAPE_SPHERE) {
                const transform_t trans = {
                    .position = editor.level.lvl.shapes[i].sphere.center,
                    .rotation = vec3_from_scalar(0),
                    .scale = vec3_from_scalar(editor.level.lvl.shapes[i].sphere.radius / 1024), // 1024 because the model is scaled by 1024 for precision
                };

                renderer_set_drawing_id(i, 3);
                renderer_draw_mesh_shaded(&editor.resources.gizmos->meshes[2], &trans, 0, 0);
            }
            else if (editor.level.lvl.shapes[i].type == SHAPE_AABB) {
                const vec3_t min = editor.level.lvl.shapes[i].aabb.min;
                const vec3_t max = editor.level.lvl.shapes[i].aabb.max;
                const vec3_t size = vec3_sub(max, min);
                transform_t trans = {
                    .position = min,
                    .rotation = vec3_from_scalar(0),
                    .scale = {size.x / -1024, size.y / 1024, size.z / -1024}, // 1024 because the model is scaled by 1024 for precision
                };

                renderer_set_drawing_id(i, 3);
                renderer_draw_mesh_shaded(&editor.resources.gizmos->meshes[3], &trans, 0, 0);
            }
            else if (editor.level.lvl.shapes[i].type == SHAPE_CONVEX_HULL) {
                renderer_set_drawing_id(i, 3);
                renderer_draw_mesh_shaded(editor.resources.specialized_meshes[i], &id_transform, 0, 0);
            }
        }
    }

    if (editor.level.lvl.n_shapes > 0) {
        static int lazy_update_convex_hulls = 0;
        lazy_update_convex_hulls++;
        lazy_update_convex_hulls %= editor.level.lvl.n_shapes;
        if (editor.level.lvl.shapes[lazy_update_convex_hulls].type == SHAPE_CONVEX_HULL) {
            mem_free(editor.resources.specialized_meshes[lazy_update_convex_hulls]);
            editor.resources.specialized_meshes[lazy_update_convex_hulls] = create_convex_hull_from_point_cloud(
                editor.level.lvl.shapes[lazy_update_convex_hulls].convex_hull.points,
                editor.level.lvl.shapes[lazy_update_convex_hulls].convex_hull.n_points,
                editor.render.hull_build_set_cap
            );
        }
    }
}

void text_editor() {
    // Text editor window
    ImGui::Begin("Text editor", NULL, ImGuiWindowFlags_None);
    if (ImGui::Button("Add")) {
        bool found = false;
        for (int i = 0; i < editor.level.lvl.n_text_entries; ++i) {
            if (editor.level.lvl.text_entries[i][0] == 127) {
                editor.level.lvl.text_entries[i][0] = 0;
                found = true;
                break;
            }
        }

        if (!found) {
            editor.level.lvl.text_entries = (char**)realloc(editor.level.lvl.text_entries, (editor.level.lvl.n_text_entries + 1) * sizeof(char**));
            editor.level.lvl.text_entries[editor.level.lvl.n_text_entries] = (char*)mem_alloc(255, MEM_CAT_UNDEFINED);
            editor.level.lvl.text_entries[editor.level.lvl.n_text_entries][0] = 0;
            ++editor.level.lvl.n_text_entries;
        }
    }

    for (int i = 0; i < editor.level.lvl.n_text_entries; ++i) {
        if (editor.level.lvl.text_entries[i][0] == 127) continue;

        if (ImGui::TreeNode(std::format("{}", i).c_str())) {
            ImGui::PushID(i);
            ImGui::InputTextMultiline("", editor.level.lvl.text_entries[i], 255);
            if (ImGui::Button("Delete")) {
                editor.level.lvl.text_entries[i][0] = 127;
            }
            ImGui::PopID();
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void texture_viewer() {
    ImGui::Begin("Texture viewer", NULL, ImGuiWindowFlags_None);
    static bool show_detail = false;
    ImGui::Checkbox("View allocation details", &show_detail);
    draw_texture_category("Level textures", TEX_CAT_LEVEL, show_detail);
    draw_texture_category("Entity textures", TEX_CAT_ENTITY, show_detail);
    draw_texture_category("Weapon textures", TEX_CAT_WEAPON, show_detail);
    draw_texture_category("Misc textures", TEX_CAT_MISC, show_detail);
    draw_texture_category("Persistent textures", TEX_CAT_PERSISTENT, show_detail);
    if (ImGui::TreeNode("Texture Atlas")) {
        auto avail = ImGui::GetContentRegionAvail().x;
        ImGui::Image(
            reinterpret_cast<ImTextureID>(renderer_debug_fetch_atlas()),
            ImVec2(avail, avail)
        );
        ImGui::TreePop();
    }
    ImGui::End();
}

void viewport() {
    int flags = ImGuizmo::IsOver() || ImGuizmo::IsUsingAny() ? ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove : 0;
    ImGui::Begin("Viewport", NULL, flags);
    {
        // Get normalized mouse position inside the viewport content area
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        const ImVec2 window_pos = ImGui::GetWindowPos();
        const ImVec2 content_offset_top_left = ImGui::GetWindowContentRegionMin();
        ImVec2 content_offset_bottom_right = ImGui::GetWindowContentRegionMax();
        content_offset_bottom_right.y -= ImGui::GetFrameHeightWithSpacing() * 1.0f;
        ImVec2 rel_mouse_pos = mouse_pos;
        rel_mouse_pos.x -= content_offset_top_left.x + window_pos.x;
        rel_mouse_pos.y -= content_offset_top_left.y + window_pos.y;
        const float window_width = content_offset_bottom_right.x - content_offset_top_left.x;
        const float window_height = content_offset_bottom_right.y - content_offset_top_left.y;
        const float nrm_mouse_x = (rel_mouse_pos.x / window_width) * 2.0 - 1.0;
        const float nrm_mouse_y = (rel_mouse_pos.y / window_height) * 2.0 - 1.0;
        renderer_update_window_res((int)window_width, (int)window_height);

        // Draw the viewport
        const auto wsize = ImVec2(renderer_width(), renderer_height());
        ImGui::Image((ImTextureID)(intptr_t)fb_texture, wsize, ImVec2(0, 1), ImVec2(1, 0));

        static int selected = 0;
        ImGui::RadioButton("Translate", &selected, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Rotate", &selected, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Scale", &selected, 2);
        if (selected == 0) editor.misc.gizmode = ImGuizmo::TRANSLATE;
        else if (selected == 1) editor.misc.gizmode = ImGuizmo::ROTATE;
        else if (selected == 2) editor.misc.gizmode = ImGuizmo::SCALE;

        // Handle entity gizmo
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, window_width, window_height);
        mat4 delta;

        bool has_selected_entity = (editor.misc.selected_entity != -1);
        bool has_selected_light = (editor.misc.selected_light != -1);
        bool has_selected_shape = (editor.misc.selected_shape != -1);

        bool light_has_position = false;
        if (has_selected_light) {
            if (editor.level.lvl.lights[editor.misc.selected_light].type == LIGHT_POINT) light_has_position = true;
        }

        if (has_selected_entity || (has_selected_light && light_has_position) || has_selected_shape) {
            // Transform to world units
            transform_t render_transform{{0, 0, 0}, {0, 0, 0}, {ONE, ONE, ONE}};

            if (has_selected_entity) {
                entity_header_t* selected_entity = entity_get_header(editor.misc.selected_entity);
                render_transform.position = selected_entity->position;
                render_transform.rotation = selected_entity->rotation;
                render_transform.scale = selected_entity->scale;
            }
            else if (has_selected_light) {
                light_t light = editor.level.lvl.lights[editor.misc.selected_light];
                render_transform.position = vec3_from_svec3(light.direction_position);
            }
            else if (has_selected_shape) {
                shape_t shape = editor.level.lvl.shapes[editor.misc.selected_shape];
                if (shape.type == SHAPE_SPHERE) {
                    render_transform.position = shape.sphere.center;
                    render_transform.scale = vec3_from_scalar(shape.sphere.radius);
                }
                if (shape.type == SHAPE_AABB) {
                    // todo: move nearest corner?
                    vec3_t size = vec3_sub(shape.aabb.max, shape.aabb.min);
                    if (size.x < 0) size.x = -size.x;
                    if (size.y < 0) size.y = -size.y;
                    if (size.z < 0) size.z = -size.z;
                    render_transform.position = vec3_add(shape.aabb.min, vec3_shift_right(size, 1));
                    render_transform.scale = size;
                }
            }

            // Calculate model matrix
            mat4 model_matrix;
            glm_mat4_identity(model_matrix);

            // Apply rotation
            // Apply translation
            // Apply scale
            vec3 position = {
                (float)render_transform.position.x / (float)ONE,
                (float)render_transform.position.y / (float)ONE,
                (float)render_transform.position.z / (float)ONE,
            };
            vec3 scale = {
                (float)render_transform.scale.x / (float)ONE,
                (float)render_transform.scale.y / (float)ONE,
                (float)render_transform.scale.z / (float)ONE,
            };
            glm_translate(model_matrix, position);
            glm_scale(model_matrix, scale);
            glm_rotate_z(model_matrix, ((float)render_transform.rotation.z * 2 * PI) / ONE, model_matrix);
            glm_rotate_y(model_matrix, ((float)render_transform.rotation.y * 2 * PI) / ONE, model_matrix);
            glm_rotate_x(model_matrix, ((float)render_transform.rotation.x * 2 * PI) / ONE, model_matrix);

            if (ImGuizmo::Manipulate(
                renderer_debug_view_matrix(),
                renderer_debug_perspective_matrix(),
                editor.misc.gizmode,
                ImGuizmo::WORLD,
                &model_matrix[0][0],
                &delta[0][0]
            )) {
                vec3 translation{}, rotation{}, scale{};
                ImGuizmo::DecomposeMatrixToComponents(&delta[0][0], &translation[0], &rotation[0], &scale[0]);

                printf("t: %3.3f, %3.3f, %3.3f\t", translation[0], translation[1], translation[2]);
                printf("r: %3.3f, %3.3f, %3.3f\t", rotation[0], rotation[1], rotation[2]);
                printf("s: %3.3f, %3.3f, %3.3f\n\n", scale[0], scale[1], scale[2]);

                if (has_selected_entity) {
                    entity_header_t* selected_entity = entity_get_header(editor.misc.selected_entity);
                    if (editor.misc.gizmode == ImGuizmo::TRANSLATE) {
                        selected_entity->position.x += SCALAR(translation[0]);
                        selected_entity->position.y += SCALAR(translation[1]);
                        selected_entity->position.z += SCALAR(translation[2]);
                    }
                    if (editor.misc.gizmode == ImGuizmo::ROTATE) {
                        selected_entity->rotation.x += SCALAR(rotation[0] / 360.0f);
                        selected_entity->rotation.y += SCALAR(rotation[1] / 360.0f);
                        selected_entity->rotation.z += SCALAR(rotation[2] / 360.0f);
                    }
                    if (editor.misc.gizmode == ImGuizmo::SCALE) {
                        selected_entity->scale.x = scalar_mul(selected_entity->scale.x, SCALAR(scale[0]));
                        selected_entity->scale.y = scalar_mul(selected_entity->scale.y, SCALAR(scale[1]));
                        selected_entity->scale.z = scalar_mul(selected_entity->scale.z, SCALAR(scale[2]));
                    }
                }
                if (has_selected_light) {
                    light_t* selected_light = &editor.level.lvl.lights[editor.misc.selected_light];
                    if (editor.misc.gizmode == ImGuizmo::TRANSLATE) {
                        selected_light->direction_position.x += translation[0];
                        selected_light->direction_position.y += translation[1];
                        selected_light->direction_position.z += translation[2];
                    }
                }
                if (has_selected_shape) {
                    shape_t* selected_shape = &editor.level.lvl.shapes[editor.misc.selected_shape];
                    if (selected_shape->type == SHAPE_SPHERE) {
                        if (editor.misc.gizmode == ImGuizmo::TRANSLATE) {
                            selected_shape->sphere.center.x += SCALAR(translation[0]);
                            selected_shape->sphere.center.y += SCALAR(translation[1]);
                            selected_shape->sphere.center.z += SCALAR(translation[2]);
                        }
                    }
                    else if (selected_shape->type == SHAPE_AABB) {
                        if (editor.misc.gizmode == ImGuizmo::TRANSLATE) {
                            selected_shape->aabb.min.x += SCALAR(translation[0]);
                            selected_shape->aabb.min.y += SCALAR(translation[1]);
                            selected_shape->aabb.min.z += SCALAR(translation[2]);
                            selected_shape->aabb.max.x += SCALAR(translation[0]);
                            selected_shape->aabb.max.y += SCALAR(translation[1]);
                            selected_shape->aabb.max.z += SCALAR(translation[2]);
                        }
                        if (editor.misc.gizmode == ImGuizmo::SCALE) {
                            if ((scale[0] != 1.0f) || (scale[1] != 1.0f) || (scale[2] != 1.0f)) {
                                vec3_t initial_size = vec3_sub(selected_shape->aabb.max, selected_shape->aabb.min);
                                const vec3_t center = vec3_add(selected_shape->aabb.min, vec3_shift_right(initial_size, 1));
                                if (initial_size.x < 0) initial_size.x *= -1;
                                if (initial_size.y < 0) initial_size.y *= -1;
                                if (initial_size.z < 0) initial_size.z *= -1;
                                vec3_t new_size = vec3_add(initial_size, vec3_from_scalars(
                                    SCALAR(translation[0]),
                                    SCALAR(translation[1]),
                                    SCALAR(translation[2])
                                ));
                                selected_shape->aabb.min = vec3_sub(center, vec3_shift_right(new_size, 1));
                                selected_shape->aabb.max = vec3_add(center, vec3_shift_right(new_size, 1));
                            }
                        }
                    }
                }
            }
        }

        // If the mouse is inside the window, check for entities under the cursor
        if (nrm_mouse_x >= -1.0f
        && nrm_mouse_x <= 1.0f
        && nrm_mouse_y >= -1.0f
        && nrm_mouse_y <= 1.0f
        ) {
            editor.misc.mouse_over_viewport = 1;

            if (input_mapping_pressed(IM_PICK, 0)) {
                // Read picking buffer
                struct {
                    uint8_t index;
                    uint8_t what;
                    uint16_t padding;
                } pick_info;
                glReadBuffer(GL_COLOR_ATTACHMENT1);
                glReadPixels((GLint)rel_mouse_pos.x, (GLint)(renderer_height() - rel_mouse_pos.y), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pick_info);

                if (!ImGuizmo::IsOver() && !ImGuizmo::IsUsingAny()) {
                    editor.misc.selected_light = -1;
                    editor.misc.selected_shape = -1;
                    editor.misc.selected_entity = -1;
                    editor.misc.vertex_selected = false;

                    if (pick_info.what == 1) editor.misc.selected_entity = pick_info.index;
                    else if (pick_info.what == 2) editor.misc.selected_light = pick_info.index;
                    else if (pick_info.what == 3) editor.misc.selected_shape = pick_info.index;
                    else if (pick_info.what == 4) {
                        float vertex_pos_float[3] = {0};
                        glReadBuffer(GL_COLOR_ATTACHMENT2);
                        glReadPixels((GLint)rel_mouse_pos.x, (GLint)(renderer_height() - rel_mouse_pos.y), 1, 1, GL_RGB, GL_FLOAT, &vertex_pos_float);
                        editor.misc.vertex_selected = true;
                        editor.misc.selected_vertex_position = vec3_from_floats(vertex_pos_float[0], vertex_pos_float[1], vertex_pos_float[2]);
                    }
                }
            }
        }
        else {
            editor.misc.mouse_over_viewport = 0;
        }
    }
    ImGui::End();
}

/// doc: desc: Entry point for the level editor.
/// doc: desc: It initializes the engine systems, spawns a player and a debug camera, and then enters the main editor loop.
int main(int argc, char** argv) {
    if      (argc == 1) {
        std::filesystem::current_path("./assets/");
    }
    else if (argc == 2) {
        std::filesystem::current_path(argv[1]);
    }
    else {
        printf("Usage: level_editor.exe [assets_folder]\n");
        printf("Default assets folder is \"./assets/\"\n");
        exit(1);
    }

    mem_init();
    renderer_init();
    input_init();
    input_mapping_init();
	entity_init();
    input_set_gamepad_stick_deadzone(SCALAR(36.0/255));
    input_mapping_register_mouse(IM_CAMERA_LOCK, INPUT_MOUSE_BUTTON_RIGHT, SCALAR(1.0));
    input_mapping_register_keyboard(IM_CAMERA_DOWN, INPUT_KEY_LEFT_SHIFT, SCALAR(1.0));
    input_mapping_register_keyboard(IM_CAMERA_UP, INPUT_KEY_SPACE, SCALAR(1.0));
    input_mapping_register_keyboard(IM_MOVE_X, INPUT_KEY_A, SCALAR(-1.0));
    input_mapping_register_keyboard(IM_MOVE_X, INPUT_KEY_D, SCALAR(+1.0));
    input_mapping_register_keyboard(IM_MOVE_Y, INPUT_KEY_S, SCALAR(-1.0));
    input_mapping_register_keyboard(IM_MOVE_Y, INPUT_KEY_W, SCALAR(1.0));
    input_mapping_register_mouse(IM_LOOK_X, INPUT_MOUSE_DELTA_X, SCALAR(0.5));
    input_mapping_register_mouse(IM_LOOK_Y, INPUT_MOUSE_DELTA_Y, SCALAR(0.5));
    input_mapping_register_keyboard(IM_CAMERA_SPEED_UP, INPUT_KEY_EQUALS, SCALAR(1.0));
    input_mapping_register_keyboard(IM_CAMERA_SPEED_DOWN, INPUT_KEY_MINUS, SCALAR(1.0));
    input_mapping_register_mouse(IM_PICK, INPUT_MOUSE_BUTTON_LEFT, SCALAR(1.0));
    input_mapping_register_keyboard(IM_SHAPE_ADD_POINT, INPUT_KEY_P, SCALAR(1.0));

    initialize();

    while (!renderer_should_close()) {
        debug_layer_begin();
        renderer_begin_frame(&editor.misc.camera.transform);

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        cleanup();
        render_3d_debug_stuff();
        level_metadata();
        general_info();
        entity_stuff();
        light_stuff();
        collision_stuff();
        text_editor();
        texture_viewer();
        viewport();

        renderer_end_frame();
        debug_layer_end();
    }
}

#endif
void debug_layer_close(void) {
    ImGui::SaveIniSettingsToDisk("imgui_layout.ini");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
