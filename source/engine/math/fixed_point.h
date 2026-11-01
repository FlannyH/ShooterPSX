#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#ifdef __cplusplus
extern "C" {
#endif

/// doc: global: This file defines math functions for a 20.12 fixed point number format

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include "../lut.h"

#include <stdint.h>
#include <stdio.h>

typedef int32_t fixed20_12_t;
typedef fixed20_12_t scalar_t;

#ifdef ONE
#undef ONE
#endif

/// doc: desc: If you read a fixed point scalar value as a raw integer, a value of 1.0 will correspond to this value
#define ONE FIXED_ONE

/// doc: desc: Convert any integer or float to a fixed point scalar, rounding to the closest value it can represent
#define SCALAR(a) (scalar_t)((a) * ONE)

/// doc: desc: Return the value of `a` as fixed point, which it already is in this case. This function exists to allow implementing other scalar formats, whether it be a different fixed point format, or floats
static inline scalar_t fixed_to_scalar(int32_t a) {
    return a;
}

/// doc: desc: Return the value of `scalar`, rounded to the nearest integer
static inline int int_from_scalar(scalar_t scalar) {
    return (scalar + SCALAR(0.5)) / ONE;
}

/// doc: desc: Print the value of `a` to the console, with 3 decimals, without adding a newline character at the end
static inline void print_scalar(scalar_t a) {
    if (a < 0) {
        a = -a;
        printf("-");
    }
    const int32_t integer = a >> FRAC_BITS;
    const int32_t fractional = a & ((1 << FRAC_BITS) - 1);
    printf("%li.%03li", integer, (fractional * 1000) / (1 << FRAC_BITS));
}

/// doc: desc: Print a formatted scalar using `print_scalar`, catching infinity scalars and adding a newline at the end
static inline void scalar_debug(const scalar_t a) {
    if (a == INT32_MAX) {
        printf("+inf\n");
        return;
    }
    if (a == -INT32_MAX) {
        printf("-inf\n");
        return;
    }
    print_scalar(a);
    printf("\n");
}

#include <stdlib.h>

#ifdef _PSX
/// doc: desc: Multiply 2 fixed point numbers together.
/// doc: desc: This is the PS1 hand-optimized version, and will only work on PS1.
/// doc: desc: It treats the scalar values as raw integers, multiplies the two to a 64 bit register, then shifts out the result by 12 bits to make it a fixed point multiplication
static inline fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    const int64_t prod64 = (int64_t)a * (int64_t)b;
    return prod64 / ONE;
}
#else
/// doc: desc: Multiply 2 fixed point numbers together.
/// doc: desc: This is the general version, which is portable across platforms
static inline fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t a64 = (int64_t)a;
    int64_t b64 = (int64_t)b;
    int64_t a_mul_b = a64 * b64;
    int64_t result32 = a_mul_b / ONE;

// #ifdef _DEBUG
    // overflow check
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
        // printf("scalar_mul resulted in infinity, check overflow!\n");
    }
    else if (result32 < -INT32_MAX) {
        result32 = -INT32_MAX;
        // printf("scalar_mul resulted in infinity, check overflow!\n");
    }

    // precision check
    if ((result32 < SCALAR(0.01)) && (result32 > SCALAR(-0.01)) && a != 0 && b != 0) {
        // printf("potential precision problem\n");
    }
// #endif

    return (fixed20_12_t)result32;
}
#endif

/// doc: desc: Calculate `a / b` in fixed point, calculating `(a * ONE) / b` with the raw integers
static inline fixed20_12_t scalar_div(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = ((int64_t)a) * ONE;
    if (b != 0) {
        result32 /= b;
    }
    else {
        result32 |= INT32_MAX;
        // WARN_IF("division by zero occured in scalar_div", 1);
    }
    // WARN_IF("division result returned zero but the dividend is not zero, possible lack of precision", result32 == 0 && a != 0);
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
    }
    else if (result32 < INT32_MIN) {
        result32 = INT32_MIN;
    }
    return (int32_t)result32;
}

