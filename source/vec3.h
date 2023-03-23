#ifndef VEC3_H
#define VEC3_H

#include "fixed_point.h"

typedef struct {
    fixed20_12_t x, y, z;
} vec3_t;

// Let's hope and cry that this will be compile-time evaluated
static vec3_t vec3_from_floats(const float x, const float y, const float z) {
    vec3_t result;
    result.x = scalar_from_float(x);
    result.y = scalar_from_float(y);
    result.z = scalar_from_float(z);
    return result;
}

// Debug
static void vec3_debug(const vec3_t a) {
    printf("%0.3f, %0.3f, %0.3f\n", ((float)a.x) / 4096.0f, ((float)a.y) / 4096.0f, ((float)a.z) / 4096.0f);
}

static vec3_t vec3_from_scalar(const scalar_t a) {
    const vec3_t result = { a, a, a };
    return result;
}

static vec3_t vec3_from_scalars(const scalar_t x, const scalar_t y, const scalar_t z) {
    const vec3_t result = { x, y, z };
    return result;
}

static vec3_t vec3_from_int32s(int32_t x, int32_t y, int32_t z) {
    vec3_t result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static vec3_t vec3_add(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

static vec3_t vec3_sub(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static vec3_t vec3_mul(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_mul(a.x, b.x);
    result.y = scalar_mul(a.y, b.y);
    result.z = scalar_mul(a.z, b.z);
    return result;
}

static vec3_t vec3_div(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_div(a.x, b.x);
    result.y = scalar_div(a.y, b.y);
    result.z = scalar_div(a.z, b.z);
    return result;
}

static scalar_t vec3_dot(const vec3_t a, const vec3_t b) {
    scalar_t result = 0;
    result += scalar_mul(a.x, b.x);
    result += scalar_mul(a.y, b.y);
    result += scalar_mul(a.z, b.z);
    return result;
}
static vec3_t vec3_scale(const vec3_t a, const scalar_t b) {
    vec3_t result;
    result.x = scalar_mul(a.x, b);
    result.y = scalar_mul(a.y, b);
    result.z = scalar_mul(a.z, b);
    return result;
}

static vec3_t vec3_min(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x < b.x) ? a.x : b.x;
    result.y = (a.y < b.y) ? a.y : b.y;
    result.z = (a.z < b.z) ? a.z : b.z;
    return result;
}

static vec3_t vec3_max(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x > b.x) ? a.x : b.x;
    result.y = (a.y > b.y) ? a.y : b.y;
    result.z = (a.z > b.z) ? a.z : b.z;
    return result;
}

static scalar_t vec3_magnitude_squared(const vec3_t a) {
    int overflow = 0;
    scalar_t length_squared = scalar_mul(a.x, a.x);
    overflow |= operator_flags.overflow;
    length_squared = length_squared + scalar_mul(a.y, a.y);
    overflow |= operator_flags.overflow;
    length_squared = length_squared + scalar_mul(a.z, a.z);
    overflow |= operator_flags.overflow;
    if (overflow) {
        length_squared = INT32_MAX;
    }
    if (length_squared < 0) {
        length_squared = INT32_MAX;
    }
    //WARN_IF("magnitude overflowed and became int32_max", length_squared == INT32_MAX);
    return length_squared;
}

static vec3_t vec3_normalize(const vec3_t a) {
    const scalar_t magnitude_squared = vec3_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    const vec3_t a_normalized = vec3_div(a, vec3_from_scalar(magnitude));
    return a_normalized;
}

static vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t result;
    result.x = scalar_mul(a.y, b.z) - scalar_mul(a.z, b.y);
    result.y = scalar_mul(a.z, b.x) - scalar_mul(a.x, b.z);
    result.z = scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x);
    return result;
}

static vec3_t vec3_shift_right(vec3_t a, int amount) {
    vec3_t result = a;
    result.x >>= amount;
    result.y >>= amount;
    result.z >>= amount;
    return result;
}

static vec3_t vec3_shift_left(vec3_t a, int amount) {
    vec3_t result = a;
    result.x <<= amount;
    result.y <<= amount;
    result.z <<= amount;
    return result;
}

static vec3_t vec3_neg(vec3_t a) {
    vec3_t result = a;
    result.x = -result.x;
    result.y = -result.y;
    result.z = -result.z;
    return result;
}
#endif // VEC3_H
