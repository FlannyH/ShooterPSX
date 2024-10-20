#include "debug_layer.h"

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <GL/gl3w.h>

#include "../entity.h"
#include "imgui.h"

#ifdef _LEVEL_EDITOR
#include <ImGuizmo.h>
#include <imfilebrowser.h>
#include <format>
#include "../entities/pickup.h"
#include "../entities/chaser.h"
#include "../entities/crate.h"
#include "../entities/door.h"
#include "../renderer.h"
#include "../input.h"
#include "../file.h"
#endif

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

void debug_layer_update_gameplay(void) {
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

#ifdef _LEVEL_EDITOR
#include <cglm/types.h>
#include <cglm/affine.h>
#include <level.h>
#include <file.h>
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
    uint8_t entity_type = entity_types[entity_id];
    if (entity_type == ENTITY_NONE) return;

    entity_header_t* entity_data = (entity_header_t*)&entity_pool[entity_id * entity_pool_stride];
            
    if (ImGui::TreeNodeEx("Entity Header", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (entity_data->mesh) ImGui::Text("Mesh: %s", entity_data->mesh->name);
        inspect_vec3(&entity_data->position, "Position");
        inspect_vec3(&entity_data->rotation, "Rotation");
        inspect_vec3(&entity_data->scale, "Scale");
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Entity Data", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (entity_type == ENTITY_DOOR) {
            entity_door_t* door = (entity_door_t*)entity_data;
            inspect_vec3(&door->open_offset, "Open offset");
            bool is_locked = (bool)door->is_locked;
            bool is_big_door = (bool)door->is_big_door;
            bool is_rotated = (bool)door->is_rotated;
            if (ImGui::Checkbox("Locked", &is_locked)) { door->is_locked = (int)is_locked; door->state_changed = 1; }
            if (ImGui::Checkbox("Big door", &is_big_door)) { door->is_big_door = (int)is_big_door; door->state_changed = 1; }
            if (ImGui::Checkbox("Rotated", &is_rotated)) { door->is_rotated = (int)is_rotated; door->state_changed = 1; }
        }
        
        if (entity_type == ENTITY_PICKUP) {
            entity_pickup_t* pickup = (entity_pickup_t*)entity_data;
            const size_t old_type = (size_t)pickup->type;
            const size_t new_type = inspect_enum((size_t)pickup->type, pickup_names, "Pickup type");
            if (old_type != new_type) {
                pickup->type = new_type;
                pickup->entity_header.mesh = NULL; // force refresh mesh data
            }
        }
        
        if (entity_type == ENTITY_CRATE) {
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
void debug_layer_manipulate_entity(transform_t* camera, int* selected_entity_slot, int* mouse_over_viewport, level_t* curr_level, player_t* player) {
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    static ImGui::FileBrowser file_dialog(ImGuiFileBrowserFlags_EnterNewFilename);

    // Level metadata
    static vec3_t player_spawn_position = (vec3_t){ 0, 0, 0 };
    static vec3_t player_spawn_rotation = (vec3_t){ 0, 0, 0 };
    static char* level_path = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_music = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_bank = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_texture = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_collision = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_vislist = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_model = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* path_model_lod = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static char* level_name = (char*)mem_alloc(256, MEM_CAT_UNDEFINED);
    static bool initialized = false;

    // Debug state
    static bool render_level_graphics = true;
    static bool render_level_collision = false;
    static bool render_level_bvh = false;
    static bool render_level_nav_graph = false;
    static bool render_level_vislist_regions = false;
    static int render_level_bvh_start_depth = 0;
    static int render_level_bvh_end_depth = 6;
    
    if (render_level_graphics) renderer_draw_model_shaded(curr_level->graphics, &curr_level->transform, NULL, 0);
    if (render_level_collision) renderer_draw_model_shaded(curr_level->collision_mesh_debug, &id_transform, NULL, 0);
    if (render_level_bvh) bvh_debug_draw(&curr_level->collision_bvh, render_level_bvh_start_depth, render_level_bvh_end_depth, (pixel32_t){ .r = 160, .g = 240, .b = 80, .a = 255 });
    if (render_level_nav_graph) bvh_debug_draw_nav_graph(&curr_level->collision_bvh);
    if (render_level_vislist_regions) {
        uint32_t node_stack[2048] = {0};
        uint32_t node_handle_ptr = 0;
        uint32_t node_add_ptr = 1;
        // printf("draw");
        while (node_handle_ptr != node_add_ptr) {
            // check a node
            visbvh_node_t* node = &curr_level->vislist.bvh_root[node_stack[node_handle_ptr]];
            
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
                const static transform_t trans = { {0,0,0},{0,0,0}, {4096, 4096, 4096} };
                renderer_debug_draw_aabb(&aabb, green, &trans);
            }

            node_handle_ptr = (node_handle_ptr + 1) % 2048;
        }
    }


    if (!initialized) {
        level_path[0] = 0;
        path_music[0] = 0;
        path_bank[0] = 0;
        path_texture[0] = 0;
        path_collision[0] = 0;
        path_vislist[0] = 0;
        path_model[0] = 0;
        path_model_lod[0] = 0;
        level_name[0] = 0;
        initialized = true;
    }
    ImGui::Begin("Level Metadata");
    {
        auto load = [curr_level, player]() {
            uint32_t* data;
            size_t size;
            file_read(level_path, &data, &size, 1, STACK_TEMP);
            level_header_t* header = (level_header_t*)data;
            char* binary_section = (char*)(&header[1]);
            strcpy(path_music, binary_section + header->path_music_offset);
            strcpy(path_bank, binary_section + header->path_bank_offset);
            strcpy(path_texture, binary_section + header->path_texture_offset);
            strcpy(path_collision, binary_section + header->path_collision_offset);
            strcpy(path_vislist, binary_section + header->path_vislist_offset);
            strcpy(path_model, binary_section + header->path_model_offset);
            strcpy(path_model_lod, binary_section + header->path_model_lod_offset);
            strcpy(level_name, binary_section + header->level_name_offset);

            *curr_level = level_load(level_path);
            player_spawn_position = vec3_from_svec3(curr_level->player_spawn_position);
            player_spawn_rotation = curr_level->player_spawn_rotation;
            player->position = player_spawn_position;
            player->rotation = player_spawn_rotation;
            player_update(player, &curr_level->collision_bvh, 0, 0); // Tick the player with 0 delta time to update the camera transform
        };

        auto save = [curr_level]() {
            std::vector<uint8_t> binary_section;

            auto write_text_and_get_offset = [](std::vector<uint8_t>& output, char* string) {
                auto string_offset_in_file = output.size();
                intptr_t offset = 0;
                do {
                    output.push_back(string[offset]);
                } while (string[offset++] != 0);
                return string_offset_in_file;
            };

            auto write_data_and_get_offset = [](std::vector<uint8_t>& output, const void* data, size_t size_in_bytes) {
                uint8_t* ptr = (uint8_t*)data;
                
                // align to 4 bytes
                while ((output.size() % 4) != 0) output.push_back(0);

                auto offset_in_file = output.size();
                uintptr_t offset = 0;
                do {
                    output.push_back(ptr[offset]);
                } while (++offset < size_in_bytes);
                return offset_in_file;
            };

            // Construct level header and write into binary section as we go along
            entity_defragment(); // make entity allocations contiguous and compact so we can save space...
            int n_entities = entity_how_many_active(); // ...and reuse this function

            // Serialize the entities, removing all pointers in the process
            std::vector<uint8_t> entity_data_serialized;
            for (intptr_t i = 0; i < n_entities; ++i) {
                // Get header
                const entity_header_t header = *((entity_header_t*)(entity_pool + (entity_pool_stride * i)));

                // Write entity header
                write_data_and_get_offset(entity_data_serialized, &header.position, sizeof(vec3_t));
                write_data_and_get_offset(entity_data_serialized, &header.rotation, sizeof(vec3_t));
                write_data_and_get_offset(entity_data_serialized, &header.scale, sizeof(vec3_t));

                // Write entity data
                const uint8_t* entity_data = entity_pool + (entity_pool_stride * i) + sizeof(entity_header_t);
                const size_t size = entity_pool_stride - sizeof(entity_header_t);
                write_data_and_get_offset(entity_data_serialized, entity_data, size);
            }

            level_header_t header = {
                .file_magic = MAGIC_FLVL,
                .path_music_offset = (uint32_t)write_text_and_get_offset(binary_section, path_music),
                .path_bank_offset = (uint32_t)write_text_and_get_offset(binary_section, path_bank),
                .path_texture_offset = (uint32_t)write_text_and_get_offset(binary_section, path_texture),
                .path_collision_offset = (uint32_t)write_text_and_get_offset(binary_section, path_collision),
                .path_vislist_offset = (uint32_t)write_text_and_get_offset(binary_section, path_vislist),
                .path_model_offset = (uint32_t)write_text_and_get_offset(binary_section, path_model),
                .path_model_lod_offset = (uint32_t)write_text_and_get_offset(binary_section, path_model_lod),
                .entity_types_offset = (uint32_t)write_data_and_get_offset(binary_section, entity_types, (n_entities + 3) & ~0x03), // 4-byte padding
                .entity_pool_offset = (uint32_t)write_data_and_get_offset(binary_section, entity_data_serialized.data(), entity_data_serialized.size() * sizeof(entity_data_serialized[0])),
                .level_name_offset = (uint32_t)write_text_and_get_offset(binary_section, level_name),
                .player_spawn_position = svec3_from_vec3(player_spawn_position),
                .player_spawn_rotation = player_spawn_rotation,
                .n_entities = (uint16_t)n_entities,
            };

            FILE* file = fopen(level_path, "wb");
            fwrite(&header, sizeof(header), 1, file);
            fwrite(binary_section.data(), sizeof(binary_section[0]), binary_section.size(), file);
            fclose(file);
        };

        ImGui::SeparatorText("Level File");
        ImGui::InputText("Level File Path", level_path, 255);

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
            strcpy(level_path, file_dialog.GetSelected().string().c_str());
            file_dialog.ClearSelected();

            if (load_after_select) load();
            if (save_after_select) save();

            load_after_select = false;
            save_after_select = false;
        }

        // Load button
        if (ImGui::Button("Load")) {
            // If the level path is empty
            if (level_path[0] == '\0' || level_path[0] == ' ') {
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
            // If the level path is empty
            if (level_path[0] == '0' || level_path[0] == ' ') {
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
            save_after_select = true;
            file_dialog.SetTitle("Open level file");
            file_dialog.SetTypeFilters({ ".lvl" });
            file_dialog.Open();
        }

        ImGui::SeparatorText("Level Header");
        ImGui::InputText("Music Sequence Path", path_music, 255);
        ImGui::InputText("Music Soundbank Path", path_bank, 255);
        ImGui::InputText("Texture Collection Path", path_texture, 255);
        ImGui::InputText("Collision Path", path_collision, 255);
        ImGui::InputText("Visility List Path", path_vislist, 255);
        ImGui::InputText("Model Path", path_model, 255);
        ImGui::InputText("Model LOD Path", path_model_lod, 255);
        ImGui::InputText("Level Name", level_name, 255);
        inspect_vec3(&player_spawn_position, "Player Spawn Position");
        inspect_vec3(&player_spawn_rotation, "Player Spawn Rotation");

        if (ImGui::Button("Hot reload")) {
            mem_stack_release(STACK_TEMP);
            mem_stack_release(STACK_LEVEL);
            mem_stack_release(STACK_ENTITY);
            entity_init();

            // Load level textures
            texture_cpu_t* tex_level;
            tex_level_start = tex_alloc_cursor;
            const uint32_t n_level_textures = texture_collection_load(path_texture, &tex_level, 1, STACK_TEMP);
            for (uint8_t i = 0; i < n_level_textures; ++i) {
                renderer_upload_texture(&tex_level[i], i + tex_level_start);
            }

            // Load graphics and collision data
            curr_level->graphics = model_load(path_model, 1, STACK_LEVEL, tex_level_start);
            curr_level->collision_mesh_debug = model_load_collision_debug(path_collision, 0, (stack_t)0);
            curr_level->collision_mesh = model_load_collision(path_collision, 1, STACK_LEVEL);
            curr_level->transform = { {0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096} };
            curr_level->vislist = vislist_load(path_vislist, 1, STACK_LEVEL);

            curr_level->collision_bvh = bvh_from_file(path_collision, 1, STACK_LEVEL);
            
            player->position = player_spawn_position;
            player->rotation = player_spawn_rotation;
            player_update(player, &curr_level->collision_bvh, 0, 0); // Tick the player with 0 delta time to update the camera transform
        }
    }
    ImGui::End();

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
            inspect_vec3(&camera->rotation, "Rotation");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Debug Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Render level graphics", &render_level_graphics);
            ImGui::Checkbox("Render level collision", &render_level_collision);
            ImGui::Checkbox("Render level BVH", &render_level_bvh);
            ImGui::DragInt("Min level", &render_level_bvh_start_depth);
            ImGui::DragInt("Max level", &render_level_bvh_end_depth);
            ImGui::Checkbox("Render level vislist regions", &render_level_vislist_regions);
            ImGui::Checkbox("Render Level navgraph", &render_level_nav_graph);
            ImGui::TreePop();
        }
    }
    ImGui::End();

    // Entity spawn menu
    ImGui::Begin("Entity spawning");
    {
        // Entity count:
        int entity_count = 0;
        for (size_t i = 0; i < ENTITY_LIST_LENGTH; ++i) {
            if (entity_types[i] != ENTITY_NONE) {
                ++entity_count;
            }
        }

        ImGui::Text("Entities: %i / %i", entity_count, ENTITY_LIST_LENGTH);

        // Entity select dropdown
        static size_t curr_selected_entity_type = 1;
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
                case ENTITY_CHASER:
                    entity = (entity_header_t*)entity_chaser_new();
                    entity->position = spawn_pos;
                    break;
            }
        }

        if (ImGui::Button("Defragment")) {
            entity_defragment();
        }
    }
    ImGui::End();

    // Entity inspector menu
    ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_None);
    {
        if (ImGui::TreeNode("All entities")) {
            for (size_t i = 0; i < ENTITY_LIST_LENGTH; ++i) {
                if (entity_types[i] != ENTITY_NONE) {
                    static std::string tree_nodes[ENTITY_LIST_LENGTH];
                    tree_nodes[i] = std::format("{} - {}", i, entity_names[entity_types[i]]);
                    if (ImGui::TreeNode(tree_nodes[i].c_str())) {
                        inspect_entity(i);
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }
        if (selected_entity_slot) {
            ImGui::Text("Selected entity");
            ImGui::Spacing();
            inspect_entity(*selected_entity_slot);
            
            // If deleted, deselect it
            if (entity_types[*selected_entity_slot] == ENTITY_NONE) {
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
        ImGui::Image((ImTextureID)(intptr_t)fb_texture, wsize, ImVec2(0, 1), ImVec2(1, 0));

        // Handle entity gizmo
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, window_width, window_height);
        mat4 delta;
        
        if (*selected_entity_slot != -1) {
                entity_header_t* selected_entity = (entity_header_t*)&entity_pool[(*selected_entity_slot) * entity_pool_stride];

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

#endif
void debug_layer_close(void) {
    ImGui::SaveIniSettingsToDisk("imgui_layout.ini");
}