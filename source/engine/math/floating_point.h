#ifndef FLOATING_POINT_H
#define FLOATING_POINT_H

#ifdef __cplusplus
extern "C" {
#endif


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"


#include "scalar.h"
#include "../lut.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef float scalar_t;

#ifdef ONE
#undef ONE
#endif
#define ONE FLOAT_ONE

#define SCALAR(a) ((float)(a))

static inline scalar_t fixed_to_scalar(int32_t a) {
    return (float)a / FIXED_ONE;
}

static inline int int_from_scalar(scalar_t scalar) {
    return (int)roundf(scalar);
}

static inline void print_scalar(scalar_t a) {
    printf("%.3f", a);
}

static inline void scalar_debug(const scalar_t a) {
    printf("%.3f\n", a);
}

static inline scalar_t scalar_mul(const scalar_t a, const scalar_t b) {
    return a * b;
}

static inline scalar_t scalar_div(const scalar_t a, const scalar_t b) {
    return a / b;
}

static inline scalar_t scalar_min(const scalar_t a, const scalar_t b) {
    return (a < b) ? a : b;
}

static inline scalar_t scalar_max(const scalar_t a, const scalar_t b) {
    return (a > b) ? a : b;
}

static inline scalar_t scalar_sqrt(scalar_t a) {
    if (a < 0) return 0;
    return sqrtf(a);
}

static inline scalar_t scalar_abs(scalar_t a) {
    if (a < 0) {
        a = -a;
    }
    return a;
}

static inline scalar_t scalar_clamp(scalar_t a, const scalar_t min, const scalar_t max) {
    assert(max >= min);
    if (a < min) {
        a = min;
    }
    else if (a > max) {
        a = max;
    }
    return a;
}

static inline scalar_t scalar_lerp(const scalar_t a, const scalar_t b, const scalar_t t) {
	return a + scalar_mul(b-a, t);
}

static inline scalar_t scalar_shift_left(const scalar_t a, uint32_t amount) {
    return ldexpf(a, +((int)amount));
}

static inline scalar_t scalar_shift_right(const scalar_t a, uint32_t amount) {
    return ldexpf(a, -((int)amount));
}

static inline int is_infinity(const scalar_t a) {
    return isinf(a);
}

static inline scalar_t trig_sin(scalar_t angle) {
    return sinf(angle * (2 * 3.14159265359f));
}

static inline scalar_t trig_cos(scalar_t angle) {
    return cosf(angle * (2 * 3.14159265359f));
}

static inline size_t serialize_scalar(void* destination, scalar_t scalar) {
    int32_t fixed_point_20_12 = (int32_t)(roundf(scalar * FIXED_ONE));
    memcpy(destination, &fixed_point_20_12, sizeof(fixed_point_20_12));
    return sizeof(fixed_point_20_12);
}

static inline size_t deserialize_scalar(const void *const source, scalar_t* scalar) {
    int32_t fixed_point_20_12 = 0;
    memcpy(&fixed_point_20_12, source, sizeof(fixed_point_20_12));
    *scalar = fixed_to_scalar(fixed_point_20_12);
    return sizeof(*scalar);
}

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif

#endif // FLOATING_POINT_H
