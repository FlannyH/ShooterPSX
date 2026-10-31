#ifdef __cplusplus
extern "C" {
#endif

#include "math/scalar.h"
#include "input.h"

#define MAX_ID_COUNT 128
#define MAX_MAPPING_COUNT 8

typedef enum {
    INPUT_TYPE_NONE = 0,
    INPUT_TYPE_GAMEPAD,
    INPUT_TYPE_KEYBOARD,
    INPUT_TYPE_MOUSE,
} input_type_t;

void input_mapping_init(void);

void input_mapping_register_gamepad(int id, input_gamepad_t input, scalar_t scale); // `id` can be any integer from 0 to 255. suggested strat: bring your own enum for `int id`
void input_mapping_register_keyboard(int id, input_keyboard_t input, scalar_t scale); // `id` can be any integer from 0 to 255. suggested strat: bring your own enum for `int id`
void input_mapping_register_mouse(int id, input_mouse_t input, scalar_t scale); // `id` can be any integer from 0 to 255. suggested strat: bring your own enum for `int id`

void input_mapping_clear(int id, input_type_t type);

scalar_t input_mapping_value(int id, int player_id);
int input_mapping_pressed(int id, int player_id);
int input_mapping_held(int id, int player_id);
int input_mapping_released(int id, int player_id);

#ifdef __cplusplus
}
#endif
