#include <nds/arm9/input.h>

#include "../input.h"

#include "../common.h"
#include "psx.h"

typedef struct {
    int up : 1, down : 1, left : 1, right : 1;
    scalar_t x : 1, b : 1, a : 1, y : 1;
    scalar_t start : 1, select : 1;
    scalar_t l : 1, r : 1;
} controller_state_t;

#define N_CONTROLLER 1
controller_state_t controller_prev[N_CONTROLLER] = {0};
controller_state_t controller_curr[N_CONTROLLER] = {0};

void input_init(void) {
    // No initialization required
    return;
}

void input_update(void) {
    scanKeys();
    const uint16_t key_state = keysHeld();
    controller_curr[0].a = (key_state & KEY_A) ? 1 : 0;
    controller_curr[0].b = (key_state & KEY_B) ? 1 : 0;
    controller_curr[0].x = (key_state & KEY_X) ? 1 : 0;
    controller_curr[0].y = (key_state & KEY_Y) ? 1 : 0;
    controller_curr[0].up = (key_state & KEY_UP) ? 1 : 0;
    controller_curr[0].left = (key_state & KEY_LEFT) ? 1 : 0;
    controller_curr[0].down = (key_state & KEY_DOWN) ? 1 : 0;
    controller_curr[0].right = (key_state & KEY_RIGHT) ? 1 : 0;
    controller_curr[0].l = (key_state & KEY_L) ? 1 : 0;
    controller_curr[0].r = (key_state & KEY_R) ? 1 : 0;
    controller_curr[0].start = (key_state & KEY_START) ? 1 : 0;
    controller_curr[0].select = (key_state & KEY_SELECT) ? 1 : 0;
}

int input_gamepad_connected(int player_id) {
    return (player_id == 0);
}

scalar_t input_value_gamepad(input_gamepad_t input, int player_id) {
    if (player_id < 0 || player_id >= N_CONTROLLER) return 0;

    switch (input) {
        default:                          return 0;
        case INPUT_GAMEPAD_UP:            return controller_curr[player_id].up * ONE;
        case INPUT_GAMEPAD_DOWN:          return controller_curr[player_id].down * ONE;
        case INPUT_GAMEPAD_LEFT:          return controller_curr[player_id].left * ONE;
        case INPUT_GAMEPAD_RIGHT:         return controller_curr[player_id].right * ONE;
        case INPUT_GAMEPAD_NORTH:         return controller_curr[player_id].x * ONE;
        case INPUT_GAMEPAD_SOUTH:         return controller_curr[player_id].b * ONE;
        case INPUT_GAMEPAD_WEST:          return controller_curr[player_id].y * ONE;
        case INPUT_GAMEPAD_EAST:          return controller_curr[player_id].a * ONE;
        case INPUT_GAMEPAD_START:         return controller_curr[player_id].start * ONE;
        case INPUT_GAMEPAD_SELECT:        return controller_curr[player_id].select * ONE;
        case INPUT_GAMEPAD_L1:            return controller_curr[player_id].l * ONE;
        case INPUT_GAMEPAD_R1:            return controller_curr[player_id].r * ONE;
    }
}
scalar_t input_value_gamepad_prev(input_gamepad_t input, int player_id) {
    if (player_id < 0 || player_id >= N_CONTROLLER) return 0;

    switch (input) {
        default:                          return 0;
        case INPUT_GAMEPAD_UP:            return controller_prev[player_id].up * ONE;
        case INPUT_GAMEPAD_DOWN:          return controller_prev[player_id].down * ONE;
        case INPUT_GAMEPAD_LEFT:          return controller_prev[player_id].left * ONE;
        case INPUT_GAMEPAD_RIGHT:         return controller_prev[player_id].right * ONE;
        case INPUT_GAMEPAD_NORTH:         return controller_prev[player_id].x * ONE;
        case INPUT_GAMEPAD_SOUTH:         return controller_prev[player_id].b * ONE;
        case INPUT_GAMEPAD_WEST:          return controller_prev[player_id].y * ONE;
        case INPUT_GAMEPAD_EAST:          return controller_prev[player_id].a * ONE;
        case INPUT_GAMEPAD_START:         return controller_prev[player_id].start * ONE;
        case INPUT_GAMEPAD_SELECT:        return controller_prev[player_id].select * ONE;
        case INPUT_GAMEPAD_L1:            return controller_prev[player_id].r * ONE;
        case INPUT_GAMEPAD_R1:            return controller_prev[player_id].l * ONE;
    }
}

// no equivalent on this platform
void input_lock_mouse(void) {}
void input_unlock_mouse(void) {}
void input_rumble(scalar_t left_strength, scalar_t right_enable) {}
void input_set_gamepad_stick_deadzone(scalar_t) {}
scalar_t input_value_keyboard(input_keyboard_t input) { return 0; }
scalar_t input_value_mouse(input_mouse_t input) { return 0; }
scalar_t input_value_keyboard_prev(input_keyboard_t input) { return 0; }
scalar_t input_value_mouse_prev(input_mouse_t input) { return 0; }
