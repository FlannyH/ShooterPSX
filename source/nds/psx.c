#ifndef PSX_H
#define PSX_H
#include "psx.h"

#include <nds/arm9/trig_lut.h>

// Angle value: -65536 to +65535
int hisin(const int a) {
    // sinLerp takes in a value from -32768 to +32767
    // sinLerp returns a 4.12 fixed point number, same as PSX
    return (int)sinLerp((s16)(a / 4));
}

// Angle value: -65536 to +65535
int hicos(const int a) {
    // cosLerp takes in a value from -32768 to +32767
    // cosLerp returns a 4.12 fixed point number, same as PSX
    return (int)cosLerp((s16)(a / 4));
}

// Angle value: -2048 to +2047
int isin(const int a) {
    // cosLerp takes in a value from -32768 to +32767
    // cosLerp returns a 4.12 fixed point number, same as PSX
    return (int)cosLerp((s16)(a * 8));
}

// Angle value: -2048 to +2047
int icos(const int a) {
    // cosLerp takes in a value from -32768 to +32767
    // cosLerp returns a 4.12 fixed point number, same as PSX
    return (int)cosLerp((s16)(a * 8));
}

char* FntFlush(int id) {
    (void)id;
    return 0;
}

#endif
