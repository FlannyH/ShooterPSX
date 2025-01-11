#include "input.h"

#include "windows/psx.h"

#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <stdio.h>

extern GLFWwindow* window;
int8_t left_stick_x[2] = { 0, 0 };
int8_t left_stick_y[2] = { 0, 0 };
int8_t right_stick_x[2] = { 0, 0 };
int8_t right_stick_y[2] = { 0, 0 };
uint16_t button_prev[2] = { 0, 0 };
uint16_t button_curr[2] = { 0, 0 };
int8_t deadzone = 24;
int8_t currently_active_deadzone = 24;
int keyboard_focus = 1;
int player1_index = -1;
int player2_index = -1;
int button_pressed_this_frame = 0;
int mouse_lock = 0;
uint16_t input_buffer[32];

void input_init(void) {}

void input_update(void) {
    // Detect controllers
    for (int i = 0; i < 8; ++i) {
        if (i == player1_index || i == player2_index)
            continue;

        GLFWgamepadstate state;
        if (glfwGetGamepadState(i, &state)) {
            if (player1_index == -1)
                player1_index = i;
            else if (player2_index == -1) {
                player2_index = i;
                break;
            }
        }
    }

    if (player1_index == -1) player1_index = 0;
    if (player2_index == -1) player2_index = 0;

    // Reset buttons
    button_prev[0] = button_curr[0];
    button_prev[1] = button_curr[1];
    button_curr[0] = 0;
    button_curr[1] = 0;

    // Update analog sticks
    GLFWgamepadstate state1;
    GLFWgamepadstate state2;
    glfwGetGamepadState(player1_index, &state1);
    glfwGetGamepadState(player2_index, &state2);
    
    // Gamepad
    if (glfwGetGamepadState(player1_index, &state1)) {
        left_stick_x[0] = (int8_t)(state1.axes[GLFW_GAMEPAD_AXIS_LEFT_X] * 127.f);
        left_stick_y[0] = (int8_t)(state1.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * 127.f);
        right_stick_x[0] = (int8_t)(state1.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * 127.f);
        right_stick_y[0] = (int8_t)(state1.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * 127.f);
        button_curr[0] |= (PAD_L2)*(state1.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.0f ? 1 : 0);
        button_curr[0] |= (PAD_R2)*(state1.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.0f ? 1 : 0);
        button_curr[0] |= (PAD_SELECT)*     state1.buttons[GLFW_GAMEPAD_BUTTON_BACK];
        button_curr[0] |= (PAD_L3)*         state1.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
        button_curr[0] |= (PAD_R3)*         state1.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
        button_curr[0] |= (PAD_START)*      state1.buttons[GLFW_GAMEPAD_BUTTON_START];
        button_curr[0] |= (PAD_UP)*         state1.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
        button_curr[0] |= (PAD_RIGHT)*      state1.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
        button_curr[0] |= (PAD_DOWN)*       state1.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
        button_curr[0] |= (PAD_LEFT)*       state1.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
        button_curr[0] |= (PAD_L1)*         state1.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
        button_curr[0] |= (PAD_R1)*         state1.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
        button_curr[0] |= (PAD_TRIANGLE)*   state1.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE];
        button_curr[0] |= (PAD_CIRCLE)*     state1.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE];
        button_curr[0] |= (PAD_CROSS)*      state1.buttons[GLFW_GAMEPAD_BUTTON_CROSS];
        button_curr[0] |= (PAD_SQUARE)*     state1.buttons[GLFW_GAMEPAD_BUTTON_SQUARE];
        if (button_curr[0]) keyboard_focus = 0;
    }
    else {
        keyboard_focus = 1;
    }
    if (glfwGetGamepadState(player2_index, &state2)) {
        left_stick_x[1] = (int8_t)(state2.axes[GLFW_GAMEPAD_AXIS_LEFT_X] * 127.f);
        left_stick_y[1] = (int8_t)(state2.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * 127.f);
        right_stick_x[1] = (int8_t)(state2.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * 127.f);
        right_stick_y[1] = (int8_t)(state2.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * 127.f);
        button_curr[1] |= (PAD_L2)*(state2.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.0f ? 1 : 0);
        button_curr[1] |= (PAD_R2)*(state2.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.0f ? 1 : 0);
        button_curr[1] |= (PAD_SELECT)*     state2.buttons[GLFW_GAMEPAD_BUTTON_BACK];
        button_curr[1] |= (PAD_L3)*         state2.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
        button_curr[1] |= (PAD_R3)*         state2.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
        button_curr[1] |= (PAD_START)*      state2.buttons[GLFW_GAMEPAD_BUTTON_START];
        button_curr[1] |= (PAD_UP)*         state2.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
        button_curr[1] |= (PAD_RIGHT)*      state2.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
        button_curr[1] |= (PAD_DOWN)*       state2.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
        button_curr[1] |= (PAD_LEFT)*       state2.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
        button_curr[1] |= (PAD_L1)*         state2.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
        button_curr[1] |= (PAD_R1)*         state2.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
        button_curr[1] |= (PAD_TRIANGLE)*   state2.buttons[GLFW_GAMEPAD_BUTTON_TRIANGLE];
        button_curr[1] |= (PAD_CIRCLE)*     state2.buttons[GLFW_GAMEPAD_BUTTON_CIRCLE];
        button_curr[1] |= (PAD_CROSS)*      state2.buttons[GLFW_GAMEPAD_BUTTON_CROSS];
        button_curr[1] |= (PAD_SQUARE)*     state2.buttons[GLFW_GAMEPAD_BUTTON_SQUARE];
    }

    // Keyboard & mouse input
    static double cursor_pos_prev_x = 0.0;
    static double cursor_pos_prev_y = 0.0;
    double cursor_pos_x, cursor_pos_y;
    glfwGetCursorPos(window, &cursor_pos_x, &cursor_pos_y);
    right_stick_x[0] = (int8_t)(cursor_pos_x - cursor_pos_prev_x);
    right_stick_y[0] = (int8_t)(cursor_pos_y - cursor_pos_prev_y);

    if (mouse_lock) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        w /= 2;
        h /= 2;
        glfwSetCursorPos(window, w, h);
        cursor_pos_prev_x = w;
        cursor_pos_prev_y = h;
    }
    else {
        cursor_pos_prev_x = cursor_pos_x;
        cursor_pos_prev_y = cursor_pos_y;
    }
    left_stick_x[0] = 0;
    left_stick_y[0] = 0;
    if (glfwGetKey(window, GLFW_KEY_W)) { left_stick_y[0] += 127; }
    if (glfwGetKey(window, GLFW_KEY_S)) { left_stick_y[0] -= 127; }
    if (glfwGetKey(window, GLFW_KEY_A)) { left_stick_x[0] += 127; }
    if (glfwGetKey(window, GLFW_KEY_D)) { left_stick_x[0] -= 127; }
    button_curr[0] |= (PAD_SELECT)*glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT);
    //button_curr[0] |= (PAD_L3)*       glfwGetKey(window, GLFW_KEY_)   
    //button_curr[0] |= (PAD_R3)*       glfwGetKey(window, GLFW_KEY_)   
    button_curr[0] |= (PAD_START)*glfwGetKey(window, GLFW_KEY_ESCAPE);
    button_curr[0] |= (PAD_UP)*glfwGetKey(window, GLFW_KEY_UP);
    button_curr[0] |= (PAD_RIGHT)*glfwGetKey(window, GLFW_KEY_RIGHT);
    button_curr[0] |= (PAD_DOWN)*glfwGetKey(window, GLFW_KEY_DOWN);
    button_curr[0] |= (PAD_LEFT)*glfwGetKey(window, GLFW_KEY_LEFT);
    //button_curr[0] |= (PAD_L1)*glfwGetKey(window, GLFW_KEY_)
    //button_curr[0] |= (PAD_R1)*glfwGetKey(window, GLFW_KEY_)
    button_curr[0] |= (PAD_L2)*glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    button_curr[0] |= (PAD_R2)*glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    button_curr[0] |= (PAD_TRIANGLE)* glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
    button_curr[0] |= (PAD_CIRCLE)*glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL);
    button_curr[0] |= (PAD_CROSS)*glfwGetKey(window, GLFW_KEY_SPACE);
    button_curr[0] |= (PAD_SQUARE)*glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);

    currently_active_deadzone = keyboard_focus ? 0 : deadzone;

    // Update cheat buffer
    button_pressed_this_frame = 0;
    const uint16_t buttons_pressed = (button_curr[0] ^ button_prev[0]) & button_curr[0];
    if (buttons_pressed) {
        for (size_t i = 31; i > 0; --i) input_buffer[i] = input_buffer[i-1];
        input_buffer[0] = buttons_pressed;
        button_pressed_this_frame = 1;
    }
}

