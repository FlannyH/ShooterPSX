#include "main.h"

#include "engine/input_mapping.h"
#include "engine/renderer.h"
#include "engine/music.h"
#include "subnivis/input_map.h"
#include "subnivis/text.h"
#include "misc.h"
#include "ui.h"

extern volume_env_t vol_envs[N_SPU_CHANNELS];

int currently_mapping = -1;
int n_gamepad_input_box = 0;
input_gamepad_t gamepad_input_box[MAX_MAPPING_COUNT] = {0};

const char* input_names_gamepad[] = {
    "NONE",
    "UP",            "DOWN",
    "LEFT",          "RIGHT",
#if defined(_PC)
    "NORTH",         "SOUTH",
    "WEST",          "EAST",
#elif defined(_PSX)
    "TRIANGLE",      "CROSS",
    "SQUARE",        "CIRCLE",
#elif defined(_NDS)
    "X",             "B",
    "Y",             "A",
#endif
    "START",         "SELECT",
    "L1",            "R1",
    "L2",            "R2",
    "L3",            "R3",
    "STICK_LEFT_X",  "STICK_LEFT_Y",
    "STICK_RIGHT_X", "STICK_RIGHT_Y"
};

void state_enter_remap_controls(void) {
    state_enter_title_screen();
	state.global.state_to_return_to = get_prev_state();
	state.title_screen.button_pressed = 0;
	currently_mapping = -1;
	n_gamepad_input_box = 0;
	if (input_player_connected(0)) {
	    set_current_state(STATE_DEBUG_MENU_MAIN);
	}
	renderer_start_fade_in(FADE_SPEED);
	while (renderer_is_fading()) {
		renderer_delta_time(DT_TICK);
		renderer_begin_frame(&id_transform);
		ui_render_background();
		renderer_end_frame();
	}
}

void state_update_remap_controls(scalar_t dt) {
	(void)dt;

	renderer_begin_frame(&id_transform);

	ui_render_background();

    const char* mapping_names[] = { "JUMP", "MOVE_X", "MOVE_Y", "LOOK_MOUSE_X", "LOOK_MOUSE_Y", "LOOK_STICK_X", "LOOK_STICK_Y", "MENU_UP", "MENU_DOWN", "MENU_LEFT", "MENU_RIGHT", "MENU_GO", "MENU_PREVIEW", "MENU_TAB", "START_PAUSE", "FULL_SCREEN" };
    const size_t n_mappings = (sizeof(mapping_names) / sizeof(*mapping_names));

    if (currently_mapping != -1 && currently_mapping < (int)n_mappings) {
        // collect inputs
        const scalar_t tolerance = SCALAR(0.1);
        for (int i = 0; i < N_INPUT_GAMEPAD; ++i) {
            if (n_gamepad_input_box >= MAX_MAPPING_COUNT) break;
            const scalar_t delta = input_value_gamepad(i, 0) - input_value_gamepad_prev(i, 0);
            if (scalar_abs(delta) > tolerance) {
                int already_exists = 0;
                for (int j = 0; j < n_gamepad_input_box; ++j) {
                    if (gamepad_input_box[j] == (input_gamepad_t)i) {
                        already_exists = 1;
                        break;
                    }
                }
                if (!already_exists) {
                    gamepad_input_box[n_gamepad_input_box++] = i;
                }
            }
        }

        // remove on backspace press
        if (input_value_keyboard(INPUT_KEY_BACKSPACE) && !input_value_keyboard_prev(INPUT_KEY_BACKSPACE)) {
            if (n_gamepad_input_box > 0) n_gamepad_input_box--;
        }

        // draw stuff
    	renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) - SCALAR(64)}, "Collecting inputs for mapping:", 1, 1, white);
    	renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) - SCALAR(32)}, mapping_names[currently_mapping], 1, 1, white);
        if (n_gamepad_input_box == 0) {
            renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) + SCALAR(24)}, "(none)", 1, 1, white);
        }
        else {
            char text[256] = {0};
            size_t avail = 256;
            size_t cursor = 0;
            cursor += snprintf(&text[cursor], avail, "%s", input_names_gamepad[gamepad_input_box[0]]);
            for (int i = 1; i < n_gamepad_input_box; ++i) {
                cursor += snprintf(&text[cursor], avail, ", %s", input_names_gamepad[gamepad_input_box[i]]);
            }
            renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) + SCALAR(24)}, text, 1, 1, white);
        }
    	renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) + SCALAR(64)}, "Press backspace to remove a mapping", 1, 1, white);
    	renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) + SCALAR(80)}, "Press enter to continue", 1, 1, white);

        // continue on enter release
        if (!input_value_keyboard(INPUT_KEY_ENTER) && input_value_keyboard_prev(INPUT_KEY_ENTER)) {
            input_mapping_clear(currently_mapping + 1, INPUT_TYPE_GAMEPAD);
            for (int i = 0; i < n_gamepad_input_box; ++i) {
                input_mapping_register_gamepad(currently_mapping + 1, gamepad_input_box[i], SCALAR(1.0));
            }
            n_gamepad_input_box = 0;
            currently_mapping++;
        }
    }

    else if (currently_mapping >= (int)n_mappings) {
        #ifdef _DEBUG
        set_current_state(STATE_DEBUG_MENU_MAIN);
        #else
        set_current_state(STATE_TITLE_SCREEN);
        #endif
    }

    else if (currently_mapping == -1) {
        renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) - SCALAR(72)}, "Player 1", 1, 1, white);
        renderer_draw_text((vec2_t){SCALAR(256), SCALAR(128) - SCALAR(56)}, "Please select your controller", 1, 1, white);

        vec2_t pos = {SCALAR(64), SCALAR(112)};
        renderer_draw_text(pos, "Press a number on your keyboard", 0, 0, white);
        pos.y += SCALAR(16);

        int curr_i = 1;
        for (int i = 0; i < 16; ++i) {
            if (!input_device_connected(i)) continue;
            if (!input_gamepad_device_name(i)) continue;
            char number[8] = {0};
            snprintf(number, sizeof(number), "%i:", curr_i);
            renderer_draw_text(pos, number, 0, 0, white);
            pos.x += SCALAR(32);
            renderer_draw_text(pos, input_gamepad_device_name(i), 0, 0, white);
            pos.x -= SCALAR(32);
            pos.y += SCALAR(16);
            if (input_value_keyboard(INPUT_KEY_1 + curr_i - 1)) {
                input_gamepad_link_player_device(0, i);
                currently_mapping = 9999999; // todo(remap_trigger): desc: have a way to enter the remap controls wizard
            }
            curr_i++;
        }
    }

	renderer_end_frame();
	return;
}

void state_exit_remap_controls(void) {
}
