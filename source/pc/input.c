#include "input.h"
#include "GLFW/glfw3.h"
#include <string.h>

extern GLFWwindow* window;
int8_t deadzone = 24;
int mouse_lock = 0;
int mouse_lock_prev = 0;

#define MAX_CONTROLLERS 4

// callbacks write here
scalar_t state_new_gamepad[N_INPUT_GAMEPAD][MAX_CONTROLLERS];
scalar_t state_new_keyboard[N_INPUT_KEY];
scalar_t state_new_mouse[N_INPUT_MOUSE];

// then move down here the next input_update()
scalar_t state_curr_gamepad[N_INPUT_GAMEPAD][MAX_CONTROLLERS];
scalar_t state_curr_keyboard[N_INPUT_KEY];
scalar_t state_curr_mouse[N_INPUT_MOUSE];

// which then moves down here the next frame, so we can check presses and releases
scalar_t state_prev_gamepad[N_INPUT_GAMEPAD][MAX_CONTROLLERS];
scalar_t state_prev_keyboard[N_INPUT_KEY];
scalar_t state_prev_mouse[N_INPUT_MOUSE];

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window;
    (void)mods;

    // convert mouse button to our own enum
    size_t state_mouse_index = 0;
    switch (button) {
        default: return;
        case GLFW_MOUSE_BUTTON_LEFT: state_mouse_index = INPUT_MOUSE_BUTTON_LEFT; break;
        case GLFW_MOUSE_BUTTON_RIGHT: state_mouse_index = INPUT_MOUSE_BUTTON_RIGHT; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: state_mouse_index = INPUT_MOUSE_BUTTON_MIDDLE; break;
    }

    // what did we do with the button?
    switch (action) {
        default: return;
        case GLFW_PRESS: state_new_mouse[state_mouse_index] = SCALAR(1.0); break;
        case GLFW_RELEASE: state_new_mouse[state_mouse_index] = SCALAR(0.0); break;
    }
}

void scroll_callback(GLFWwindow* window, double x, double y) {
    (void)window;

    state_new_mouse[INPUT_MOUSE_WHEEL_X] += SCALAR(x);
    state_new_mouse[INPUT_MOUSE_WHEEL_Y] += SCALAR(y);
}

