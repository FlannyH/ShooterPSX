#ifdef _PSX
#include "input.h"
#include "fixed_point.h"
#include <psxapi.h>     // API header, has InitPAD() and StartPAD() defs
#include <psxpad.h>

uint8_t pad_buff[2][34];
uint16_t button_prev[2] = {0, 0};
uint16_t button_curr[2] = {0, 0};
int8_t left_stick_x_curr[2] = {0, 0};
int8_t left_stick_y_curr[2] = {0, 0};
int8_t right_stick_x_curr[2] = {0, 0};
int8_t right_stick_y_curr[2] = {0, 0};
int8_t left_stick_x_prev[2] = {0, 0};
int8_t left_stick_y_prev[2] = {0, 0};
int8_t right_stick_x_prev[2] = {0, 0};
int8_t right_stick_y_prev[2] = {0, 0};
int8_t deadzone = 16;
PADTYPE* pad[2] = {0, 0};

void input_init(void) {
    // Init controller
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	StartPAD();
	ChangeClearPAD(0);
    pad[0] = ((PADTYPE*)pad_buff[0]);
    pad[1] = ((PADTYPE*)pad_buff[1]);
}

int8_t apply_deadzone(uint8_t input) {
    // In range -128, +127
    const int16_t deadzone16 = (int16_t)deadzone;
    int16_t stick = ((int16_t)input) - 128;

    // Separate sign and magnitude
    const int16_t sign = (stick < 0) ? -1 : +1;
    stick = scalar_abs(stick);

    // Subtract deadzone and clamp to 0, 127
    stick = scalar_clamp(stick - deadzone16, 0, 127);
    
    // Remap from 0, 127-deadzone to 0, 127
    stick = stick * 127 / (127-deadzone16);

    // Re-apply sign
    stick *= sign;


    return (int8_t)stick;
}

void input_update(void) {
    for (int i = 0; i < 2; ++i) {
        // Pass current to previous
        button_prev[i] = button_curr[i];
        left_stick_x_prev[i] =  left_stick_x_curr[i];
        left_stick_y_prev[i] =  left_stick_y_curr[i];
        right_stick_x_prev[i] = right_stick_x_curr[i];
        right_stick_y_prev[i] = right_stick_y_curr[i];

        // Update current
        if (!input_is_connected(i)) return;
        button_curr[i] = ~pad[i]->btn;

        // If we have analog sticks, update those, taking deadzone into account
        if (input_has_analog(i)) {
            left_stick_x_curr[i] = apply_deadzone(pad[i]->ls_x);
            left_stick_y_curr[i] = apply_deadzone(pad[i]->ls_y);
            right_stick_x_curr[i] = apply_deadzone(pad[i]->rs_x);
            right_stick_y_curr[i] = apply_deadzone(pad[i]->rs_y);
        }
    }
}

void input_set_stick_deadzone(int8_t new_deadzone) {
    deadzone = new_deadzone;
}

int input_has_analog(int player_id) {
    return ((pad[player_id]->type == 0x05) || (pad[player_id]->type == 0x07));
}

int input_is_connected(int player_id) {
    return (pad[player_id]->stat == 0);
}

// Returns non-zero if held
int input_held(const uint16_t button_mask, const int player_id) {
    const uint16_t all_button = button_curr[player_id];
    return (all_button & button_mask);
}

// Returns non-zero if pressed this frame
int input_pressed(const uint16_t button_mask, const int player_id) {
    const uint16_t all_button = (button_curr[player_id] ^ button_prev[player_id]) & button_curr[player_id];
    return (all_button & button_mask);
}

// Returns non-zero if released this frame
int input_released(const uint16_t button_mask, const int player_id) {
    const uint16_t all_button = (button_curr[player_id] ^ button_prev[player_id]) & button_prev[player_id];
    return (all_button & button_mask);
}

int8_t input_left_stick_x(const int player_id) {
    return left_stick_x_curr[player_id];
}

int8_t input_left_stick_x_relative(const int player_id) {
    return left_stick_x_curr[player_id] - left_stick_x_prev[player_id];
}

int8_t input_left_stick_y(const int player_id) {
    return left_stick_y_curr[player_id];
}

int8_t input_left_stick_y_relative(const int player_id) {
    return left_stick_y_curr[player_id] - left_stick_y_prev[player_id];
}

int8_t input_right_stick_x(const int player_id) {
    return right_stick_x_curr[player_id];
}

int8_t input_right_stick_x_relative(const int player_id) {
    return right_stick_x_curr[player_id] - left_stick_x_prev[player_id];
}

int8_t input_right_stick_y(const int player_id) {
    return right_stick_y_curr[player_id];
}

int8_t input_right_stick_y_relative(const int player_id) {
    return right_stick_y_curr[player_id] - left_stick_y_prev[player_id];
}
#endif