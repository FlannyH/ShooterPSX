#ifndef INPUT_H
#define INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "math/scalar.h"

#include <stdint.h>
#include <stddef.h>

// todo(input_remap): desc: mappable controls
//                           have an action system, where external code can link an action index (which could be an
//                           enum provided by the gameplay code), to a button, whether that be keyboard, gamepad,
//                           mouse, or multiple at once
// todo(input_remap_menu): desc: menu to remap controls

typedef enum {
    INPUT_GAMEPAD_NONE = 0,
    INPUT_GAMEPAD_UP,            INPUT_GAMEPAD_DOWN,
    INPUT_GAMEPAD_LEFT,          INPUT_GAMEPAD_RIGHT,
    INPUT_GAMEPAD_NORTH,         INPUT_GAMEPAD_SOUTH,
    INPUT_GAMEPAD_WEST,          INPUT_GAMEPAD_EAST,
    INPUT_GAMEPAD_START,         INPUT_GAMEPAD_SELECT,
    INPUT_GAMEPAD_L1,            INPUT_GAMEPAD_R1,
    INPUT_GAMEPAD_L2,            INPUT_GAMEPAD_R2,
    INPUT_GAMEPAD_L3,            INPUT_GAMEPAD_R3,
    INPUT_GAMEPAD_STICK_LEFT_X,  INPUT_GAMEPAD_STICK_LEFT_Y,
    INPUT_GAMEPAD_STICK_RIGHT_X, INPUT_GAMEPAD_STICK_RIGHT_Y,
    N_INPUT_GAMEPAD
} input_gamepad_t;

typedef enum {
    INPUT_KEY_NONE = 0,
    INPUT_KEY_ESC,         INPUT_KEY_F1,           INPUT_KEY_F2,            INPUT_KEY_F3,
    INPUT_KEY_F4,          INPUT_KEY_F5,           INPUT_KEY_F6,            INPUT_KEY_F7,
    INPUT_KEY_F8,          INPUT_KEY_F9,           INPUT_KEY_F10,           INPUT_KEY_F11,
    INPUT_KEY_F12,         INPUT_KEY_BACKTICK,     INPUT_KEY_MINUS,         INPUT_KEY_EQUALS,
    INPUT_KEY_BACKSPACE,   INPUT_KEY_INSERT,       INPUT_KEY_HOME,          INPUT_KEY_PAGE_UP,
    INPUT_KEY_TAB,         INPUT_KEY_BRACKET_OPEN, INPUT_KEY_BRACKET_CLOSE, INPUT_KEY_BACKSLASH,
    INPUT_KEY_CAPS_LOCK,   INPUT_KEY_SEMICOLON,    INPUT_KEY_QUOTE,         INPUT_KEY_ENTER,
    INPUT_KEY_LEFT_SHIFT,  INPUT_KEY_COMMA,        INPUT_KEY_DOT,           INPUT_KEY_SLASH,
    INPUT_KEY_RIGHT_SHIFT, INPUT_KEY_LEFT_CTRL,    INPUT_KEY_LEFT_ALT,      INPUT_KEY_SPACE,
    INPUT_KEY_RIGHT_ALT,   INPUT_KEY_RIGHT_CTRL,   INPUT_KEY_UP,            INPUT_KEY_DOWN,
    INPUT_KEY_LEFT,        INPUT_KEY_RIGHT,        INPUT_KEY_1,             INPUT_KEY_2,
    INPUT_KEY_3,           INPUT_KEY_4,            INPUT_KEY_5,             INPUT_KEY_6,
    INPUT_KEY_7,           INPUT_KEY_8,            INPUT_KEY_9,             INPUT_KEY_0,
    INPUT_KEY_Q,           INPUT_KEY_W,            INPUT_KEY_E,             INPUT_KEY_R,
    INPUT_KEY_T,           INPUT_KEY_Y,            INPUT_KEY_U,             INPUT_KEY_I,
    INPUT_KEY_O,           INPUT_KEY_P,            INPUT_KEY_A,             INPUT_KEY_S,
    INPUT_KEY_D,           INPUT_KEY_F,            INPUT_KEY_G,             INPUT_KEY_H,
    INPUT_KEY_J,           INPUT_KEY_K,            INPUT_KEY_L,             INPUT_KEY_Z,
    INPUT_KEY_X,           INPUT_KEY_C,            INPUT_KEY_V,             INPUT_KEY_B,
    INPUT_KEY_N,           INPUT_KEY_M,
    N_INPUT_KEY
} input_keyboard_t;

typedef enum {
    INPUT_MOUSE_NONE = 0,
    INPUT_MOUSE_BUTTON_LEFT,
    INPUT_MOUSE_BUTTON_RIGHT,
    INPUT_MOUSE_BUTTON_MIDDLE,
    INPUT_MOUSE_WHEEL_X,
    INPUT_MOUSE_WHEEL_Y,
    INPUT_MOUSE_DELTA_X,
    INPUT_MOUSE_DELTA_Y,
    INPUT_MOUSE_POS_X,
    INPUT_MOUSE_POS_Y,
    N_INPUT_MOUSE
} input_mouse_t;

void input_init(void);
void input_update(void);
void input_lock_mouse(void);
void input_unlock_mouse(void);
void input_rumble(scalar_t left_strength, scalar_t right_enable);
void input_set_gamepad_stick_deadzone(scalar_t new_deadzone);
scalar_t input_value_gamepad(input_gamepad_t input, int player_id);
scalar_t input_value_keyboard(input_keyboard_t input);
scalar_t input_value_mouse(input_mouse_t input);
scalar_t input_value_gamepad_prev(input_gamepad_t input, int player_id);
scalar_t input_value_keyboard_prev(input_keyboard_t input);
scalar_t input_value_mouse_prev(input_mouse_t input);

#ifdef __cplusplus
}
#endif
#endif