void mouse_pos_callback(GLFWwindow* window, double x, double y) {
    (void)window;

    state_new_mouse[INPUT_MOUSE_POS_X] = SCALAR(x);
    state_new_mouse[INPUT_MOUSE_POS_Y] = SCALAR(y);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    size_t state_keyboard_index = 0;
    switch (key) {
        case GLFW_KEY_ESCAPE: state_keyboard_index = INPUT_KEY_ESC; break;
        case GLFW_KEY_F1: state_keyboard_index = INPUT_KEY_F1; break;
        case GLFW_KEY_F2: state_keyboard_index = INPUT_KEY_F2; break;
        case GLFW_KEY_F3: state_keyboard_index = INPUT_KEY_F3; break;
        case GLFW_KEY_F4: state_keyboard_index = INPUT_KEY_F4; break;
        case GLFW_KEY_F5: state_keyboard_index = INPUT_KEY_F5; break;
        case GLFW_KEY_F6: state_keyboard_index = INPUT_KEY_F6; break;
        case GLFW_KEY_F7: state_keyboard_index = INPUT_KEY_F7; break;
        case GLFW_KEY_F8: state_keyboard_index = INPUT_KEY_F8; break;
        case GLFW_KEY_F9: state_keyboard_index = INPUT_KEY_F9; break;
        case GLFW_KEY_F10: state_keyboard_index = INPUT_KEY_F10; break;
        case GLFW_KEY_F11: state_keyboard_index = INPUT_KEY_F11; break;
        case GLFW_KEY_F12: state_keyboard_index = INPUT_KEY_F12; break;
        case GLFW_KEY_GRAVE_ACCENT: state_keyboard_index = INPUT_KEY_BACKTICK; break;
        case GLFW_KEY_MINUS: state_keyboard_index = INPUT_KEY_MINUS; break;
        case GLFW_KEY_EQUAL: state_keyboard_index = INPUT_KEY_EQUALS; break;
        case GLFW_KEY_BACKSPACE: state_keyboard_index = INPUT_KEY_BACKSPACE; break;
        case GLFW_KEY_INSERT: state_keyboard_index = INPUT_KEY_INSERT; break;
        case GLFW_KEY_HOME: state_keyboard_index = INPUT_KEY_HOME; break;
        case GLFW_KEY_PAGE_UP: state_keyboard_index = INPUT_KEY_PAGE_UP; break;
        case GLFW_KEY_TAB: state_keyboard_index = INPUT_KEY_TAB; break;
        case GLFW_KEY_LEFT_BRACKET: state_keyboard_index = INPUT_KEY_BRACKET_OPEN; break;
        case GLFW_KEY_RIGHT_BRACKET: state_keyboard_index = INPUT_KEY_BRACKET_CLOSE; break;
        case GLFW_KEY_BACKSLASH: state_keyboard_index = INPUT_KEY_BACKSLASH; break;
        case GLFW_KEY_CAPS_LOCK: state_keyboard_index = INPUT_KEY_CAPS_LOCK; break;
        case GLFW_KEY_SEMICOLON: state_keyboard_index = INPUT_KEY_SEMICOLON; break;
        case GLFW_KEY_APOSTROPHE: state_keyboard_index = INPUT_KEY_QUOTE; break;
        case GLFW_KEY_ENTER: state_keyboard_index = INPUT_KEY_ENTER; break;
        case GLFW_KEY_LEFT_SHIFT: state_keyboard_index = INPUT_KEY_LEFT_SHIFT; break;
        case GLFW_KEY_COMMA: state_keyboard_index = INPUT_KEY_COMMA; break;
        case GLFW_KEY_PERIOD: state_keyboard_index = INPUT_KEY_DOT; break;
        case GLFW_KEY_SLASH: state_keyboard_index = INPUT_KEY_SLASH; break;
        case GLFW_KEY_RIGHT_SHIFT: state_keyboard_index = INPUT_KEY_RIGHT_SHIFT; break;
        case GLFW_KEY_LEFT_CONTROL: state_keyboard_index = INPUT_KEY_LEFT_CTRL; break;
        case GLFW_KEY_LEFT_ALT: state_keyboard_index = INPUT_KEY_LEFT_ALT; break;
        case GLFW_KEY_SPACE: state_keyboard_index = INPUT_KEY_SPACE; break;
        case GLFW_KEY_RIGHT_ALT: state_keyboard_index = INPUT_KEY_RIGHT_ALT; break;
        case GLFW_KEY_RIGHT_CONTROL: state_keyboard_index = INPUT_KEY_RIGHT_CTRL; break;
        case GLFW_KEY_UP: state_keyboard_index = INPUT_KEY_UP; break;
        case GLFW_KEY_DOWN: state_keyboard_index = INPUT_KEY_DOWN; break;
        case GLFW_KEY_LEFT: state_keyboard_index = INPUT_KEY_LEFT; break;
        case GLFW_KEY_RIGHT: state_keyboard_index = INPUT_KEY_RIGHT; break;
        case GLFW_KEY_1: state_keyboard_index = INPUT_KEY_1; break;
        case GLFW_KEY_2: state_keyboard_index = INPUT_KEY_2; break;
        case GLFW_KEY_3: state_keyboard_index = INPUT_KEY_3; break;
        case GLFW_KEY_4: state_keyboard_index = INPUT_KEY_4; break;
        case GLFW_KEY_5: state_keyboard_index = INPUT_KEY_5; break;
        case GLFW_KEY_6: state_keyboard_index = INPUT_KEY_6; break;
        case GLFW_KEY_7: state_keyboard_index = INPUT_KEY_7; break;
        case GLFW_KEY_8: state_keyboard_index = INPUT_KEY_8; break;
        case GLFW_KEY_9: state_keyboard_index = INPUT_KEY_9; break;
        case GLFW_KEY_0: state_keyboard_index = INPUT_KEY_0; break;
        case GLFW_KEY_Q: state_keyboard_index = INPUT_KEY_Q; break;
        case GLFW_KEY_W: state_keyboard_index = INPUT_KEY_W; break;
        case GLFW_KEY_E: state_keyboard_index = INPUT_KEY_E; break;
        case GLFW_KEY_R: state_keyboard_index = INPUT_KEY_R; break;
        case GLFW_KEY_T: state_keyboard_index = INPUT_KEY_T; break;
        case GLFW_KEY_Y: state_keyboard_index = INPUT_KEY_Y; break;
        case GLFW_KEY_U: state_keyboard_index = INPUT_KEY_U; break;
        case GLFW_KEY_I: state_keyboard_index = INPUT_KEY_I; break;
        case GLFW_KEY_O: state_keyboard_index = INPUT_KEY_O; break;
        case GLFW_KEY_P: state_keyboard_index = INPUT_KEY_P; break;
        case GLFW_KEY_A: state_keyboard_index = INPUT_KEY_A; break;
        case GLFW_KEY_S: state_keyboard_index = INPUT_KEY_S; break;
        case GLFW_KEY_D: state_keyboard_index = INPUT_KEY_D; break;
        case GLFW_KEY_F: state_keyboard_index = INPUT_KEY_F; break;
        case GLFW_KEY_G: state_keyboard_index = INPUT_KEY_G; break;
        case GLFW_KEY_H: state_keyboard_index = INPUT_KEY_H; break;
        case GLFW_KEY_J: state_keyboard_index = INPUT_KEY_J; break;
        case GLFW_KEY_K: state_keyboard_index = INPUT_KEY_K; break;
        case GLFW_KEY_L: state_keyboard_index = INPUT_KEY_L; break;
        case GLFW_KEY_Z: state_keyboard_index = INPUT_KEY_Z; break;
        case GLFW_KEY_X: state_keyboard_index = INPUT_KEY_X; break;
        case GLFW_KEY_C: state_keyboard_index = INPUT_KEY_C; break;
        case GLFW_KEY_V: state_keyboard_index = INPUT_KEY_V; break;
        case GLFW_KEY_B: state_keyboard_index = INPUT_KEY_B; break;
        case GLFW_KEY_N: state_keyboard_index = INPUT_KEY_N; break;
        case GLFW_KEY_M: state_keyboard_index = INPUT_KEY_M; break;
    }

    switch (action) {
        default: return;
        case GLFW_PRESS: state_new_keyboard[state_keyboard_index] = SCALAR(1.0); break;
        case GLFW_RELEASE: state_new_keyboard[state_keyboard_index] = SCALAR(0.0); break;
    }
}