/// doc: desc: Return the lowest value of `a` and `b`
static inline fixed20_12_t scalar_min(const fixed20_12_t a, const fixed20_12_t b) {
    return (a < b) ? a : b;
}

/// doc: desc: Return the highest value of `a` and `b`
static inline fixed20_12_t scalar_max(const fixed20_12_t a, const fixed20_12_t b) {
    return (a > b) ? a : b;
}

static inline int leading_zeros(int y) {
    int zeros = 0;

    #ifdef _PSX
        __asm__ (
            "mtc2 %1, $30 \n\t"
            "nop \n\t"
            "nop \n\t"
            "mfc2 %0, $31 \n\t"
            : "=&r"(zeros)
            : "r"(y)
        );
    #else
        if (y > 0) zeros = __builtin_clz(y);
        else if (y < 0) zeros = __builtin_clz((-y) - 1);
    #endif

    return zeros;
}

static inline int int_sqrt(int y) {
    if (y <= 0) return 0;

    int op = y;
    int result = 0;
    int zeros = leading_zeros(op);

    int one = 1 << ((31u - zeros) & ~1u);

    while (one != 0) {
        int delta_square = result + one;
        if (op >= delta_square) {
            op -= delta_square;
            result += one << 1;
        }
        result >>= 1;
        one >>= 2;
    }

    return result;
}

/// doc: desc: Return the square root of a fixed point number. If the value is negative, this function returns 0
static inline fixed20_12_t scalar_sqrt(fixed20_12_t a) {
    if (a <= 0) return 0;

    int op = (int)a;
    int result = 0;
    int zeros = leading_zeros(op);

    /*
     * 11111....1111 - invalid
     * 01111....1111 - 1 - shift right 1x
     * 00111....1111 - 2 - what we want
     * 00011....1111 - 3 - shift left 1x
     * 00001....1111 - 4 - shift left 2x, etc
     * shift has to be multiples of 2, because (1 << n)^2 == 1 << 2n, so sqrt(1 << 2n) = (1 << n)
     */

    int shifted_right_at_start = 0;

    if (zeros < 2) {
        shifted_right_at_start = 2;
        op >>= 2;
    }
    else if (zeros > 2) {
        shifted_right_at_start = -((zeros - 2) & ~0x01);
        op <<= -shifted_right_at_start;
    }
    zeros = 2;

    int one = 1 << ((31u - zeros) & ~1u);;

    while (one != 0) {
        int delta_square = result + one;
        if (op >= delta_square) {
            op -= delta_square;
            result += one << 1;
        }
        result >>= 1;
        one >>= 2;
    }

    shifted_right_at_start += FRAC_BITS;
    if (shifted_right_at_start < 0) {
        int final_right_shift = (-shifted_right_at_start >> 1);
        result >>= final_right_shift;
    }
    if (shifted_right_at_start > 0) {
        int final_left_shift = (shifted_right_at_start >> 1);
        result <<= final_left_shift;
    }
    return result;
}

/// doc: desc: Return the absolute value of a fixed point number so that it's never negative
static inline fixed20_12_t scalar_abs(fixed20_12_t a) {
    if (a < 0) {
        a = -a;
    }
    return a;
}

/// doc: desc: If `a` is less than `min`, return `min`.
/// doc: desc: If `a` is greater than `max`, return `max`
/// doc: desc: Otherwise, just return `a`
static inline fixed20_12_t scalar_clamp(fixed20_12_t a, const fixed20_12_t min, const fixed20_12_t max) {
    if (a < min) {
        a = min;
    }
    else if (a > max) {
        a = max;
    }
    return a;
}

/// doc: desc: Return `a` linearly interpolated to `b`, with `t` typically going from 0 to ONE, but you can extrapolate if you go outside that range
static inline fixed20_12_t scalar_lerp(const fixed20_12_t a, const fixed20_12_t b, const fixed20_12_t t) {
	return a + scalar_mul(b-a, t);
}

/// doc: desc: Shift `a` left by `shift` bits. Equivalent to `a * (1 << amount)`
static inline fixed20_12_t scalar_shift_left(const fixed20_12_t a, uint32_t amount) {
    return a << amount;
}

