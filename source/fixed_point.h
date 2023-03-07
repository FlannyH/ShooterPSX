#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>
#include <stdio.h>

#include "common.h"

typedef struct {
    union {
        struct {
            uint32_t fraction : 12;
            int32_t integer : 20;
        };
        int32_t raw;
    };
} fixed20_12_t;

typedef fixed20_12_t scalar_t;

// Let's hope and pray that this will be compile-time evaluated
static fixed20_12_t scalar_from_float(const float a) {
    fixed20_12_t result;
    result.raw = (int32_t)(a * 4096.0f);
    return result;
}

static void scalar_debug(const scalar_t a) {
    printf("%0.3f\n", ((float)a.raw) / 4096.0f);
}

#ifdef _PSX
#include <psxgte.h>
#else
#include <corecrt_math.h>
#endif
#include <stdlib.h>

static fixed20_12_t scalar_from_int32(const int32_t raw) {
    fixed20_12_t result;
    result.raw = raw;
    return result;
}

static fixed20_12_t scalar_neg(const fixed20_12_t a) {
    fixed20_12_t result = a;
    result.raw = -result.raw;
    return result;
}

static fixed20_12_t scalar_add(const fixed20_12_t a, const fixed20_12_t b) {
    fixed20_12_t result;
    result.raw = a.raw + b.raw;
    //WARN_IF("overflow/underflow occured during scalar_add", ((int64_t)a.raw + (int64_t)b.raw) > (int64_t)INT32_MAX);
    //WARN_IF("overflow/underflow occured during scalar_add", ((int64_t)a.raw + (int64_t)b.raw) < (int64_t)INT32_MIN);
    return result;
}

static fixed20_12_t scalar_sub(const fixed20_12_t a, const fixed20_12_t b) {
    fixed20_12_t result;
    result.raw = a.raw - b.raw;
    //WARN_IF("overflow/underflow occured during scalar_sub", ((int64_t)a.raw - (int64_t)b.raw) > (int64_t)INT32_MAX);
    //WARN_IF("overflow/underflow occured during scalar_sub", ((int64_t)a.raw - (int64_t)b.raw) < (int64_t)INT32_MIN);
    return result;
}

static fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = ((int64_t)a.raw * (int64_t)b.raw) >> 12;

    // overflow check
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
        //WARN_IF("overflow occured during scalar_mul", 1);
    }
    else if (result32 < -INT32_MAX) {
        result32 = -INT32_MAX;
        //WARN_IF("overflow occured during scalar_mul", 1);
    }
    return scalar_from_int32((int32_t)result32);
}

static fixed20_12_t scalar_div(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = (int64_t)a.raw << 12;
    if (b.raw != 0) {
        result32 /= b.raw;
    }
    else {
        result32 |= INT32_MAX;
        //WARN_IF("division by zero occured in scalar_div", 1);
    }
    //WARN_IF("division result returned zero but the dividend is not zero, possible lack of precision", result32 == 0 && a.raw != 0);
    fixed20_12_t result;
    result.raw = (int32_t)result32;
    return result;
}

static fixed20_12_t scalar_min(const fixed20_12_t a, const fixed20_12_t b) {
    return (a.raw < b.raw) ? a : b;
}

static fixed20_12_t scalar_max(const fixed20_12_t a, const fixed20_12_t b) {
    return (a.raw > b.raw) ? a : b;
}

static fixed20_12_t scalar_sqrt(fixed20_12_t a) {
#ifdef _PSX
    return scalar_from_int32(SquareRoot12(a.raw));
#else
    return scalar_from_float(sqrtf((float)a.raw / 4096.0f));
#endif
}

static fixed20_12_t scalar_shift_right(const fixed20_12_t a, const int shift) {
    fixed20_12_t ret;
    ret.raw = a.raw >> shift;
    return ret;
}

#endif // FIXED_POINT_H
