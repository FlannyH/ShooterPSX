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

inline static int32_t random_range(const int32_t min_inclusive, const int32_t max_exclusive) {
    return ((int32_t)(random_u32() & 0x7FFFFFFF) % (max_exclusive - min_inclusive) + min_inclusive);
}

#endif
