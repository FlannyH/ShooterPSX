#ifdef _PSX
#include "input.h"
#include <psxapi.h>     // API header, has InitPAD() and StartPAD() defs
#include <psxpad.h>

char pad_buff[2][34];
uint16_t button_prev[2] = {0, 0};
uint16_t button_curr[2] = {0, 0};
uint8_t left_stick_x_curr[2] = {128, 128};
uint8_t left_stick_y_curr[2] = {128, 128};
uint8_t right_stick_x_curr[2] = {128, 128};
uint8_t right_stick_y_curr[2] = {128, 128};
uint8_t left_stick_x_prev[2] = {128, 128};
uint8_t left_stick_y_prev[2] = {128, 128};
uint8_t right_stick_x_prev[2] = {128, 128};
uint8_t right_stick_y_prev[2] = {128, 128};
int8_t deadzone = 16;
PADTYPE* pad[2] = {0, 0};

void input_init() {
    // Init controller
	InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
	StartPAD();
	ChangeClearPAD(0);
    pad[0] = ((PADTYPE*)pad_buff[0]);
    pad[1] = ((PADTYPE*)pad_buff[1]);
}

void input_update() {
    for (int i = 0; i < 2; ++i) {
        // Pass current to previous
        button_prev[i] = button_curr[i];
        left_stick_x_prev[i] =  left_stick_x_curr[i];
        left_stick_y_prev[i] =  left_stick_y_curr[i];
        right_stick_x_prev[i] = right_stick_x_curr[i];
        right_stick_y_prev[i] = right_stick_y_curr[i];

        // Update current
        if (!input_is_connected(i)) return;
        button_curr[i] = pad[i]->btn;

        // If we have analog sticks, update those, taking deadzone into account
        if (input_has_analog(i)) {
            left_stick_x_curr[i] = (((pad[i]->ls_x-128) < -deadzone ) || ( (pad[i]->ls_x-128) > deadzone)) * (pad[i]->ls_x - 128);
            left_stick_y_curr[i] = (((pad[i]->ls_y-128) < -deadzone ) || ( (pad[i]->ls_y-128) > deadzone)) * (pad[i]->ls_y - 128);
            right_stick_x_curr[i] = (((pad[i]->rs_x-128) < -deadzone ) || ( (pad[i]->rs_x-128) > deadzone)) * (pad[i]->rs_x - 128);
            right_stick_y_curr[i] = (((pad[i]->rs_y-128) < -deadzone ) || ( (pad[i]->rs_y-128) > deadzone)) * (pad[i]->rs_y - 128);
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
int input_held(uint16_t button_mask, int player_id) {
    uint16_t all_button = button_curr[player_id];
    return (all_button & button_mask);
}

// Returns non-zero if pressed this frame
int input_pressed(uint16_t button_mask, int player_id) {
    uint16_t all_button = (button_curr[player_id] ^ button_prev[player_id]) & button_curr[player_id];
    return (all_button & button_mask);
}

// Returns non-zero if released this frame
int input_released(uint16_t button_mask, int player_id) {
    uint16_t all_button = (button_curr[player_id] ^ button_prev[player_id]) & button_prev[player_id];
    return (all_button & button_mask);
}

int8_t input_left_stick_x(int player_id) {
    return left_stick_x_curr[player_id];
}

int8_t input_left_stick_x_relative(int player_id) {
    return left_stick_x_curr[player_id] - left_stick_x_prev[player_id];
}

int8_t input_left_stick_y(int player_id) {
    return left_stick_y_curr[player_id];
}

int8_t input_left_stick_y_relative(int player_id) {
    return left_stick_y_curr[player_id] - left_stick_y_prev[player_id];
}

int8_t input_right_stick_x(int player_id) {
    return right_stick_x_curr[player_id];
}

int8_t input_right_stick_x_relative(int player_id) {
    return right_stick_x_curr[player_id] - left_stick_x_prev[player_id];
}

int8_t input_right_stick_y(int player_id) {
    return right_stick_y_curr[player_id];
}

int8_t input_right_stick_y_relative(int player_id) {
    return right_stick_y_curr[player_id] - left_stick_y_prev[player_id];
}
#endif