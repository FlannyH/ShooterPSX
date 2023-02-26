#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>
#include <stdio.h>

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

typedef struct {
    fixed20_12_t x, y, z;
} vec3_t;

// Let's hope and pray that this will be compile-time evaluated
static inline fixed20_12_t scalar_from_float(const float a) {
    fixed20_12_t result;
    result.raw = (int32_t)(a * 4096.0f);
    return result;
}

// Let's hope and cry that this will be compile-time evaluated
static inline vec3_t vec3_from_floats(const float x, const float y, const float z) {
    vec3_t result;
    result.x = scalar_from_float(x);
    result.y = scalar_from_float(y);
    result.z = scalar_from_float(z);
    return result;
}

// Debug
static inline void vec3_debug(const vec3_t a) {
    printf("%0.3f, %0.3f, %0.3f\n", ((float)a.x.raw) / 4096.0f, ((float)a.y.raw) / 4096.0f, ((float)a.z.raw) / 4096.0f);
}

static inline void scalar_debug(const scalar_t a) {
    printf("%0.3f\n", ((float)a.raw) / 4096.0f);
}



#ifdef _PSX
#include <psxgte.h>
#else
#include <corecrt_math.h>
#endif
#include <stdlib.h>


static inline fixed20_12_t scalar_from_int32(const int32_t raw) {
    fixed20_12_t result;
    result.raw = raw;
    return result;
}

static inline fixed20_12_t scalar_neg(const fixed20_12_t a) {
    fixed20_12_t result = a;
    result.raw = -result.raw;
    return result;
}

static inline fixed20_12_t scalar_add(const fixed20_12_t a, const fixed20_12_t b) {
    fixed20_12_t result;
    result.raw = a.raw + b.raw;
    return result;
}

static inline fixed20_12_t scalar_sub(const fixed20_12_t a, const fixed20_12_t b) {
    fixed20_12_t result;
    result.raw = a.raw - b.raw;
    return result;
}

static inline fixed20_12_t scalar_mul(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = ((int64_t)a.raw * (int64_t)b.raw) >> 12;

    // overflow check
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
    }
    else if (result32 < -INT32_MAX) {
        result32 = -INT32_MAX;
    }
    return scalar_from_int32((int32_t)result32);
}

static inline fixed20_12_t scalar_div(const fixed20_12_t a, const fixed20_12_t b) {
    int64_t result32 = (int64_t)a.raw << 12;
    if (b.raw != 0) {
        result32 /= b.raw;
    }
    else {
        result32 |= INT32_MAX;
    }
    fixed20_12_t result;
    result.raw = (int32_t)result32;
    return result;
}

static inline fixed20_12_t scalar_min(const fixed20_12_t a, const fixed20_12_t b) {
    return (a.raw < b.raw) ? a : b;
}

static inline fixed20_12_t scalar_max(const fixed20_12_t a, const fixed20_12_t b) {
    return (a.raw > b.raw) ? a : b;
}

static inline fixed20_12_t scalar_sqrt(fixed20_12_t a) {
#ifdef _PSX
    return scalar_from_int32(SquareRoot12(a.raw));
#else
    return scalar_from_float(sqrtf((float)a.raw / 4096.0f));
#endif
}

static inline vec3_t vec3_from_scalar(const scalar_t a) {
    const vec3_t result = { a, a, a };
    return result;
}

static inline vec3_t vec3_from_scalars(const scalar_t x, const scalar_t y, const scalar_t z) {
    const vec3_t result = { x, y, z };
    return result;
}

static inline vec3_t vec3_from_int32s(int32_t x, int32_t y, int32_t z) {
    vec3_t result;
    result.x.raw = x;
    result.y.raw = y;
    result.z.raw = z;
    return result;
}

static inline vec3_t vec3_add(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_add(a.x, b.x);
    result.y = scalar_add(a.y, b.y);
    result.z = scalar_add(a.z, b.z);
    return result;
}

static inline vec3_t vec3_sub(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(a.x, b.x);
    result.y = scalar_sub(a.y, b.y);
    result.z = scalar_sub(a.z, b.z);
    return result;
}

static inline vec3_t vec3_mul(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_mul(a.x, b.x);
    result.y = scalar_mul(a.y, b.y);
    result.z = scalar_mul(a.z, b.z);
    return result;
}

static inline vec3_t vec3_div(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_div(a.x, b.x);
    result.y = scalar_div(a.y, b.y);
    result.z = scalar_div(a.z, b.z);
    return result;
}

static inline scalar_t vec3_dot(const vec3_t a, const vec3_t b) {
    scalar_t result;
    result.raw = 0;
    result = scalar_add(result, scalar_mul(a.x, b.x));
    result = scalar_add(result, scalar_mul(a.y, b.y));
    result = scalar_add(result, scalar_mul(a.z, b.z));
    return result;
}
static inline vec3_t vec3_scale(const vec3_t a, const scalar_t b) {
    vec3_t result;
    result.x = scalar_mul(a.x, b);
    result.y = scalar_mul(a.y, b);
    result.z = scalar_mul(a.z, b);
    return result;
}

static inline vec3_t vec3_min(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x.raw < b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw < b.y.raw) ? a.y : b.y;
    result.z = (a.z.raw < b.z.raw) ? a.z : b.z;
    return result;
}

static inline vec3_t vec3_max(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x.raw > b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw > b.y.raw) ? a.y : b.y;
    result.z = (a.z.raw > b.z.raw) ? a.z : b.z;
    return result;
}

static inline scalar_t vec3_magnitude_squared(const vec3_t a) {
    scalar_t length_squared = scalar_mul(a.x, a.x);
    length_squared = scalar_add(length_squared, scalar_mul(a.y, a.y));
    length_squared = scalar_add(length_squared, scalar_mul(a.z, a.z));
    return length_squared;
}

static inline vec3_t vec3_normalize(const vec3_t a) {
    const scalar_t magnitude_squared = vec3_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude.raw == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    const vec3_t a_normalized = vec3_div(a, vec3_from_scalar(magnitude));
    return a_normalized;
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(scalar_mul(a.y, b.z), scalar_mul(a.z, b.y));
    result.y = scalar_sub(scalar_mul(a.z, b.x), scalar_mul(a.x, b.z));
    result.z = scalar_sub(scalar_mul(a.x, b.y), scalar_mul(a.y, b.x));
    return result;
}

static inline vec3_t vec3_shift_right(vec3_t a, int amount) {
    vec3_t result = a;
    result.x.raw >>= amount;
    result.y.raw >>= amount;
    result.z.raw >>= amount;
    return result;
}

static inline vec3_t vec3_neg(vec3_t a) {
    vec3_t result = a;
    result.x.raw = -result.x.raw;
    result.y.raw = -result.y.raw;
    result.z.raw = -result.z.raw;
    return result;
}
#endif // FIXED_POINT_H
