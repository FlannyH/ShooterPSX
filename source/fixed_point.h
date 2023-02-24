#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    union {
        struct {
            uint8_t fraction;
            int8_t integer_low;
            int16_t integer_high;
        };
        int32_t raw;
    };
} fixed24_8_t;

typedef fixed24_8_t scalar_t;

typedef struct {
    fixed24_8_t x, y, z;
} vec3_t;

// Let's hope and pray that this will be compile-time evaluated
inline fixed24_8_t scalar_from_float(const float a) {
    fixed24_8_t result;
    result.raw = (int32_t)(a * 256.0f);
    return result;
}

// Let's hope and cry that this will be compile-time evaluated
inline vec3_t vec3_from_floats(const float x, const float y, const float z) {
    vec3_t result;
    result.x = scalar_from_float(x);
    result.y = scalar_from_float(y);
    result.z = scalar_from_float(z);
    return result;
}

// Otherwise just use these
fixed24_8_t scalar_from_int32(int32_t raw);
fixed24_8_t scalar_add(fixed24_8_t a, fixed24_8_t b);
fixed24_8_t scalar_sub(fixed24_8_t a, fixed24_8_t b);
fixed24_8_t scalar_mul(fixed24_8_t a, fixed24_8_t b);
fixed24_8_t scalar_div(fixed24_8_t a, fixed24_8_t b);
fixed24_8_t scalar_min(fixed24_8_t a, fixed24_8_t b);
fixed24_8_t scalar_max(fixed24_8_t a, fixed24_8_t b);
fixed24_8_t scalar_sqrt(fixed24_8_t a);
vec3_t vec3_from_scalar(scalar_t a);
vec3_t vec3_from_scalars(scalar_t x, scalar_t y, scalar_t z);
vec3_t vec3_from_int32s(int32_t x, int32_t y, int32_t z);
vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_sub(vec3_t a, vec3_t b);
vec3_t vec3_mul(vec3_t a, vec3_t b);
vec3_t vec3_div(vec3_t a, vec3_t b);
vec3_t vec3_cross(vec3_t a, vec3_t b);
vec3_t vec3_min(vec3_t a, vec3_t b);
vec3_t vec3_max(vec3_t a, vec3_t b);
vec3_t vec3_normalize(vec3_t a);
scalar_t vec3_dot(vec3_t a, vec3_t b);
scalar_t vec3_magnitude_squared(vec3_t a);
scalar_t vec3_angle(vec3_t a, vec3_t b);

// Debug
inline void vec3_debug(vec3_t a) {
    printf("%0.3f, %0.3f, %0.3f\n", ((float)a.x.raw) / 256.0f, ((float)a.y.raw) / 256.0f, ((float)a.z.raw) / 256.0f);
}

inline void scalar_debug(scalar_t a) {
    printf("%0.3f\n", ((float)a.raw) / 256.0f);
}

#endif // FIXED_POINT_H
