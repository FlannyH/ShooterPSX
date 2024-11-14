#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include "common.h"

#include <stdint.h>
#include <stdio.h>

typedef int32_t fixed20_12_t;
typedef fixed20_12_t scalar_t;

// Let's hope and pray that this will be compile-time evaluated
ALWAYS_INLINE fixed20_12_t scalar_from_float(const float a) {
    fixed20_12_t result;
    result = (int32_t)(a * 4096.0f);
    return result;
}

ALWAYS_INLINE void print_fixed_point(scalar_t a) {
    if (a < 0) {
        a = -a;
        printf("-");
    }
    const int n_fractional_bits = 12;
    const int32_t integer = a >> n_fractional_bits;   
    const int32_t fractional = a & ((1 << n_fractional_bits) - 1);
    printf("%li.%03li", integer, (fractional * 1000) / (1 << n_fractional_bits));
}

ALWAYS_INLINE void scalar_debug(const scalar_t a) {
    if (a == INT32_MAX) {
        printf("+inf\n");
        return;
    }
    if (a == -INT32_MAX) {
        printf("-inf\n");
        return;
    }
    print_fixed_point(a);
    printf("\n");
}

#ifdef _PSX
#include <psxgte.h>
#else
#include "math.h"
#endif
#include <stdlib.h>

#ifdef _PSX
ALWAYS_INLINE static fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    register fixed20_12_t temp, result;
    __asm__ volatile (
        // shift the bits into the right place
        // ? = unknown
        // s = sign bit
        // o = high bits to discard
        // . low bits to discard
        // S = useful integer bits in high byte with sign
        // H = useful integer bits in high byte
        // h = useful integer bits in high byte but with incorrect sign
        // L = useful integer bits in low byte
        // f = fractional bits
        "mult %2, %3             \n\t" // hi, low = a*b 

        // --------------------------- // writes to r1 ----------------- | writes to r0 --------------------| 
        "mfhi %0                 \n\t" // _                              |   r0 = temp = hi                 |
        "srl  %1, %0, 31         \n\t" // r0[soooohHH] -> r1[       s]   |                                  |
        "sll  %1, %1, 31         \n\t" // r1[       s] -> r1[s       ]   |                                  |
        "andi %0, %0, 0x07FF     \n\t" // _                              |   r0[soooohHH] -> r0[     HHH]   |
        "sll  %0, %0, 20         \n\t" // _                              |   r0[     HHH] -> r0[HHH     ]   |
        "or   %1, %1, %0         \n\t" // r1[s       ] -> r1[SHH     ]   | -------------------------------- |
        "mflo %0                 \n\t" // _                              |   r0 = temp = low                |
        "srl  %0, %0, 12         \n\t" // _                              |   r0[LLrrr...] -> r0[   LLfff]   |
        "or   %1, %1, %0         \n\t" // r1[SHH     ] -> r1[SHHLLfff]   |   _                              |
        // --------------------------- // ------------------------------ | ---------------------------------| 
        : "=&r"(temp), "=&r"(result)
        : "r"(a), "r"(b)
    );
    return result;
}
#else
ALWAYS_INLINE static fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = ((int64_t)a * ((int64_t)b)) >> 12;

#ifdef _DEBUG
    // overflow check
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
    }
    else if (result32 < -INT32_MAX) {
        result32 = -INT32_MAX;
    }
#endif

    return (fixed20_12_t)result32;
}
#endif

ALWAYS_INLINE fixed20_12_t scalar_div(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = (int64_t)a * 4096;
    if (b != 0) {
        result32 /= b;
    }
    else {
        result32 |= INT32_MAX;
        //WARN_IF("division by zero occured in scalar_div", 1);
    }
    //WARN_IF("division result returned zero but the dividend is not zero, possible lack of precision", result32 == 0 && a != 0);
    return (int32_t)result32;
}

ALWAYS_INLINE fixed20_12_t scalar_min(const fixed20_12_t a, const fixed20_12_t b) {
    return (a < b) ? a : b;
}

ALWAYS_INLINE fixed20_12_t scalar_max(const fixed20_12_t a, const fixed20_12_t b) {
    return (a > b) ? a : b;
}

ALWAYS_INLINE fixed20_12_t scalar_sqrt(fixed20_12_t a) {
#ifdef _PSX
    return SquareRoot12(a);
#else
    return scalar_from_float(sqrtf((float)a / 4096.0f));
#endif
}

ALWAYS_INLINE fixed20_12_t scalar_abs(fixed20_12_t a) {
    if (a < 0) {
        a = -a;
    }
    return a;
}

ALWAYS_INLINE fixed20_12_t scalar_clamp(fixed20_12_t a, const fixed20_12_t min, const fixed20_12_t max) {
    if (a < min) {
        a = min;
    }
    else if (a > max) {
        a = max;
    }
    return a;
}

ALWAYS_INLINE static fixed20_12_t scalar_lerp(const fixed20_12_t a, const fixed20_12_t b, const fixed20_12_t t) {
	return a + scalar_mul(b-a, t);
}

ALWAYS_INLINE int is_infinity(const fixed20_12_t a) {
    return (a == INT32_MAX || a == -INT32_MAX);
}

#endif // FIXED_POINT_H
