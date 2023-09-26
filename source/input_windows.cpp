#include <cstdio>
#include <algorithm>

#include "fixed_point.h"
#include "input.h"
#include "win/psx.h"
#include "JSL/JoyShockLibrary.h"

int8_t left_stick_x[2] = { 0, 0 };
int8_t left_stick_y[2] = { 0, 0 };
int8_t right_stick_x[2] = { 0, 0 };
int8_t right_stick_y[2] = { 0, 0 };
uint16_t button_prev[2] = { 0, 0 };
uint16_t button_curr[2] = { 0, 0 };
float gyro_x[2] = { 0.0, 0.0 };
float gyro_y[2] = { 0.0, 0.0 };
int8_t deadzone = 24;
int player1_index = -1;
int player2_index = -1;
int button_pressed_this_frame = 0;
uint16_t input_buffer[32];
int devices_connected[32];
int n_devices_connected;

void input_init() {
    n_devices_connected = JslConnectDevices();
    JslGetConnectedDeviceHandles(devices_connected, n_devices_connected);
}

void input_update() {
    // Detect controller the player is using by waiting for an input
    if (player1_index == -1) {
        n_devices_connected = JslConnectDevices();
        printf("n_devices_connected: %i\n", n_devices_connected);
        JslGetConnectedDeviceHandles(devices_connected, n_devices_connected);
        for (int i = 0; i < n_devices_connected; ++i) {
            const auto button_state = JslGetSimpleState(i);
            player1_index = i;
            JslSetPlayerNumber(i, 1);
            JslSetAutomaticCalibration(player1_index, true);
            printf("Controller found!\n");
            break;
        
            return;
        }
    }


    // Reset buttons
    button_prev[0] = button_curr[0];
    button_prev[1] = button_curr[1];
    button_curr[0] = 0;
    button_curr[1] = 0;

    auto button_state = JslGetSimpleState(player1_index);
    // D-PAD
    button_curr[0] |= ((button_state.buttons & JSMASK_UP) > 0) ? PAD_UP : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_DOWN) > 0) ? PAD_DOWN : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_LEFT) > 0) ? PAD_LEFT : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_RIGHT) > 0) ? PAD_RIGHT : 0;
    // ABXY
    button_curr[0] |= ((button_state.buttons & JSMASK_N) > 0) ? PAD_TRIANGLE : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_S) > 0) ? PAD_CROSS : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_W) > 0) ? PAD_SQUARE : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_E) > 0) ? PAD_CIRCLE : 0;
    // Start Select
    button_curr[0] |= ((button_state.buttons & JSMASK_MINUS) > 0) ? PAD_SELECT : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_PLUS) > 0) ? PAD_START : 0;
    // Shoulder/Triggers
    button_curr[0] |= ((button_state.buttons & JSMASK_L) > 0) ? PAD_L1 : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_R) > 0) ? PAD_R1 : 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_ZL) > 0) ? PAD_L2: 0;
    button_curr[0] |= ((button_state.buttons & JSMASK_ZR) > 0) ? PAD_R2: 0;
    // Joysticks
    left_stick_x[0] = static_cast<int8_t>(std::clamp(button_state.stickLX * 127.0f, -127.0f, 127.0f));
    left_stick_y[0] = static_cast<int8_t>(std::clamp(button_state.stickLY * -127.0f, -127.0f, 127.0f));
    right_stick_x[0] = static_cast<int8_t>(std::clamp(button_state.stickRX * 127.0f, -127.0f, 127.0f));
    right_stick_y[0] = static_cast<int8_t>(std::clamp(button_state.stickRY * -127.0f, -127.0f, 127.0f));
    // Gyro
    float x, y, z;
    JslGetAndFlushAccumulatedGyro(player1_index, x, y, z);
    gyro_x[0] = x * 2.0f;
    gyro_y[0] = -y * 2.0f;

    // Update cheat buffer
    button_pressed_this_frame = 0;
    uint16_t buttons_pressed = (button_curr[0] ^ button_prev[0]) & button_curr[0];
    if (buttons_pressed) {
        for (size_t i = 31; i > 0; --i) input_buffer[i] = input_buffer[i-1];
        input_buffer[0] = buttons_pressed;
        button_pressed_this_frame = 1;
    }
}

scalar_t input_gyro_x(int player_id) {
    return scalar_from_float(gyro_x[player_id]);
}

scalar_t input_gyro_y(int player_id) {
    return scalar_from_float(gyro_y[player_id]);
}

void input_set_stick_deadzone(int8_t new_deadzone) {
    deadzone = new_deadzone;
}

int input_has_analog(int player_id) {
    return 1;
}

int input_is_connected(int player_id) {
    return 1;
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
    const int8_t value = left_stick_x[player_id];
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
    return 0;
}

int8_t input_left_stick_x_relative(int player_id) {
    return 0;
}

int8_t input_left_stick_y(const int player_id) {
    const int8_t value = left_stick_y[player_id];
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
    return 0;
}

int8_t input_left_stick_y_relative(int player_id) {
    return 0;
}

int8_t input_right_stick_x(const int player_id) {
    const int8_t value = right_stick_x[player_id];
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
    return 0;
}

int8_t input_right_stick_x_relative(int player_id) {
    return 0;
}

int8_t input_right_stick_y(const int player_id) {
    const int8_t value = right_stick_y[player_id];
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
    return 0;
}

int8_t input_right_stick_y_relative(int player_id) {
    return 0;
}

int input_check_cheat_buffer(int n_inputs, uint16_t* inputs_to_check) {\
    int match = button_pressed_this_frame;
    for (int i = 0; i < n_inputs; ++i) {
        //printf("expect: %i, got: %i\n", inputs_to_check[i], input_buffer[i]);
        if (inputs_to_check[i] != input_buffer[i]) {
            match = 0;
        }
    }
    return match;
}

void input_rumble(uint8_t left_strength, uint8_t right_enable)
{
    JslSetRumble(player1_index, left_strength, right_enable);
}