#include <stdio.h>

#include "input.h"
#include "glfw/glfw3.h"

extern GLFWwindow* window;
int8_t left_stick_x[2] = { 0, 0 };
int8_t left_stick_y[2] = { 0, 0 };
int8_t right_stick_x[2] = { 0, 0 };
int8_t right_stick_y[2] = { 0, 0 };
int8_t deadzone = 24;

void input_init() {
}

void input_update() {
    // Update analog sticks
    int count = 0;
    const float* axes_1 = glfwGetJoystickAxes(0, &count);
    const float* axes_2 = glfwGetJoystickAxes(0, &count);
    if (axes_1) {
        left_stick_x[0] = (int8_t)(axes_1[0] * 127.f);
        left_stick_y[0] = (int8_t)(axes_1[1] * 127.f);
        right_stick_x[0] = (int8_t)(axes_1[2] * 127.f);
        right_stick_y[0] = (int8_t)(axes_1[3] * 127.f);
    }
    if (axes_2) {
        left_stick_x[1] = (int8_t)(axes_2[0] * 127.f);
        left_stick_y[1] = (int8_t)(axes_2[1] * 127.f);
        right_stick_x[1] = (int8_t)(axes_2[2] * 127.f);
        right_stick_y[1] = (int8_t)(axes_2[3] * 127.f);
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

int input_held(uint16_t button_mask, int player_id) {
    return 0;
}

int input_pressed(uint16_t button_mask, int player_id) {
    return 0;
}

int input_released(uint16_t button_mask, int player_id) {
    return 0;
}

int8_t input_left_stick_x(const int player_id) {
    const int8_t value = left_stick_x[player_id];
    if (value < deadzone && value > -deadzone)
        return 0;
    return value;
}

int8_t input_left_stick_x_relative(int player_id) {
    return 0;
}

int8_t input_left_stick_y(const int player_id) {
    const int8_t value = left_stick_y[player_id];
    if (value < deadzone && value > -deadzone)
        return 0;
    return value;
}

int8_t input_left_stick_y_relative(int player_id) {
    return 0;
}

int8_t input_right_stick_x(const int player_id) {
    const int8_t value = right_stick_x[player_id];
    if (value < deadzone && value > -deadzone)
        return 0;
    return value;
}

int8_t input_right_stick_x_relative(int player_id) {
    return 0;
}

int8_t input_right_stick_y(const int player_id) {
    const int8_t value = right_stick_y[player_id];
    if (value < deadzone && value > -deadzone)
        return 0;
    return value;
}

int8_t input_right_stick_y_relative(int player_id) {
    return 0;
}
