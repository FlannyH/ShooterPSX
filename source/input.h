#ifndef INPUT_H
#define INPUT_H
#ifdef __cplusplus
extern "C" {
#endif
#include "fixed_point.h"

#include <stdint.h>
#include <stddef.h>

void input_init(void);
void input_update(void);
void input_set_stick_deadzone(int8_t new_deadzone);
int input_has_analog(int player_id);
int input_is_connected(int player_id);
int input_held(uint16_t button_mask, int player_id);
int input_pressed(uint16_t button_mask, int player_id);
int input_released(uint16_t button_mask, int player_id);
int8_t input_left_stick_x(int player_id);
int8_t input_left_stick_x_relative(int player_id);
int8_t input_left_stick_y(int player_id);
int8_t input_left_stick_y_relative(int player_id);
int8_t input_right_stick_x(int player_id);
int8_t input_right_stick_x_relative(int player_id);
int8_t input_right_stick_y(int player_id);
int8_t input_right_stick_y_relative(int player_id);
int input_check_cheat_buffer(int n_inputs, const uint16_t* inputs_to_check);
void input_rumble(uint8_t left_strength, uint8_t right_enable);
int input_mouse_connected(void);
void input_lock_mouse(void);
void input_unlock_mouse(void);
int input_mouse_movement_x(void);
int input_mouse_movement_y(void);

#ifdef __cplusplus
}
#endif
#endif
