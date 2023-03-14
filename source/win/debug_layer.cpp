#include "debug_layer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

void debug_layer_update() {
    // Begin a new frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Update deltatime
    dt_smooth = dt_smooth + ((static_cast<double>(renderer_get_delta_time_ms()) / 1000.) - dt_smooth) * 0.01;

    // Draw debug windows
    ImGui::Begin("DebugInfo");
    ImGui::Text("Frametime: %f ms", dt_smooth);
    ImGui::Text("FPS: %f", 1.0 / dt_smooth);
    ImGui::Text("Triangles: %i\n", renderer_get_n_total_triangles());
    ImGui::Text("n_ray_aabb_intersects: %i\n", n_ray_aabb_intersects);
    ImGui::Text("n_ray_triangle_intersects: %i\n", n_ray_triangle_intersects);
    ImGui::Text("n_sphere_aabb_intersects: %i\n", n_sphere_aabb_intersects);
    ImGui::Text("n_sphere_triangle_intersects: %i\n", n_sphere_triangle_intersects);
    ImGui::Text("n_vertical_cylinder_aabb_intersects: %i\n", n_vertical_cylinder_aabb_intersects);
    ImGui::Text("n_vertical_cylinder_triangle_intersects: %i\n", n_vertical_cylinder_triangle_intersects);
    ImGui::Text("n_ray_aabb_intersects: %i\n", n_ray_aabb_intersects);
    ImGui::Text("n_ray_aabb_intersects: %i\n", n_ray_aabb_intersects);
    ImGui::End();

    // Render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    collision_clear_stats();
}

void debug_layer_close() {
    ImGui::SaveIniSettingsToDisk("imgui_layout.ini");
}