#include <stdio.h>

#include "input.h"
#include "glfw/glfw3.h"
#include "win/psx.h"

extern GLFWwindow* window;
int8_t left_stick_x[2] = { 0, 0 };
int8_t left_stick_y[2] = { 0, 0 };
int8_t right_stick_x[2] = { 0, 0 };
int8_t right_stick_y[2] = { 0, 0 };
uint16_t button_prev[2] = { 0, 0 };
uint16_t button_curr[2] = { 0, 0 };
int8_t deadzone = 24;

void input_init() {
}

void input_update() {
    // Reset buttons
    button_prev[0] = button_curr[0];
    button_prev[1] = button_curr[1];
    button_curr[0] = 0;
    button_curr[1] = 0;

    // Update analog sticks
    int count = 0;
    const float* axes_1 = glfwGetJoystickAxes(0, &count);
    const float* axes_2 = glfwGetJoystickAxes(1, &count);
    const unsigned char* buttons_1 = glfwGetJoystickButtons(0, &count);
    const unsigned char* buttons_2 = glfwGetJoystickButtons(1, &count);
    if (axes_1) {
        left_stick_x[0] = (int8_t)(axes_1[0] * 127.f);
        left_stick_y[0] = (int8_t)(axes_1[1] * 127.f);
        right_stick_x[0] = (int8_t)(axes_1[2] * 127.f);
        right_stick_y[0] = (int8_t)(axes_1[3] * 127.f);
        button_curr[0] |= (PAD_L2)*axes_1[4] > 0.5f ? 1 : 0;
    }
    if (axes_2) {
        left_stick_x[1] = (int8_t)(axes_2[0] * 127.f);
        left_stick_y[1] = (int8_t)(axes_2[1] * 127.f);
        right_stick_x[1] = (int8_t)(axes_2[2] * 127.f);
        right_stick_y[1] = (int8_t)(axes_2[3] * 127.f);
        button_curr[1] |= (PAD_L2)*axes_2[4] > 0.5f ? 1 : 0;
    }

    // Update buttons
    // TODO: test this with xbox controller as ground truth
    if (buttons_1) {
        button_curr[0] |= (PAD_SELECT)*buttons_1[6];
        button_curr[0] |= (PAD_L3)*buttons_1[8];
        button_curr[0] |= (PAD_R3)*buttons_1[9];
        button_curr[0] |= (PAD_START)*buttons_1[7];
        button_curr[0] |= (PAD_UP)*buttons_1[10];
        button_curr[0] |= (PAD_RIGHT)*buttons_1[11];
        button_curr[0] |= (PAD_DOWN)*buttons_1[12];
        button_curr[0] |= (PAD_LEFT)*buttons_1[13];
        //button_curr[0] |= (PAD_L2)*buttons_1[6];
        //button_curr[0] |= (PAD_R2)*buttons_1[6];
        button_curr[0] |= (PAD_L1)*buttons_1[4];
        button_curr[0] |= (PAD_R1)*buttons_1[5];
        button_curr[0] |= (PAD_TRIANGLE)*buttons_1[3];
        button_curr[0] |= (PAD_CIRCLE)*buttons_1[1];
        button_curr[0] |= (PAD_CROSS)*buttons_1[0];
        button_curr[0] |= (PAD_SQUARE)*buttons_1[2];
    }
    if (buttons_2) {
        button_curr[1] |= (PAD_SELECT)*buttons_2[6];
        button_curr[1] |= (PAD_L3)*buttons_2[8];
        button_curr[1] |= (PAD_R3)*buttons_2[9];
        button_curr[1] |= (PAD_START)*buttons_2[7];
        button_curr[1] |= (PAD_UP)*buttons_2[10];
        button_curr[1] |= (PAD_RIGHT)*buttons_2[11];
        button_curr[1] |= (PAD_DOWN)*buttons_2[12];
        button_curr[1] |= (PAD_LEFT)*buttons_2[13];
        //button_curr[1] |= (PAD_L2)*buttons_2[6];
        //button_curr[1] |= (PAD_R2)*buttons_2[6];
        button_curr[1] |= (PAD_L1)*buttons_2[4];
        button_curr[1] |= (PAD_R1)*buttons_2[5];
        button_curr[1] |= (PAD_TRIANGLE)*buttons_2[3];
        button_curr[1] |= (PAD_CIRCLE)*buttons_2[1];
        button_curr[1] |= (PAD_CROSS)*buttons_2[0];
        button_curr[1] |= (PAD_SQUARE)*buttons_2[2];
    }
    
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
    if (value < deadzone && value > -deadzone) return 0;
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
}

int8_t input_left_stick_x_relative(int player_id) {
    return 0;
}

int8_t input_left_stick_y(const int player_id) {
    const int8_t value = left_stick_y[player_id];
    if (value < deadzone && value > -deadzone) return 0;
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
}

int8_t input_left_stick_y_relative(int player_id) {
    return 0;
}

int8_t input_right_stick_x(const int player_id) {
    const int8_t value = right_stick_x[player_id];
    if (value < deadzone && value > -deadzone) return 0;
    if(value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if(value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
}

int8_t input_right_stick_x_relative(int player_id) {
    return 0;
}

int8_t input_right_stick_y(const int player_id) {
    const int8_t value = right_stick_y[player_id];
    if (value < deadzone && value > -deadzone) return 0;
    if (value < -deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)deadzone) / (127 - (int16_t)deadzone));
    if (value > deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)deadzone) / (127 - (int16_t)deadzone));
}

int8_t input_right_stick_y_relative(int player_id) {
    return 0;
}
