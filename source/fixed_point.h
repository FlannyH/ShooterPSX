#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>
#include <stdio.h>

#include "common.h"

typedef int32_t fixed20_12_t;
typedef fixed20_12_t scalar_t;

// Let's hope and pray that this will be compile-time evaluated
static fixed20_12_t scalar_from_float(const float a) {
    fixed20_12_t result;
    result = (int32_t)(a * 4096.0f);
    return result;
}

static void scalar_debug(const scalar_t a) {
    if (a == INT32_MAX) {
        printf("+inf\n");
        return;
    }
    if (a == -INT32_MAX) {
        printf("-inf\n");
        return;
    }
    printf("%0.3f\n", ((float)a) / 4096.0f);
}

#ifdef _PSX
#include <psxgte.h>
#else
#include "math.h"
#endif
#include <stdlib.h>

static struct {
    int overflow : 1;
} operator_flags;

static fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = ((int64_t)(a >> 6) * ((int64_t)b >> 6));

    // overflow check
    operator_flags.overflow = 0;
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
        operator_flags.overflow = 1;
        //WARN_IF("overflow occured during scalar_mul", 1);
    }
    else if (result32 < -INT32_MAX) {
        result32 = -INT32_MAX;
        operator_flags.overflow = 1;
        //WARN_IF("overflow occured during scalar_mul", 1);
    }
    return (fixed20_12_t)result32;
}

static fixed20_12_t scalar_div(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = (int64_t)a << 12;
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

static fixed20_12_t scalar_min(const fixed20_12_t a, const fixed20_12_t b) {
    return (a < b) ? a : b;
}

static fixed20_12_t scalar_max(const fixed20_12_t a, const fixed20_12_t b) {
    return (a > b) ? a : b;
}

static fixed20_12_t scalar_sqrt(fixed20_12_t a) {
#ifdef _PSX
    return SquareRoot12(a);
#else
    return scalar_from_float(sqrtf((float)a / 4096.0f));
#endif
}

static fixed20_12_t scalar_abs(fixed20_12_t a) {
    if (a < 0) {
        a = -a;
    }
    return a;
}

static fixed20_12_t scalar_clamp(fixed20_12_t a, const fixed20_12_t min, const fixed20_12_t max) {
    if (a < min) {
        a = min;
    }
    else if (a > max) {
        a = max;
    }
    return a;
}

static int is_infinity(const fixed20_12_t a) {
    return (a == INT32_MAX || a == -INT32_MAX);
}

#endif // FIXED_POINT_H
