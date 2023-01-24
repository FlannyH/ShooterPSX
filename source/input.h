#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>

void input_init();
void input_update();
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

#endif