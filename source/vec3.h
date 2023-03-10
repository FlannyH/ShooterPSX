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
    printf("%0.3f, %0.3f, %0.3f\n", ((float)a.x.raw) / 4096.0f, ((float)a.y.raw) / 4096.0f, ((float)a.z.raw) / 4096.0f);
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
    result.x.raw = x;
    result.y.raw = y;
    result.z.raw = z;
    return result;
}

static vec3_t vec3_add(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_add(a.x, b.x);
    result.y = scalar_add(a.y, b.y);
    result.z = scalar_add(a.z, b.z);
    return result;
}

static vec3_t vec3_sub(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(a.x, b.x);
    result.y = scalar_sub(a.y, b.y);
    result.z = scalar_sub(a.z, b.z);
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
    scalar_t result;
    result.raw = 0;
    result = scalar_add(result, scalar_mul(a.x, b.x));
    result = scalar_add(result, scalar_mul(a.y, b.y));
    result = scalar_add(result, scalar_mul(a.z, b.z));
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
    result.x = (a.x.raw < b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw < b.y.raw) ? a.y : b.y;
    result.z = (a.z.raw < b.z.raw) ? a.z : b.z;
    return result;
}

static vec3_t vec3_max(const vec3_t a, const vec3_t b) {
    vec3_t result;
    result.x = (a.x.raw > b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw > b.y.raw) ? a.y : b.y;
    result.z = (a.z.raw > b.z.raw) ? a.z : b.z;
    return result;
}

static scalar_t vec3_magnitude_squared(const vec3_t a) {
    scalar_t length_squared = scalar_mul(a.x, a.x);
    length_squared = scalar_add(length_squared, scalar_mul(a.y, a.y));
    length_squared = scalar_add(length_squared, scalar_mul(a.z, a.z));
    return length_squared;
}

static vec3_t vec3_normalize(const vec3_t a) {
    const scalar_t magnitude_squared = vec3_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude.raw == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    const vec3_t a_normalized = vec3_div(a, vec3_from_scalar(magnitude));
    return a_normalized;
}

static vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(scalar_mul(a.y, b.z), scalar_mul(a.z, b.y));
    result.y = scalar_sub(scalar_mul(a.z, b.x), scalar_mul(a.x, b.z));
    result.z = scalar_sub(scalar_mul(a.x, b.y), scalar_mul(a.y, b.x));
    return result;
}

static vec3_t vec3_shift_right(vec3_t a, int amount) {
    vec3_t result = a;
    result.x.raw >>= amount;
    result.y.raw >>= amount;
    result.z.raw >>= amount;
    return result;
}

static vec3_t vec3_shift_left(vec3_t a, int amount) {
    vec3_t result = a;
    result.x.raw <<= amount;
    result.y.raw <<= amount;
    result.z.raw <<= amount;
    return result;
}

static vec3_t vec3_neg(vec3_t a) {
    vec3_t result = a;
    result.x.raw = -result.x.raw;
    result.y.raw = -result.y.raw;
    result.z.raw = -result.z.raw;
    return result;
}
#endif // VEC3_H