void input_set_stick_deadzone(int8_t new_deadzone) {
    deadzone = new_deadzone;
}

int input_has_analog(int player_id) {
    (void)player_id;
    return 1;
}

int input_is_connected(int player_id) {
    (void)player_id;
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
    if (value < -currently_active_deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)currently_active_deadzone) / (127 - (int16_t)deadzone));
    if (value > currently_active_deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)currently_active_deadzone) / (127 - (int16_t)deadzone));
    return 0;
}

int8_t input_left_stick_x_relative(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_left_stick_y(const int player_id) {
    const int8_t value = left_stick_y[player_id];
    if (value < -currently_active_deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)currently_active_deadzone) / (127 - (int16_t)deadzone));
    if (value > currently_active_deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)currently_active_deadzone) / (127 - (int16_t)deadzone));
    return 0;
}

int8_t input_left_stick_y_relative(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_right_stick_x(const int player_id) {
    const int8_t value = right_stick_x[player_id];
    if (value < -currently_active_deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)currently_active_deadzone) / (127 - (int16_t)currently_active_deadzone));
    if (value > currently_active_deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)currently_active_deadzone) / (127 - (int16_t)currently_active_deadzone));
    return 0;
}

int8_t input_right_stick_x_relative(int player_id) {
    (void)player_id;
    return 0;
}

int8_t input_right_stick_y(const int player_id) {
    const int8_t value = right_stick_y[player_id];
    if (value < -currently_active_deadzone) return (int8_t)(127 * ((int16_t)value + (int16_t)currently_active_deadzone) / (127 - (int16_t)currently_active_deadzone));
    if (value > currently_active_deadzone) return (int8_t)(127 * ((int16_t)value - (int16_t)currently_active_deadzone) / (127 - (int16_t)currently_active_deadzone));
    return 0;
}

int8_t input_right_stick_y_relative(int player_id) {
    (void)player_id;
    return 0;
}

int input_check_cheat_buffer(int n_inputs, const uint16_t* inputs_to_check) {\
    int match = button_pressed_this_frame;
    for (int i = 0; i < n_inputs; ++i) {
        if (inputs_to_check[i] != input_buffer[i]) {
            match = 0;
        }
    }
    return match;
}

void input_rumble(uint8_t left_strength, uint8_t right_enable) {
    (void)left_strength;
    (void)right_enable;
}

int input_mouse_connected(void) {
    return keyboard_focus;
}

void input_lock_mouse(void) {
    mouse_lock = 1;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void input_unlock_mouse(void) {
    mouse_lock = 0;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}
