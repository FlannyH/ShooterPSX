#include "debug_layer.h"

#include <GL/gl3w.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"
#include <cmath>
#include <vector>
#include <string>

#include "../renderer.h"
#include "../input.h"
#include "../entity.h"
#include "../entities/door.h"
#include "../entities/crate.h"
#include "../entities/pickup.h"

static double dt_smooth = 0.0f;

void debug_layer_init(GLFWwindow* window) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    ImGui::LoadIniSettingsFromDisk("imgui_layout.ini");
}

void debug_layer_begin() {
    // Begin a new frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void debug_layer_end() {
	// Clear screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render to window
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void debug_layer_update_gameplay() {
    // Update deltatime
    dt_smooth = dt_smooth + ((static_cast<double>(renderer_get_delta_time_ms()) / 1000.) - dt_smooth) * 0.01;

    // Draw debug windows
    ImGui::Begin("DebugInfo");
    ImGui::Text("Frametime: %f ms", dt_smooth * 1000);
    ImGui::Text("FPS: %f", 1.0 / dt_smooth);
    ImGui::Text("n_ray_aabb_intersects: %i\n", n_ray_aabb_intersects);
    ImGui::Text("n_ray_triangle_intersects: %i\n", n_ray_triangle_intersects);
    ImGui::Text("n_vertical_cylinder_aabb_intersects: %i\n", n_vertical_cylinder_aabb_intersects);
    ImGui::Text("n_vertical_cylinder_triangle_intersects: %i\n", n_vertical_cylinder_triangle_intersects);
    ImGui::Text("n_ray_aabb_intersects: %i\n", n_ray_aabb_intersects);
    ImGui::Text("n_ray_aabb_intersects: %i\n", n_ray_aabb_intersects);
    ImGui::End();
    collision_clear_stats();
}

#include <cglm/types.h>
#include <cglm/affine.h>
extern "C" {
    extern mat4 perspective_matrix;
    extern mat4 view_matrix;
    extern int w;
    extern int h;
    extern GLuint fb_texture;
    extern GLuint fbo;
}

float scalar_to_float(scalar_t a) {
    return (float)a / (float)ONE;
}
float world_space_to_collision_space(scalar_t a) {
    return (float)a * ((float)COL_SCALE / (float)ONE);
}

void inspect_vec3(vec3_t* vec, const char* label) {
    float vec_float[] =  {
        scalar_to_float(vec->x),
        scalar_to_float(vec->y),
        scalar_to_float(vec->z),
    };

    if (ImGui::DragFloat3(label, vec_float)) {
        *vec = vec3_from_floats(vec_float[0], vec_float[1], vec_float[2]); 
    }
}

size_t inspect_enum(size_t value, const char** names, const char* label) {
    size_t new_value = value;
    if (ImGui::BeginCombo(label, names[value]))
    {
        size_t i = 1;
        while (names[i]) {
            if (ImGui::Selectable(names[i], false)) {
                new_value = i;
            }
            ++i;
        }
        ImGui::EndCombo();
    }
    return new_value;
}

void inspect_entity(size_t entity_id) {
    entity_slot_t* entity_slot = &entity_list[entity_id];
    if (entity_slot->type == ENTITY_NONE) return;

    entity_header_t* entity_data = entity_slot->data;
            
    if (ImGui::TreeNodeEx("Entity Header", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Mesh: %s", entity_data->mesh->name);
        inspect_vec3(&entity_data->position, "Position");
        inspect_vec3(&entity_data->rotation, "Rotation");
        inspect_vec3(&entity_data->scale, "Scale");
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Entity Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (entity_slot->type == ENTITY_DOOR) {
            entity_door_t* door = (entity_door_t*)entity_data;
            inspect_vec3(&door->open_offset, "Open offset");
            bool is_locked = (bool)door->is_locked;
            bool is_big_door = (bool)door->is_big_door;
            bool is_rotated = (bool)door->is_rotated;
            if (ImGui::Checkbox("Locked", &is_locked)) { door->is_locked = (int)is_locked; door->state_changed = 1; }
            if (ImGui::Checkbox("Big door", &is_big_door)) { door->is_big_door = (int)is_big_door; door->state_changed = 1; }
            if (ImGui::Checkbox("Rotated", &is_rotated)) { door->is_rotated = (int)is_rotated; door->state_changed = 1; }
        }
        
        if (entity_slot->type == ENTITY_PICKUP) {
            entity_pickup_t* pickup = (entity_pickup_t*)entity_data;
            const size_t old_type = (size_t)pickup->type;
            const size_t new_type = inspect_enum((size_t)pickup->type, pickup_names, "Pickup type");
            if (old_type != new_type) {
                pickup->type = new_type;
                pickup->entity_header.mesh = NULL; // force refresh mesh data
            }
        }
        
        if (entity_slot->type == ENTITY_CRATE) {
            entity_crate_t* crate = (entity_crate_t*)entity_data;
            crate->pickup_to_spawn = inspect_enum((size_t)crate->pickup_to_spawn, pickup_names, "Pickup type to spawn");
        }
        ImGui::TreePop();
    }

    if (ImGui::Button("Delete")) {
        entity_kill(entity_id);
    }
}

#define PI 3.14159265358979f
void debug_layer_manipulate_entity(transform_t* camera, size_t* selected_entity_slot, int* mouse_over_viewport) {
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // General info
    ImGui::Begin("Info");
    {
        if (ImGui::TreeNodeEx("Camera Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            float vec_float[] =  {
                world_space_to_collision_space(scalar_to_float(-camera->position.x)),
                world_space_to_collision_space(scalar_to_float(-camera->position.y)),
                world_space_to_collision_space(scalar_to_float(-camera->position.z)),
            };

            if (ImGui::DragFloat3("Position", vec_float)) {
                camera->position = vec3_from_floats(-vec_float[0], -vec_float[1], -vec_float[2]); 
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();

    // Entity spawn menu
    ImGui::Begin("Entity spawning");
    {
        static size_t curr_selected_entity_type = 1;
        // Entity select dropdown
        curr_selected_entity_type = inspect_enum(curr_selected_entity_type, entity_names, "Entity type");

        if (ImGui::Button("Spawn")) {
            // Figure out where to spawn - in front of the camera
            vec3_t forward = renderer_get_forward_vector();
            vec3_t spawn_pos = vec3_add(vec3_muls(camera->position, -COL_SCALE), vec3_muls(forward, -80 * ONE));

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
            }
        }
    }
    ImGui::End();

    // Entity inspector menu
    ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_None);
    {
        if (selected_entity_slot) {
            ImGui::Text("Selected entity");
            ImGui::Spacing();
            inspect_entity(*selected_entity_slot);
            
            // If deleted, deselect it
            if (entity_list[*selected_entity_slot].type == ENTITY_NONE) {
                *selected_entity_slot = -1;
            }
        }
    }
    ImGui::End();

    // Viewport with gizmos
    int flags = ImGuizmo::IsOver() || ImGuizmo::IsUsingAny() ? ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove : 0;
    ImGui::Begin("Viewport", NULL, flags);
    {
        // Get normalized mouse position inside the viewport content area
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 content_offset_top_left = ImGui::GetWindowContentRegionMin();
        ImVec2 content_offset_bottom_right = ImGui::GetWindowContentRegionMax();
        ImVec2 rel_mouse_pos = mouse_pos;
        rel_mouse_pos.x -= content_offset_top_left.x + window_pos.x;
        rel_mouse_pos.y -= content_offset_top_left.y + window_pos.y;
        float window_width = content_offset_bottom_right.x - content_offset_top_left.x;
        float window_height = content_offset_bottom_right.y - content_offset_top_left.y;
        float nrm_mouse_x = (rel_mouse_pos.x / window_width) * 2.0 - 1.0;
        float nrm_mouse_y = (rel_mouse_pos.y / window_width) * 2.0 - 1.0;
        w = (int)window_width;
        h = (int)window_height;    

        // Draw the viewport
        auto wsize = ImVec2(w, h);
        ImGui::Image((ImTextureID)fb_texture, wsize, ImVec2(0, 1), ImVec2(1, 0));

        // Handle entity gizmo
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, window_width, window_height);
        mat4 delta;
        
        if (*selected_entity_slot != -1) {
                entity_slot_t* entity_slot = &entity_list[*selected_entity_slot];
                entity_header_t* selected_entity = entity_slot->data;

                // Transform to world units
                transform_t render_transform;
                render_transform.position.x = -selected_entity->position.x / COL_SCALE;
                render_transform.position.y = -selected_entity->position.y / COL_SCALE;
                render_transform.position.z = -selected_entity->position.z / COL_SCALE;
                render_transform.rotation.x = -selected_entity->rotation.x;
                render_transform.rotation.y = -selected_entity->rotation.y;
                render_transform.rotation.z = -selected_entity->rotation.z;
                render_transform.scale.x = selected_entity->scale.x;
                render_transform.scale.y = selected_entity->scale.x;
                render_transform.scale.z = selected_entity->scale.x;

                // Calculate model matrix
                mat4 model_matrix;
                glm_mat4_identity(model_matrix);

                // Apply rotation
                // Apply translation
                // Apply scale
                vec3 position = {
                        (float)render_transform.position.x,
                        (float)render_transform.position.y,
                        (float)render_transform.position.z,
                };
                vec3 scale = {
                        (float)render_transform.scale.x / (float)COL_SCALE,
                        (float)render_transform.scale.y / (float)COL_SCALE,
                        (float)render_transform.scale.z / (float)COL_SCALE,
                };
                glm_translate(model_matrix, position);
                glm_scale(model_matrix, scale);
                glm_rotate_x(model_matrix, (float)render_transform.rotation.x * 2 * PI / 131072.0f, model_matrix);
                glm_rotate_y(model_matrix, (float)render_transform.rotation.y * 2 * PI / 131072.0f, model_matrix);
                glm_rotate_z(model_matrix, (float)render_transform.rotation.z * 2 * PI / 131072.0f, model_matrix);
            if (ImGuizmo::Manipulate(
                &view_matrix[0][0],
                &perspective_matrix[0][0],
                ImGuizmo::TRANSLATE,
                ImGuizmo::LOCAL,
                &model_matrix[0][0],
                &delta[0][0]
            )) {
                vec3 translation, rotation, scale;
                ImGuizmo::DecomposeMatrixToComponents(&delta[0][0], &translation[0], &rotation[0], &scale[0]);
                selected_entity->position.x -= (scalar_t)(translation[0] * (float)COL_SCALE);
                selected_entity->position.y -= (scalar_t)(translation[1] * (float)COL_SCALE);
                selected_entity->position.z -= (scalar_t)(translation[2] * (float)COL_SCALE);
            }
        }
        
        // If the mouse is inside the window, check for entities under the cursor
        if (nrm_mouse_x >= -1.0f 
        && nrm_mouse_x <= 1.0f
        && nrm_mouse_y >= -1.0f
        && nrm_mouse_y <= 1.0f
        ) {
            *mouse_over_viewport = 1;

            if (input_pressed(PAD_L2, 0)) {
                // Read stencil buffer
                uint8_t entity_index = 255;
                glReadPixels((GLint)rel_mouse_pos.x, (GLint)(h-rel_mouse_pos.y), 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &entity_index);
                
                if (!ImGuizmo::IsOver() && !ImGuizmo::IsUsingAny()) {
                    if (entity_index != 255) {
                        *selected_entity_slot = entity_index;
                    }
                    else {
                        *selected_entity_slot = -1;
                    }
                }
            }
        }
        else {
            *mouse_over_viewport = 0;
        }
    }
    ImGui::End();
}

void debug_layer_close() {
    ImGui::SaveIniSettingsToDisk("imgui_layout.ini");
}