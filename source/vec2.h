#ifndef VEC2_H
#define VEC2_H
#include "fixed_point.h"

typedef struct {
    fixed20_12_t x, y;
} vec2_t;

static vec2_t vec2_from_scalar(const scalar_t a) {
    const vec2_t result = { a, a };
    return result;
}

static vec2_t vec2_from_scalars(const scalar_t x, const scalar_t y) {
    const vec2_t result = { x, y };
    return result;
}

static vec2_t vec2_from_int32s(int32_t x, int32_t y, int32_t z) {
    vec2_t result;
    result.x.raw = x;
    result.y.raw = y;
    return result;
}

static vec2_t vec2_add(const vec2_t a, const vec2_t b) {
    vec2_t result;
    result.x = scalar_add(a.x, b.x);
    result.y = scalar_add(a.y, b.y);
    return result;
}

static vec2_t vec2_sub(const vec2_t a, const vec2_t b) {
    vec2_t result;
    result.x = scalar_sub(a.x, b.x);
    result.y = scalar_sub(a.y, b.y);
    return result;
}

static vec2_t vec2_mul(const vec2_t a, const vec2_t b) {
    vec2_t result;
    result.x = scalar_mul(a.x, b.x);
    result.y = scalar_mul(a.y, b.y);
    return result;
}

static vec2_t vec2_div(const vec2_t a, const vec2_t b) {
    vec2_t result;
    result.x = scalar_div(a.x, b.x);
    result.y = scalar_div(a.y, b.y);
    return result;
}

static scalar_t vec2_dot(const vec2_t a, const vec2_t b) {
    scalar_t result;
    result.raw = 0;
    result = scalar_add(result, scalar_mul(a.x, b.x));
    result = scalar_add(result, scalar_mul(a.y, b.y));
    return result;
}
static vec2_t vec2_scale(const vec2_t a, const scalar_t b) {
    vec2_t result;
    result.x = scalar_mul(a.x, b);
    result.y = scalar_mul(a.y, b);
    return result;
}

static vec2_t vec2_min(const vec2_t a, const vec2_t b) {
    vec2_t result;
    result.x = (a.x.raw < b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw < b.y.raw) ? a.y : b.y;
    return result;
}

static vec2_t vec2_max(const vec2_t a, const vec2_t b) {
    vec2_t result;
    result.x = (a.x.raw > b.x.raw) ? a.x : b.x;
    result.y = (a.y.raw > b.y.raw) ? a.y : b.y;
    return result;
}

static vec2_t vec2_perpendicular(const vec2_t a) {
    vec2_t result;
    result.x = scalar_neg(a.y);
    result.y = a.x;
    return result;
}

static scalar_t vec2_cross(const vec2_t a, const vec2_t b) {
    return scalar_sub(scalar_mul(a.x, b.y), scalar_mul(a.y, b.x));
}

static scalar_t vec2_magnitude_squared(const vec2_t a) {
    return scalar_add(scalar_mul(a.x, a.x), scalar_mul(a.y, a.y));
}

static vec2_t vec2_normalize(const vec2_t a) {
    const scalar_t magnitude_squared = vec2_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude.raw == 0) {
        return vec2_from_int32s(0, 0, 0);
    }
    const vec2_t a_normalized = vec2_div(a, vec2_from_scalar(magnitude));
    return a_normalized;
}

static vec2_t vec2_shift_right(vec2_t a, int amount) {
    vec2_t result = a;
    result.x.raw >>= amount;
    result.y.raw >>= amount;
    return result;
}

static vec2_t vec2_neg(vec2_t a) {
    vec2_t result = a;
    result.x.raw = -result.x.raw;
    result.y.raw = -result.y.raw;
    return result;
}

static void vec2_debug(const vec2_t a) {
    printf("%0.3f, %0.3f\n", ((float)a.x.raw) / 4096.0f, ((float)a.y.raw) / 4096.0f);
}
#endif // VEC2_H
