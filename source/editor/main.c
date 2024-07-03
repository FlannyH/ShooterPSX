#include "../renderer.h"
#include "../input.h"
#include "../memory.h"
#include "../player.h"
#include "../entity.h"
#include "../entities/door.h"
#include "../windows/debug_layer.h"

typedef struct {
    model_t* graphics;
    model_t* collision_mesh_debug;
    collision_mesh_t* collision_mesh;
    transform_t transform;
    vislist_t vislist;
    bvh_t collision_bvh;
    // todo: add other fields as specified in FLVL documentation
} level_t;

/// Load level from path, including .LVL;1 extension.
level_t load_level(const char* path) {
    // Get file names
    const size_t len_path = strlen(path) + 1; // include null character
    const size_t extension_offset = len_path - 6;
    char* path_graphics = mem_alloc(len_path, MEM_CAT_UNDEFINED);
    char* path_textures = mem_alloc(len_path, MEM_CAT_UNDEFINED);
    char* path_collision = mem_alloc(len_path, MEM_CAT_UNDEFINED);
    char* path_vislist = mem_alloc(len_path, MEM_CAT_UNDEFINED);
    strcpy(path_graphics, path);
    strcpy(path_textures, path);
    strcpy(path_collision, path);
    strcpy(path_vislist, path);
    memcpy(path_graphics + extension_offset, "MSH", 3);
    memcpy(path_textures + extension_offset, "TXC", 3);
    memcpy(path_collision + extension_offset, "COL", 3);
    memcpy(path_vislist + extension_offset, "VIS", 3);

    // Load level textures
	texture_cpu_t *tex_level;
    tex_level_start = 0;
	const uint32_t n_level_textures = texture_collection_load(path_textures, &tex_level, 1, STACK_TEMP);
    for (uint8_t i = 0; i < n_level_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i + tex_level_start);
	}

    // Load entity textures
	texture_cpu_t *entity_textures;
    tex_entity_start = tex_level_start + n_level_textures;
	const uint32_t n_entity_textures = texture_collection_load("\\ASSETS\\MODELS\\ENTITY.TXC;1", &entity_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
	    renderer_upload_texture(&entity_textures[i], i + tex_entity_start);
	}

    level_t level = (level_t) {
        .graphics = model_load(path_graphics, 1, STACK_LEVEL),
        .collision_mesh_debug = model_load_collision_debug(path_collision, 1, STACK_LEVEL),
        .collision_mesh = model_load_collision(path_collision, 1, STACK_LEVEL),
        .transform = (transform_t){{0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096}},
        .vislist = vislist_load(path_vislist, 1, STACK_LEVEL),
    };
    bvh_from_model(&level.collision_bvh, level.collision_mesh);

    mem_free(path_graphics);
    mem_free(path_textures);
    mem_free(path_collision);
    mem_free(path_vislist);

    return level;
}

int main(void) {
    renderer_init();
    input_init();
	entity_init();
    input_set_stick_deadzone(36);
    
    level_t level = load_level("\\ASSETS\\MODELS\\LEVEL.LVL;1");
    player_t player = (player_t){};
    player.position.x = 11705653 / 2;
   	player.position.y = 11413985 / 2;
    player.position.z = 2112866  / 2;
	player.rotation.y = 4096 * 16;
    player_update(&player, &level.collision_bvh, 0, 0);

    int dt = 40;
    int time_counter = 0;
    int mouse_lock = 0;
    input_unlock_mouse();


    // temp    
    entity_header_t* selected_entity = NULL;
    entity_header_t* hovered_entity = NULL;
	entity_door_t* crate1;
	entity_door_t* crate2;
	crate1 = entity_door_new(); crate1->entity_header.position = (vec3_t){(1338 + 64*2)*ONE, 1294*ONE, (465+64*0)*ONE}; crate1->is_big_door = 1;
	crate2 = entity_door_new(); crate2->entity_header.position = (vec3_t){(1338 + 64*3)*ONE, 1294*ONE, (465+64*0)*ONE}; crate2->is_big_door = 1;

    int mouse_over_viewport = 0;

    while (!renderer_should_close()) {
	    mem_stack_release(STACK_TEMP);

        // Delta time
        dt = renderer_get_delta_time_ms();
        dt = scalar_min(dt, 40);
        time_counter += dt;

        // Allow locking and unlocking the mouse
        if (input_pressed(PAD_R2, 0) && mouse_over_viewport) {
            mouse_lock = 1;
            input_lock_mouse();
        }
        if (input_released(PAD_R2, 0)) {
            mouse_lock = 0;
            input_unlock_mouse();
        }

        // Update camera
        input_update();
        if (mouse_lock) {
            player_update(&player, &level.collision_bvh, dt, time_counter);
        }
        
        renderer_begin_frame(&player.transform);
        {
            renderer_draw_model_shaded(level.graphics, &level.transform, NULL, 0);
            entity_update_all(&player, 0);
            
            debug_layer_begin();
            debug_layer_manipulate_entity(&player.transform, &selected_entity, &mouse_over_viewport);
            debug_layer_end();
        }
        renderer_end_frame();
    }

    return 0;
}