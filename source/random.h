#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

static uint32_t random_state = 0x26082023;
static uint32_t random_u32(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static int32_t random_range(int32_t min_inclusive, int32_t max_exclusive) {
    return ((int32_t)random_u32() % (max_exclusive - min_inclusive) + min_inclusive);
}

#endif
