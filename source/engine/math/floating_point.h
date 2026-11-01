#ifndef FLOATING_POINT_H
#define FLOATING_POINT_H

#ifdef __cplusplus
extern "C" {
#endif

/// doc: global: This file defines math functions for the typical floating point number format.
/// doc: global: Having the option to switch to floats can be a useful feature for debugging. If an algorithm works in floating point mode, but it doesn't work in fixed point mode, you at least know the issue is likely in the math code, not the algorithm or whatever mathy stuff you're trying to implement. Think of it as a ground truth

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
/// doc: desc: In fixed point, this definition would be what integer value corresponds to 1.0. Since this is a floating point format, we can just put 1.0 here
#define ONE FLOAT_ONE

/// doc: desc: Convert any integer or float to a floating point scalar
#define SCALAR(a) ((float)(a))

/// doc: desc: Return the value of `a` as floating point. This function exists to allow multiple scalar formats, whether it be a fixed point format, or floats
static inline scalar_t fixed_to_scalar(int32_t a) {
    return (float)a / FIXED_ONE;
}

/// doc: desc: Return the value of `scalar`, rounded to the nearest integer
static inline int int_from_scalar(scalar_t scalar) {
    return (int)roundf(scalar);
}

/// doc: desc: Print the value of `a` to the console, with 3 decimals, without adding a newline character at the end
static inline void print_scalar(scalar_t a) {
    printf("%.3f", a);
}

/// doc: desc: Print a formatted scalar value with 3 decimals
static inline void scalar_debug(const scalar_t a) {
    printf("%.3f\n", a);
}

/// doc: desc: Multiply 2 floating point numbers together.
static inline scalar_t scalar_mul(const scalar_t a, const scalar_t b) {
    return a * b;
}

/// doc: desc: Calculate `a / b` in floating point.
/// doc: desc: If `b` is equal to zero, the function returns positive or negative infinity depending on the sign of `a`
static inline scalar_t scalar_div(const scalar_t a, const scalar_t b) {
    return a / b;
}

/// doc: desc: Return the lowest value of `a` and `b`
static inline scalar_t scalar_min(const scalar_t a, const scalar_t b) {
    return (a < b) ? a : b;
}

/// doc: desc: Return the highest value of `a` and `b`
static inline scalar_t scalar_max(const scalar_t a, const scalar_t b) {
    return (a > b) ? a : b;
}

/// doc: desc: Return the square root of a floating point number. If the value is negative, this function returns 0
static inline scalar_t scalar_sqrt(scalar_t a) {
    if (a < 0) return 0;
    return sqrtf(a);
}

/// doc: desc: Return the absolute value of a floating point number so that it's never negative
static inline scalar_t scalar_abs(scalar_t a) {
    if (a < 0) {
        a = -a;
    }
    return a;
}

/// doc: desc: If `a` is less than `min`, return `min`.
/// doc: desc: If `a` is greater than `max`, return `max`
/// doc: desc: Otherwise, just return `a`
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

/// doc: desc: Return `a` linearly interpolated to `b`, with `t` typically going from 0 to ONE, but you can extrapolate if you go outside that range
static inline scalar_t scalar_lerp(const scalar_t a, const scalar_t b, const scalar_t t) {
	return a + scalar_mul(b-a, t);
}

/// doc: desc: Equivalent to `a * (1 << amount)`
static inline scalar_t scalar_shift_left(const scalar_t a, uint32_t amount) {
    return ldexpf(a, +((int)amount));
}

/// doc: desc: Equivalent to `a / (1 << amount)`
static inline scalar_t scalar_shift_right(const scalar_t a, uint32_t amount) {
    return ldexpf(a, -((int)amount));
}

/// doc: desc: Returns 1 if `a` is infinity, returns 0 otherwise
static inline int is_infinity(const scalar_t a) {
    return isinf(a);
}

// doc: desc: Returns the sine of `angle`, where an angle of `1.0` = 360 degrees = 2pi radians
static inline scalar_t trig_sin(scalar_t angle) {
    return sinf(angle * (2 * 3.14159265359f));
}

// doc: desc: Returns the cosine of `angle`, where an angle of `1.0` = 360 degrees = 2pi radians
static inline scalar_t trig_cos(scalar_t angle) {
    return cosf(angle * (2 * 3.14159265359f));
}

// doc: desc: serializes and writes a `scalar` to `destination`
// doc: desc: Returns number of bytes written
static inline size_t serialize_scalar(void* destination, scalar_t scalar) {
    int32_t fixed_point_20_12 = (int32_t)(roundf(scalar * FIXED_ONE));
    memcpy(destination, &fixed_point_20_12, sizeof(fixed_point_20_12));
    return sizeof(fixed_point_20_12);
}

// doc: desc: reads and deserializes a scalar, and then returns that scalar
// doc: desc: Returns number of bytes written
// doc: param(void* source): Where to read the serialized scalar from
// doc: param(scalar_t* scalar): Where to store the deserialized scalar to
static inline size_t deserialize_scalar(const void *const source, scalar_t* scalar) {
    int32_t fixed_point_20_12 = 0;
    memcpy(&fixed_point_20_12, source, sizeof(fixed_point_20_12));
    *scalar = ((scalar_t)fixed_point_20_12) / (1 << 12);
    return sizeof(*scalar);
}

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif

#endif // FLOATING_POINT_H
