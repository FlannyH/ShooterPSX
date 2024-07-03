#include "../renderer.h"
#include "../input.h"
#include "../memory.h"
#include "../player.h"
#include "../entity.h"
#include "../level.h"
#include "../entities/door.h"
#include "../windows/debug_layer.h"
#include "../windows/psx.h"

int main(void) {
    renderer_init();
    input_init();
	entity_init();
    input_set_stick_deadzone(36);
    
    //level_t level = level_load("\\ASSETS\\MODELS\\LEVEL.LVL;1");
    level_t level = (level_t){0};
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

    entity_header_t* selected_entity = NULL;
    entity_header_t* hovered_entity = NULL;

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