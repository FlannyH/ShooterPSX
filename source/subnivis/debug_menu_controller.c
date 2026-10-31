#include "main.h"

#include "engine/input_mapping.h"
#include "engine/renderer.h"
#include "subnivis/input_map.h"
#include "ui.h"

void state_enter_debug_menu_controller(void) {
    state_enter_title_screen();
    renderer_stop_fade();
    state.global.state_to_return_to = get_prev_state();
    state.title_screen.button_pressed = 0;
    for (int i = 0; i < 2; ++i) {
        renderer_begin_frame(&id_transform);
        ui_render_background();
    	renderer_end_frame();
    	input_update();
    }
}

void state_update_debug_menu_controller(scalar_t dt) {
    (void)dt;
    renderer_begin_frame(&id_transform);
    ui_render_background();

    const char* names[] = {
        "None",
        "Up",     "Down",  "Left", "Right",
        "North",  "South", "West", "East",
        "Start",  "Select",
        "L1",     "R1",
        "L2",     "R2",
        "L3",     "R3",
        "SLX",    "SLY",
        "SRX",    "SRY", "why1", "why2", "why3"
    };

    char text[48] = {0};

    for (int i = INPUT_GAMEPAD_NONE; i < N_INPUT_GAMEPAD; ++i) {
        int ix = i % 8;
        int iy = i / 8;
        const scalar_t value = input_value_gamepad(i, 0);
        #ifdef _FLOAT
            snprintf(text, sizeof(text), "%.3f", value);
        #else
            const int32_t integer = scalar_abs(value) / FIXED_ONE;
            const int32_t fractional = scalar_abs(value) & ((1 << FRAC_BITS) - 1);
            char sign = ' ';
            if      (value > 0) sign = '+';
            else if (value < 0) sign = '-';
            snprintf(text, sizeof(text), "%c%Li.%03Li", sign, integer, (fractional * 1000) / (1 << FRAC_BITS));
        #endif
        renderer_draw_text(vec2_from_ints(32 + (60 * ix), 32 + (80 * iy)), names[i], 0, 0, white);
        renderer_draw_text(vec2_from_ints(24 + (60 * ix), 48 + (80 * iy)), text, 0, 0, white);
    }

    renderer_end_frame();
}

void state_exit_debug_menu_controller(void) {
    renderer_start_fade_out(FADE_SPEED);

    while (renderer_is_fading()) {
        renderer_delta_time(DT_TICK);
        renderer_begin_frame(&id_transform);
        input_update();
        ui_render_background();
        renderer_end_frame();
    }
}