/// doc: desc: Shift `a` right by `shift` bits. Equivalent to `a / (1 << amount)`
static inline fixed20_12_t scalar_shift_right(const fixed20_12_t a, uint32_t amount) {
    return a >> amount;
}

/// doc: desc: Returns 1 if `a` is infinity, which in fixed point means positive or negative INT32_MAX. Returns 0 otherwise
static inline int is_infinity(const fixed20_12_t a) {
    return (a == INT32_MAX || a == -INT32_MAX);
}

// doc: desc: Returns the sine of `angle`, where an angle of `1.0` = 360 degrees = 2pi radians
static inline scalar_t trig_sin(scalar_t angle) {
    // |  XXX  |   |
    // |XX | XX|   |
    // X - - - X - - -
    // |   |   |XX | XX
    // |   |   |  XXX
    // 0   1   2   3
    const scalar_t angle_wrapped = angle & (ONE - 1);
    const size_t lut_size = sizeof(lut_quarter_sine) / sizeof(lut_quarter_sine[0]);
    const size_t lut_entry = ((angle_wrapped & ((ONE / 4) - 1)) * lut_size) / (ONE / 4);
    const size_t quadrant = angle_wrapped >> (FRAC_BITS - 2);

    if      (quadrant == 0)     return +(lut_quarter_sine[lut_entry]       / (65536 / ONE));
    else if (quadrant == 1)     return +(lut_quarter_sine[255 - lut_entry] / (65536 / ONE));
    else if (quadrant == 2)     return -(lut_quarter_sine[lut_entry]       / (65536 / ONE));
    else /*if (quadrant == 3)*/ return -(lut_quarter_sine[255 - lut_entry] / (65536 / ONE));
}

// doc: desc: Returns the cosine of `angle`, where an angle of `1.0` = 360 degrees = 2pi radians
static inline scalar_t trig_cos(scalar_t angle) {
    // XX  |   |   |  X
    // | XX|   |   |XX
    // - - X - - - X -
    // |   |XX | XX|
    // |   |  XXX  |
    // 0   1   2   3
    return trig_sin(angle + SCALAR(0.25));
}

// doc: desc: serializes and writes a `scalar` to `destination`
// doc: desc: Returns number of bytes written
static inline size_t serialize_scalar(void* destination, scalar_t scalar) {
    union {
        int32_t fixed_20_12;
        uint8_t bytes[4];
    } data;
    data.fixed_20_12 = scalar;

    #if (FRAC_BITS < 12)
    data.fixed_20_12 <<= (12 - FRAC_BITS);
    #else
    data.fixed_20_12 >>= (FRAC_BITS - 12);
    #endif

    ((uint8_t*)destination)[0] = data.bytes[0];
    ((uint8_t*)destination)[1] = data.bytes[1];
    ((uint8_t*)destination)[2] = data.bytes[2];
    ((uint8_t*)destination)[3] = data.bytes[3];

    return sizeof(scalar);
}

// doc: desc: reads and deserializes a scalar, and then returns that scalar
// doc: desc: Returns number of bytes written
// doc: param(void* source): Where to read the serialized scalar from
// doc: param(void* source): Where to store the deserialized scalar to
static inline size_t deserialize_scalar(const void *const source, scalar_t* scalar) {
    union {
        int32_t fixed_20_12;
        uint8_t bytes[4];
    } data;
    data.fixed_20_12 = 0;

    data.bytes[0] = ((uint8_t*)source)[0];
    data.bytes[1] = ((uint8_t*)source)[1];
    data.bytes[2] = ((uint8_t*)source)[2];
    data.bytes[3] = ((uint8_t*)source)[3];

    #if (FRAC_BITS < 12)
    data.fixed_20_12 >>= (12 - FRAC_BITS);
    #elif (FRAC_BITS > 12)
    data.fixed_20_12 <<= (FRAC_BITS - 12);
    #endif

    *scalar = data.fixed_20_12;
    return sizeof(*scalar);
}

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif

#endif // FIXED_POINT_H
