#ifndef WIN_GTE_H
#define WIN_GTE_H

// This entire document is a duct tape fix for the Windows debug version
#ifdef _WINDOWS
#include <stdint.h>
typedef struct {
    int32_t vx, vy, vz;
} VECTOR;

int hisin(int a);
int hicos(int a);
int FntPrint(int id, const char* fmt, ...);
char* FntFlush(int id);

#define FntPrint(a, b) {}

typedef enum {
    // Standard pads, analog joystick, Jogcon
    PAD_SELECT = 1 << 0,
    PAD_L3 = 1 << 1,
    PAD_R3 = 1 << 2,
    PAD_START = 1 << 3,
    PAD_UP = 1 << 4,
    PAD_RIGHT = 1 << 5,
    PAD_DOWN = 1 << 6,
    PAD_LEFT = 1 << 7,
    PAD_L2 = 1 << 8,
    PAD_R2 = 1 << 9,
    PAD_L1 = 1 << 10,
    PAD_R1 = 1 << 11,
    PAD_TRIANGLE = 1 << 12,
    PAD_CIRCLE = 1 << 13,
    PAD_CROSS = 1 << 14,
    PAD_SQUARE = 1 << 15,

    // Mouse
    MOUSE_LEFT = 1 << 10,
    MOUSE_RIGHT = 1 << 11,

    // neGcon
    NCON_START = 1 << 3,
    NCON_UP = 1 << 4,
    NCON_RIGHT = 1 << 5,
    NCON_DOWN = 1 << 6,
    NCON_LEFT = 1 << 7,
    NCON_R = 1 << 8,
    NCON_B = 1 << 9,
    NCON_A = 1 << 10,

    // Guncon
    GCON_A = 1 << 3,
    GCON_TRIGGER = 1 << 13,
    GCON_B = 1 << 14
} PadButton;

#else
#endif
#endif