#include "debug_layer.h"

#include <GL/gl3w.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include "../renderer.h"

static double dt_smooth = 0.0f;

void debug_layer_init(GLFWwindow* window) {
    ImGui::CreateContext();
    [[maybe_unused]] ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    ImGui::LoadIniSettingsFromDisk("imgui_layout.ini");
}

void debug_layer_begin() {
    // Begin a new frame
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Clear screen
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void debug_layer_end() {
    // Render
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
}
void debug_layer_update_editor() {

}

#define PI 3.14159265358979f
void debug_layer_manipulate_entity(transform_t* camera, entity_header_t* entity) {
    // Transform to world units
	transform_t render_transform;
    render_transform.position.x = -entity->position.x / COL_SCALE;
    render_transform.position.y = -entity->position.y / COL_SCALE;
    render_transform.position.z = -entity->position.z / COL_SCALE;
    render_transform.rotation.x = -entity->rotation.x;
    render_transform.rotation.y = -entity->rotation.y;
    render_transform.rotation.z = -entity->rotation.z;
	render_transform.scale.x = entity->scale.x;
	render_transform.scale.y = entity->scale.x;
	render_transform.scale.z = entity->scale.x;

    const aabb_t collision_box = {
        .min = vec3_sub(entity->position, entity->mesh->bounds.min),
        .max = vec3_sub(entity->position, entity->mesh->bounds.max)
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

    int flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuizmo::IsOver() || ImGuizmo::IsUsingAny() ? ImGuiWindowFlags_NoMove : 0;

    ImGui::Begin("Viewport", NULL, flags);
    {
        auto wsize = ImVec2(w, h);
        ImGui::SetWindowSize(wsize);
        ImGui::Image((ImTextureID)fb_texture, wsize, ImVec2(0, 1), ImVec2(1, 0));
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        float windowWidth = (float)ImGui::GetWindowWidth();
        float windowHeight = (float)ImGui::GetWindowHeight();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
        mat4 delta;
        
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
            entity->position.x -= (scalar_t)(translation[0] * (float)COL_SCALE);
            entity->position.y -= (scalar_t)(translation[1] * (float)COL_SCALE);
            entity->position.z -= (scalar_t)(translation[2] * (float)COL_SCALE);
        }
        
    }
    ImGui::End();
}

void debug_layer_close() {
    ImGui::SaveIniSettingsToDisk("imgui_layout.ini");
}