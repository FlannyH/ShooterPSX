#include "fixed_point.h"

#ifdef _PSX
#include <psxgte.h>
#else
#include <corecrt_math.h>
#endif
#include <stdlib.h>


fixed24_8_t scalar_from_int32(int32_t raw) {
    fixed24_8_t result;
    result.raw = raw;
    return result;
}

fixed24_8_t scalar_add(const fixed24_8_t a, const fixed24_8_t b) {
    fixed24_8_t result;
    result.raw = a.raw + b.raw;
    return result;
}

fixed24_8_t scalar_sub(const fixed24_8_t a, const fixed24_8_t b) {
    fixed24_8_t result;
    result.raw = a.raw - b.raw;
    return result;
}

fixed24_8_t scalar_mul(const fixed24_8_t a, const fixed24_8_t b) {
    // todo: optimize this, maybe use compiler intrinsics?
    int64_t result32 = ((int64_t)a.raw * (int64_t)b.raw) >> 8;
    // overflow check
    if (result32 > INT32_MAX) {
        result32 = INT32_MAX;
    }
    else if (result32 < -INT32_MAX) {
        result32 = -INT32_MAX;
    }
    fixed24_8_t result;
    result.raw = (int32_t)(result32);
    return result;
}

fixed24_8_t scalar_div(const fixed24_8_t a, const fixed24_8_t b) {
    int64_t result32 = (int64_t)a.raw << 8;
    if (b.raw != 0) {
        result32 /= b.raw;
    } else {
        result32 |= INT32_MAX;
    }
    fixed24_8_t result;
    result.raw = (int32_t)result32;
    return result;
}

fixed24_8_t scalar_min(const fixed24_8_t a, const fixed24_8_t b) {
    return (a.raw < b.raw) ? a : b;
}

fixed24_8_t scalar_max(const fixed24_8_t a, const fixed24_8_t b) {
    return (a.raw > b.raw) ? a : b;
}

fixed24_8_t scalar_sqrt(fixed24_8_t a) {
#ifdef _PSX
    return scalar_from_int32(SquareRoot12(a.raw << 4) >> 4);
#else
    return scalar_from_float(sqrtf((float)a.raw / 256.0f));
#endif
}

vec3_t vec3_from_scalar(const scalar_t a) {
    const vec3_t result = { a, a, a };
    return result;
}

vec3_t vec3_from_scalars(const scalar_t x, const scalar_t y, const scalar_t z) {
    const vec3_t result = { x, y, z };
    return result;
}

vec3_t vec3_from_int32s(int32_t x, int32_t y, int32_t z) {
    vec3_t result;
    result.x.raw = x;
    result.y.raw = y;
    result.z.raw = z;
    return result;
}

vec3_t vec3_add(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_add(a.x, b.x);
    result.y = scalar_add(a.y, b.y);
    result.z = scalar_add(a.z, b.z);
    return result;
}

vec3_t vec3_sub(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(a.x, b.x);
    result.y = scalar_sub(a.y, b.y);
    result.z = scalar_sub(a.z, b.z);
    return result;
}

vec3_t vec3_mul(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_mul(a.x, b.x);
    result.y = scalar_mul(a.y, b.y);
    result.z = scalar_mul(a.z, b.z);
    return result;
}

vec3_t vec3_div(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_div(a.x, b.x);
    result.y = scalar_div(a.y, b.y);
    result.z = scalar_div(a.z, b.z);
    return result;
}

scalar_t vec3_dot(const vec3_t a, const vec3_t b) {
    scalar_t result;
    result.raw = 0;
    result = scalar_add(result, scalar_mul(a.x, b.x));
    result = scalar_add(result, scalar_mul(a.y, b.y));
    result = scalar_add(result, scalar_mul(a.z, b.z));
    return result;
}

vec3_t vec3_cross(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(scalar_mul(a.y, b.z), scalar_mul(a.z, b.y));
    result.y = scalar_sub(scalar_mul(a.z, b.x), scalar_mul(a.x, b.z));
    result.z = scalar_sub(scalar_mul(a.x, b.y), scalar_mul(a.y, b.x));
    return result;
}

vec3_t vec3_min(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x.raw < b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw < b.y.raw) ? a.y : b.y;
    result.z = (a.z.raw < b.z.raw) ? a.z : b.z;
    return result;
}

vec3_t vec3_max(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x.raw > b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw > b.y.raw) ? a.y : b.y;
    result.z = (a.z.raw > b.z.raw) ? a.z : b.z;
    return result;
}

vec3_t vec3_normalize(vec3_t a) {
    const scalar_t magnitude_squared = vec3_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude.raw == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    const vec3_t a_normalized = vec3_div(a, vec3_from_scalar(magnitude));
    return a_normalized;
}

scalar_t vec3_magnitude_squared(const vec3_t a) {
    scalar_t length_squared = scalar_mul(a.x, a.x);
    length_squared = scalar_add(length_squared, scalar_mul(a.y, a.y));
    length_squared = scalar_add(length_squared, scalar_mul(a.z, a.z));
    return length_squared;
}