void joystick_callback(int jid, int event) {
    printf("joystick with jid %i triggered event %i\n", jid, event);
    // todo
}

void input_init(void) {
    memset(state_prev_gamepad, 0, sizeof(state_prev_gamepad));
    memset(state_prev_keyboard, 0, sizeof(state_prev_keyboard));
    memset(state_prev_mouse, 0, sizeof(state_prev_mouse));
    memset(state_curr_gamepad, 0, sizeof(state_curr_gamepad));
    memset(state_curr_keyboard, 0, sizeof(state_curr_keyboard));
    memset(state_curr_mouse, 0, sizeof(state_curr_mouse));
    memset(state_new_gamepad, 0, sizeof(state_new_gamepad));
    memset(state_new_keyboard, 0, sizeof(state_new_keyboard));
    memset(state_new_mouse, 0, sizeof(state_new_mouse));

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetJoystickCallback(joystick_callback);
}

void input_update(void) {
    // prev = curr
    memcpy(state_prev_gamepad, state_curr_gamepad, sizeof(state_prev_gamepad));
    memcpy(state_prev_keyboard, state_curr_keyboard, sizeof(state_prev_keyboard));
    memcpy(state_prev_mouse, state_curr_mouse, sizeof(state_prev_mouse));

    // curr = new
    memcpy(state_curr_gamepad, state_new_gamepad, sizeof(state_curr_gamepad));
    memcpy(state_curr_keyboard, state_new_keyboard, sizeof(state_curr_keyboard));
    memcpy(state_curr_mouse, state_new_mouse, sizeof(state_curr_mouse));

    state_curr_mouse[INPUT_MOUSE_DELTA_X] = state_curr_mouse[INPUT_MOUSE_POS_X] - state_prev_mouse[INPUT_MOUSE_POS_X];
    state_curr_mouse[INPUT_MOUSE_DELTA_Y] = state_curr_mouse[INPUT_MOUSE_POS_Y] - state_prev_mouse[INPUT_MOUSE_POS_Y];
}

scalar_t input_value_gamepad(input_gamepad_t input, int player_id) {
    return state_curr_gamepad[input][player_id];
}

scalar_t input_value_keyboard(input_keyboard_t input) {
    return state_curr_keyboard[input];
}

scalar_t input_value_mouse(input_mouse_t input) {
    return state_curr_mouse[input];
}

scalar_t input_value_gamepad_prev(input_gamepad_t input, int player_id) {
    return state_prev_gamepad[input][player_id];
}
scalar_t input_value_keyboard_prev(input_keyboard_t input) {
    return state_prev_keyboard[input];
}
scalar_t input_value_mouse_prev(input_mouse_t input) {
    return state_prev_mouse[input];
}

void input_lock_mouse(void) {
    mouse_lock = 1;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void input_unlock_mouse(void) {
    mouse_lock = 0;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void input_rumble(scalar_t left_strength, scalar_t right_enable) {
    // todo: move to different controller library because no rumble support
    (void)left_strength;
    (void)right_enable;
}

void input_set_gamepad_stick_deadzone(scalar_t new_deadzone) {
    deadzone = new_deadzone;
}
