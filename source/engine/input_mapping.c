#include "input_mapping.h"
#include "input.h"
#include <stdio.h>
#include <string.h>

#define MAX_ID_COUNT 128
#define MAX_MAPPING_COUNT 8

typedef enum {
    INPUT_TYPE_NONE = 0,
    INPUT_TYPE_GAMEPAD,
    INPUT_TYPE_KEYBOARD,
    INPUT_TYPE_MOUSE,
} input_type_t;

typedef struct {
    input_type_t type;
    scalar_t scale;
    union {
        input_gamepad_t gamepad;
        input_keyboard_t keyboard;
        input_mouse_t mouse;
    };
} input_t;

typedef struct {
    input_t inputs[MAX_MAPPING_COUNT];
    size_t n_inputs;
} input_mapping_t;

input_mapping_t input_map[MAX_ID_COUNT];

void input_mapping_init(void) {
    memset(&input_map, 0, sizeof(input_map));
}

void register_mapping(int id, input_t input) {
    const size_t index = input_map[id].n_inputs++;

    if (index >= MAX_MAPPING_COUNT) {
        printf("ignoring gamepad input mapping %i, ran out of slots\n", id);
    }

    input_map[id].inputs[index] = input;
}

void input_mapping_register_gamepad(int id, input_gamepad_t input, scalar_t scale) {
    register_mapping(id, (input_t){
        .type = INPUT_TYPE_GAMEPAD,
        .gamepad = input,
        .scale = scale
    });
}

void input_mapping_register_keyboard(int id, input_keyboard_t input, scalar_t scale) {
    register_mapping(id, (input_t){
        .type = INPUT_TYPE_KEYBOARD,
        .keyboard = input,
        .scale = scale
    });
}

void input_mapping_register_mouse(int id, input_mouse_t input, scalar_t scale) {
    register_mapping(id, (input_t){
        .type = INPUT_TYPE_MOUSE,
        .mouse = input,
        .scale = scale
    });
}

scalar_t input_mapping_value(int id, int player_id) {
    scalar_t result = 0;

    for (size_t i = 0; i < input_map[id].n_inputs; ++i) {
        const input_t *const input = &input_map[id].inputs[i];

        if ((input->type != INPUT_TYPE_GAMEPAD) && (player_id != 0)) continue;

        switch (input->type) {
            case INPUT_TYPE_GAMEPAD: result += scalar_mul(input_value_gamepad(input->gamepad, player_id), input->scale); break;
            case INPUT_TYPE_KEYBOARD: result += scalar_mul(input_value_keyboard(input->keyboard), input->scale); break;
            case INPUT_TYPE_MOUSE: result += scalar_mul(input_value_mouse(input->mouse), input->scale); break;
            default: break;
        }
    }

    return result;
}

scalar_t input_mapping_value_prev(int id, int player_id) {
    scalar_t result = 0;

    for (size_t i = 0; i < input_map[id].n_inputs; ++i) {
        const input_t *const input = &input_map[id].inputs[i];

        if ((input->type != INPUT_TYPE_GAMEPAD) && (player_id != 0)) continue;

        switch (input->type) {
            case INPUT_TYPE_GAMEPAD: result += scalar_mul(input_value_gamepad_prev(input->gamepad, player_id), input->scale); break;
            case INPUT_TYPE_KEYBOARD: result += scalar_mul(input_value_keyboard_prev(input->keyboard), input->scale); break;
            case INPUT_TYPE_MOUSE: result += scalar_mul(input_value_mouse_prev(input->mouse), input->scale); break;
            default: break;
        }
    }

    return result;
}

int input_mapping_pressed(int id, int player_id) {
    const int curr_held = (input_mapping_value(id, player_id) >= SCALAR(0.5));
    const int prev_not = (input_mapping_value_prev(id, player_id) < SCALAR(0.5));
    return (curr_held && prev_not);
}

int input_mapping_held(int id, int player_id) {
    const int curr_held = (input_mapping_value(id, player_id) >= SCALAR(0.5));
    const int prev_too = (input_mapping_value_prev(id, player_id) >= SCALAR(0.5));
    return (curr_held && prev_too);
}

int input_mapping_released(int id, int player_id) {
    const int prev_held = (input_mapping_value_prev(id, player_id) >= SCALAR(0.5));
    const int curr_not = (input_mapping_value(id, player_id) < SCALAR(0.5));
    return (prev_held && curr_not);
}
