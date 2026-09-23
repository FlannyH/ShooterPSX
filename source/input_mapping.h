#ifdef __cplusplus
extern "C" {
#endif

#include "math/fixed_point.h"
#include "input.h"

void input_mapping_init(void);

void input_mapping_register_gamepad(int id, input_gamepad_t input, scalar_t scale); // `id` can be any integer from 0 to 255. suggested strat: bring your own enum for `int id`
void input_mapping_register_keyboard(int id, input_keyboard_t input, scalar_t scale); // `id` can be any integer from 0 to 255. suggested strat: bring your own enum for `int id`
void input_mapping_register_mouse(int id, input_mouse_t input, scalar_t scale); // `id` can be any integer from 0 to 255. suggested strat: bring your own enum for `int id`

scalar_t input_mapping_value(int id, int player_id);
int input_mapping_pressed(int id, int player_id);
int input_mapping_held(int id, int player_id);
int input_mapping_released(int id, int player_id);

#ifdef __cplusplus
}
#endif
