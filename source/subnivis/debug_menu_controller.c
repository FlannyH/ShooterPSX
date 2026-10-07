#include "main.h"

#include "engine/input_mapping.h"
#include "engine/renderer.h"
#include "engine/memory.h"
#include "subnivis/input_map.h"
#include "subnivis/text.h"
#include "ui.h"

#ifdef _PSX
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxpad.h>
#endif

#ifdef _PC
#include "engine/pc/psx.h"
#include "engine/pc/debug_layer.h"
#endif

#ifdef _NDS
#include "engine/nds/psx.h"
#include <filesystem.h>
#endif

void state_enter_debug_menu_controller(void) {
    state_enter_title_screen();
    state.global.state_to_return_to = get_prev_state();
    state.title_screen.button_pressed = 0;
    renderer_start_fade_in(FADE_SPEED);
    while (renderer_is_fading()) {
        renderer_delta_time(DT_TICK);
        renderer_begin_frame(&id_transform);
        ui_render_background();
        renderer_end_frame();
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
