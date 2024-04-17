#include "debug_layer.h"

#include <GL/gl3w.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"
#include <cmath>

#include "../renderer.h"
#include "../input.h"

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

#define PI 3.14159265358979f
void debug_layer_manipulate_entity(transform_t* camera, entity_header_t** selected_entity, int* mouse_over_viewport) {
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // Entity spawn menu
    ImGui::Begin("Entity spawning", NULL, ImGuiWindowFlags_None);
    {

    }
    ImGui::End();

    // Entity inspector menu
    ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_None);
    {

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
        
        if (*selected_entity) {
                // Transform to world units
                transform_t render_transform;
                render_transform.position.x = -(*selected_entity)->position.x / COL_SCALE;
                render_transform.position.y = -(*selected_entity)->position.y / COL_SCALE;
                render_transform.position.z = -(*selected_entity)->position.z / COL_SCALE;
                render_transform.rotation.x = -(*selected_entity)->rotation.x;
                render_transform.rotation.y = -(*selected_entity)->rotation.y;
                render_transform.rotation.z = -(*selected_entity)->rotation.z;
                render_transform.scale.x = (*selected_entity)->scale.x;
                render_transform.scale.y = (*selected_entity)->scale.x;
                render_transform.scale.z = (*selected_entity)->scale.x;

                const aabb_t collision_box = {
                    .min = vec3_sub((*selected_entity)->position, (*selected_entity)->mesh->bounds.min),
                    .max = vec3_sub((*selected_entity)->position, (*selected_entity)->mesh->bounds.max)
                };

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
                (*selected_entity)->position.x -= (scalar_t)(translation[0] * (float)COL_SCALE);
                (*selected_entity)->position.y -= (scalar_t)(translation[1] * (float)COL_SCALE);
                (*selected_entity)->position.z -= (scalar_t)(translation[2] * (float)COL_SCALE);
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
                        *selected_entity = entity_list[(size_t)entity_index].data;
                    }
                    else {
                        *selected_entity = NULL;
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