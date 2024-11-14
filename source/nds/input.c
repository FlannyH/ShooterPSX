#include "input.h"

#include "nds/psx.h"

#include <nds/arm9/input.h>

uint16_t button_prev = 0;
uint16_t button_curr = 0;
int button_pressed_this_frame = 0;
uint16_t input_buffer[32];

void input_init(void) {
    // No initialization required
}

void input_update(void) {
    scanKeys();
    uint16_t key_state = keysHeld();
    button_prev = button_curr;
    button_curr = 0;
    if (key_state & KEY_A) button_curr |= PAD_R2;
    if (key_state & KEY_B) button_curr |= PAD_CROSS;
    if (key_state & KEY_X) button_curr |= PAD_TRIANGLE;
    if (key_state & KEY_Y) button_curr |= PAD_L2;
    if (key_state & KEY_UP) button_curr |= PAD_UP;
    if (key_state & KEY_LEFT) button_curr |= PAD_LEFT;
    if (key_state & KEY_DOWN) button_curr |= PAD_DOWN;
    if (key_state & KEY_RIGHT) button_curr |= PAD_RIGHT;
    if (key_state & KEY_L) button_curr |= PAD_L1;
    if (key_state & KEY_R) button_curr |= PAD_R1;
    if (key_state & KEY_START) button_curr |= PAD_START;
    if (key_state & KEY_SELECT) button_curr |= PAD_SELECT;
    
    // Update cheat buffer
    button_pressed_this_frame = 0;
    uint16_t buttons_pressed = (button_curr ^ button_prev) & button_curr;
    if (buttons_pressed) {
        for (size_t i = 31; i > 0; --i) input_buffer[i] = input_buffer[i-1];
        input_buffer[0] = buttons_pressed;
        button_pressed_this_frame = 1;
    }
}

int input_is_connected(int player_id) {
    return (player_id == 0); // NDS has 1 controller and it's on so long as the system is on
}

int input_held(uint16_t button_mask, int player_id) {
    if (player_id != 0) return 0;
    return (button_curr & button_mask);
}

int input_pressed(uint16_t button_mask, int player_id) {
    if (player_id != 0) return 0;
    return (((button_curr ^ button_prev) & button_curr) & button_mask);
}

int input_released(uint16_t button_mask, int player_id) {
    if (player_id != 0) return 0;
    return (((button_curr ^ button_prev) & button_prev) & button_mask);
}

int input_check_cheat_buffer(int n_inputs, const uint16_t* inputs_to_check) {
    int match = button_pressed_this_frame;
    for (int i = 0; i < n_inputs; ++i) {
        if (inputs_to_check[i] != input_buffer[i]) {
            match = 0;
        }
    }
    return match;
}

// Below this comment are dummy function implementations that are irrelevant for NDS, feel free to ignore
void input_set_stick_deadzone(int8_t new_deadzone) {
    (void)new_deadzone;
}

int input_has_analog(int player_id) {
    (void)player_id;
    return 0; // NDS doesn't have joysticks, so always returns false
}

int8_t input_left_stick_x(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_left_stick_x_relative(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_left_stick_y(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_left_stick_y_relative(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_right_stick_x(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_right_stick_x_relative(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_right_stick_y(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_right_stick_y_relative(int player_id) {
    (void)player_id;
    return 0;
}

void input_rumble(uint8_t left_strength, uint8_t right_enable) {
    (void)left_strength;
    (void)right_enable;
}

int input_mouse_connected(void) {
    return 0;
}

void input_lock_mouse(void) {}

void input_unlock_mouse(void) {}
