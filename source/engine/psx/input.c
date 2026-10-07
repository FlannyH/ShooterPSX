#include "../input.h"
#include "../common.h"

#include "../math/scalar.h"
#include "../math/vec2.h"

#include <psxapi.h>
#include <psxpad.h>

typedef struct {
    scalar_t up, down, left, right;
    scalar_t north, south, east, west;
    scalar_t start, select;
    scalar_t l1, r1;
    scalar_t l2, r2;
    scalar_t l3, r3;
    vec2_t stick_left;
    vec2_t stick_right;
} controller_state_t;

// ps1 stuff
uint8_t pad_buff[2][34];
PADTYPE* pad[2] = { NULL, NULL };

// my own shit
#define N_CONTROLLER 2
controller_state_t controller_prev[N_CONTROLLER] = {0};
controller_state_t controller_curr[N_CONTROLLER] = {0};

void input_init(void) {
    memset(pad_buff, 0, sizeof(pad_buff));
    InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
    StartPAD();
    ChangeClearPAD(0);
    pad[0] = ((PADTYPE*)pad_buff[0]);
    pad[1] = ((PADTYPE*)pad_buff[1]);
    memset(controller_curr, 0, sizeof(controller_curr));
    memset(controller_prev, 0, sizeof(controller_prev));
}

scalar_t stick_to_scalar(uint8_t stick_value) {
    scalar_t x  = SCALAR(stick_value); //    0.0,  128.0, 255.0
    x -= SCALAR(0x80);                 // -128.0,    0.0, 127.0
    x /= 127;                          //   -1.0079, 0.0,   1.0
    x = scalar_max(SCALAR(-1.0), x);   //   -1.0,    0.0,   1.0
    return x;
}

void input_update(void) {
    memcpy(controller_prev, controller_curr, sizeof(controller_prev));

    for (size_t i = 0; i < N_CONTROLLER; ++i) {
        if (!input_gamepad_connected(i)) continue;

        controller_curr[i].up     = (pad[i]->btn & PAD_UP) ?       0 : ONE;
        controller_curr[i].down   = (pad[i]->btn & PAD_DOWN) ?     0 : ONE;
        controller_curr[i].left   = (pad[i]->btn & PAD_LEFT) ?     0 : ONE;
        controller_curr[i].right  = (pad[i]->btn & PAD_RIGHT) ?    0 : ONE;
        controller_curr[i].north  = (pad[i]->btn & PAD_TRIANGLE) ? 0 : ONE;
        controller_curr[i].south  = (pad[i]->btn & PAD_CROSS) ?    0 : ONE;
        controller_curr[i].east   = (pad[i]->btn & PAD_CIRCLE) ?   0 : ONE;
        controller_curr[i].west   = (pad[i]->btn & PAD_SQUARE) ?   0 : ONE;
        controller_curr[i].select = (pad[i]->btn & PAD_SELECT) ?   0 : ONE;
        controller_curr[i].start  = (pad[i]->btn & PAD_START) ?   0 : ONE;
        controller_curr[i].l1     = (pad[i]->btn & PAD_L1) ?       0 : ONE;
        controller_curr[i].r1     = (pad[i]->btn & PAD_R1) ?       0 : ONE;
        controller_curr[i].l2     = (pad[i]->btn & PAD_L2) ?       0 : ONE;
        controller_curr[i].r2     = (pad[i]->btn & PAD_R2) ?       0 : ONE;
        controller_curr[i].l3     = (pad[i]->btn & PAD_L3) ?       0 : ONE;
        controller_curr[i].r3     = (pad[i]->btn & PAD_R3) ?       0 : ONE;

        controller_curr[i].stick_left.x = stick_to_scalar(pad[i]->ls_x);
        controller_curr[i].stick_left.y = stick_to_scalar(pad[i]->ls_y);
        controller_curr[i].stick_right.x = stick_to_scalar(pad[i]->rs_x);
        controller_curr[i].stick_right.y = stick_to_scalar(pad[i]->rs_y);

        // todo: mouse
    }
}

int input_gamepad_connected(int player_id) {
    return (pad[player_id]->stat == 0);
}

void input_lock_mouse(void) {}

void input_unlock_mouse(void) {}

void input_rumble(scalar_t left_strength, scalar_t right_enable) {
    (void)left_strength;
    (void)right_enable;
 //    SPI_Request *req = SPI_CreateRequest();

    // req->len              = 9;
    // req->port             = port;
    // req->callback         = callback;
    // req->pad_req.addr     = 0x01;
    // req->pad_req.cmd      = cmd;
    // req->pad_req.tap_mode = 0x00;
    // req->pad_req.motor_r  = arg1;
    // req->pad_req.motor_l  = arg2;

    // // The padding bytes must be 0xff when unlocking vibration motors.
    // memset(
    //     req->pad_req.dummy,
    //     (cmd == PAD_CMD_REQUEST_CONFIG) ? 0xff : 0x00,
    //     4
    // );
}

