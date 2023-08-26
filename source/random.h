#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

static uint32_t random_state = 0x26082023;
static uint32_t random() {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

#endif