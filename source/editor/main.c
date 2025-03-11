#include "../main.h"

#include "camera.h"
#include "../pc/debug_layer.h"
#include "../pc/psx.h"
#include "../renderer.h"
#include "../entity.h"
#include "../player.h"
#include "../input.h"
#include "../level.h"
#include "../file.h"

#include <string.h>
#include <unistd.h> 

int widescreen = 0;
state_t current_state = STATE_NONE;
state_t prev_state = STATE_NONE;
state_vars_t state;

void set_current_state(state_t state) { current_state = state; }
state_t get_current_state(void) { return current_state; }
state_t get_prev_state(void) { return prev_state; }

int main(int argc, char** argv) {
    if      (argc == 1) chdir("./assets/");
    else if (argc == 2) chdir(argv[1]);
    else {
        printf("Usage: level_editor.exe [assets_folder]\n");
        printf("Default assets folder is \"./assets/\"\n");
        exit(1);
    }

    mem_init();
    renderer_init();
    input_init();
	entity_init();
    input_set_stick_deadzone(36);
    
    level_t level = (level_t){0}; // start with empty level
    memset(&level, 0, sizeof(level));
    player_t player; player_init(&player, vec3_from_scalar(0), vec3_from_scalar(0), 40, 0, 0);
    player_update(&player, &level.collision_bvh, 0, 0);
    debug_camera_t camera = debug_camera_new();

    int dt = 40;
    int time_counter = 0;
    int mouse_lock = 0;
    input_unlock_mouse();

    int selected_entity = -1;
    int selected_light = -1;
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
        debug_camera_update(&camera, dt, mouse_lock);
        
        renderer_begin_frame(&camera.transform);
        {
            entity_update_all(&player, 0);
            
            debug_layer_begin();
            debug_layer_manipulate_entity(&camera.transform, &selected_entity, &selected_light, &mouse_over_viewport, &level, &player);
            debug_layer_end();
        }
        renderer_end_frame();
    }

    return 0;
}
