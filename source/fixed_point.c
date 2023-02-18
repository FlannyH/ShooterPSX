#include "fixed_point.h"

fixed8_8_t scalar_from_int16(const int16_t raw) {
    fixed8_8_t result;
    result.raw = raw;
    return result;
}

fixed8_8_t scalar_add(const fixed8_8_t a, const fixed8_8_t b) {
    fixed8_8_t result;
    result.raw = a.raw + b.raw;
    return result;
}

fixed8_8_t scalar_sub(const fixed8_8_t a, const fixed8_8_t b) {
    fixed8_8_t result;
    result.raw = a.raw - b.raw;
    return result;
}

fixed8_8_t scalar_mul(const fixed8_8_t a, const fixed8_8_t b) {
    const int32_t result32 = a.raw * b.raw;
    fixed8_8_t result;
    result.raw = (int16_t)(result32 >> 8);
    return result;
}

fixed8_8_t scalar_div(const fixed8_8_t a, const fixed8_8_t b) {
    int32_t result32 = (int32_t)a.raw << 8;
    result32 /= b.raw;
    fixed8_8_t result;
    result.raw = (int16_t)result32;
    return result;
}

vec3_t vec3_from_scalar(const scalar_t a) {
    const vec3_t result = { a, a, a };
    return result;
}

vec3_t vec3_from_scalars(const scalar_t x, const scalar_t y, const scalar_t z) {
    const vec3_t result = { x, y, z };
    return result;
}

vec3_t vec3_from_int16s(const int16_t x, const int16_t y, const int16_t z) {
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

vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t result;
    result.x = scalar_sub(scalar_mul(a.y, b.z), scalar_mul(a.z, b.y));
    result.y = scalar_sub(scalar_mul(a.z, b.x), scalar_mul(a.x, b.z));
    result.z = scalar_sub(scalar_mul(a.x, b.y), scalar_mul(a.y, b.x));
    return result;
}

scalar_t vec3_magnitude_squared(const vec3_t a) {
    scalar_t length_squared = scalar_mul(a.x, a.x);
    length_squared = scalar_add(length_squared, scalar_mul(a.y, a.y));
    length_squared = scalar_add(length_squared, scalar_mul(a.z, a.z));
    return length_squared;
}