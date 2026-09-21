#ifndef CHEATS_H
#define CHEATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef _PSX
#include <psxpad.h>
#elif defined(_NDS)
#include "nds/psx.h"
#else
#include "pc/psx.h"
#endif

// Cheats are defined in reverse order, it was easier to implement that way
static const uint16_t cheat_doom_mode[] = {
    PAD_RIGHT,
    PAD_LEFT,
    PAD_RIGHT,
    PAD_LEFT,
    PAD_CIRCLE,
    PAD_SQUARE,
    PAD_CIRCLE,
    PAD_SQUARE,
};

#ifdef __cplusplus
}
#endif

#endif