void input_set_gamepad_stick_deadzone(scalar_t new_deadzone) {
    (void)new_deadzone;
}

scalar_t input_value_gamepad(input_gamepad_t input, int player_id) {
    switch (input) {
        default:                          return 0;
        case INPUT_GAMEPAD_UP:            return controller_curr[player_id].up;
        case INPUT_GAMEPAD_DOWN:          return controller_curr[player_id].down;
        case INPUT_GAMEPAD_LEFT:          return controller_curr[player_id].left;
        case INPUT_GAMEPAD_RIGHT:         return controller_curr[player_id].right;
        case INPUT_GAMEPAD_NORTH:         return controller_curr[player_id].north;
        case INPUT_GAMEPAD_SOUTH:         return controller_curr[player_id].south;
        case INPUT_GAMEPAD_WEST:          return controller_curr[player_id].west;
        case INPUT_GAMEPAD_EAST:          return controller_curr[player_id].east;
        case INPUT_GAMEPAD_START:         return controller_curr[player_id].start;
        case INPUT_GAMEPAD_SELECT:        return controller_curr[player_id].select;
        case INPUT_GAMEPAD_L1:            return controller_curr[player_id].l1;
        case INPUT_GAMEPAD_R1:            return controller_curr[player_id].r1;
        case INPUT_GAMEPAD_L2:            return controller_curr[player_id].l2;
        case INPUT_GAMEPAD_R2:            return controller_curr[player_id].r2;
        case INPUT_GAMEPAD_L3:            return controller_curr[player_id].l3;
        case INPUT_GAMEPAD_R3:            return controller_curr[player_id].r3;
        case INPUT_GAMEPAD_STICK_LEFT_X:  return controller_curr[player_id].stick_left.x;
        case INPUT_GAMEPAD_STICK_LEFT_Y:  return controller_curr[player_id].stick_left.y;
        case INPUT_GAMEPAD_STICK_RIGHT_X: return controller_curr[player_id].stick_right.x;
        case INPUT_GAMEPAD_STICK_RIGHT_Y: return controller_curr[player_id].stick_right.y;
    }
    return 0;
}

scalar_t input_value_keyboard(input_keyboard_t input) {
    (void)input;
    return 0;
}

scalar_t input_value_mouse(input_mouse_t input) {
    (void)input;
    return 0;
}

scalar_t input_value_gamepad_prev(input_gamepad_t input, int player_id) {
    switch (input) {
        default:                          return 0;
        case INPUT_GAMEPAD_UP:            return controller_prev[player_id].up;
        case INPUT_GAMEPAD_DOWN:          return controller_prev[player_id].down;
        case INPUT_GAMEPAD_LEFT:          return controller_prev[player_id].left;
        case INPUT_GAMEPAD_RIGHT:         return controller_prev[player_id].right;
        case INPUT_GAMEPAD_NORTH:         return controller_prev[player_id].north;
        case INPUT_GAMEPAD_SOUTH:         return controller_prev[player_id].south;
        case INPUT_GAMEPAD_WEST:          return controller_prev[player_id].west;
        case INPUT_GAMEPAD_EAST:          return controller_prev[player_id].east;
        case INPUT_GAMEPAD_START:         return controller_prev[player_id].start;
        case INPUT_GAMEPAD_SELECT:        return controller_prev[player_id].select;
        case INPUT_GAMEPAD_L1:            return controller_prev[player_id].l1;
        case INPUT_GAMEPAD_R1:            return controller_prev[player_id].r1;
        case INPUT_GAMEPAD_L2:            return controller_prev[player_id].l2;
        case INPUT_GAMEPAD_R2:            return controller_prev[player_id].r2;
        case INPUT_GAMEPAD_L3:            return controller_prev[player_id].l3;
        case INPUT_GAMEPAD_R3:            return controller_prev[player_id].r3;
        case INPUT_GAMEPAD_STICK_LEFT_X:  return controller_prev[player_id].stick_left.x;
        case INPUT_GAMEPAD_STICK_LEFT_Y:  return controller_prev[player_id].stick_left.y;
        case INPUT_GAMEPAD_STICK_RIGHT_X: return controller_prev[player_id].stick_right.x;
        case INPUT_GAMEPAD_STICK_RIGHT_Y: return controller_prev[player_id].stick_right.y;
    }
    return 0;
}

scalar_t input_value_keyboard_prev(input_keyboard_t input) {
    (void)input;
    return 0;
}

scalar_t input_value_mouse_prev(input_mouse_t input) {
    (void)input;
    return 0;
}
