#ifndef VEC2_H
#define VEC2_H

#include "fixed_point.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

typedef struct {
    fixed20_12_t x, y;
} vec2_t;

typedef struct {
    int16_t x, y; // 2D position
} svec2_t;

static vec2_t vec2_from_scalar(const scalar_t a) {
    return (vec2_t){ a, a };
}

static vec2_t vec2_from_scalars(const scalar_t x, const scalar_t y) {
    return (vec2_t){ x, y };
}

static vec2_t vec2_from_int32s(int32_t x, int32_t y) {
    return (vec2_t){ x, y };
}

static vec2_t vec2_add(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x + b.x),
        (a.y + b.y),
    };
}

static vec2_t vec2_sub(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x - b.x),
        (a.y - b.y),
    };
}

static vec2_t vec2_mul(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b.x),
        scalar_mul(a.y, b.y),
    };
}

static vec2_t vec2_div(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_div(a.x, b.x),
        scalar_div(a.y, b.y),
    };
}

static vec2_t vec2_divs(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        scalar_div(a.x, b),
        scalar_div(a.y, b),
    };
}

static scalar_t vec2_dot(const vec2_t a, const vec2_t b) {
    return scalar_mul(a.x, b.x) 
    +      scalar_mul(a.y, b.y);
}

static vec2_t vec2_scale(const vec2_t a, const scalar_t b) {
    return (vec2_t) { 
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
    };
}

static vec2_t vec2_min(const vec2_t a, const vec2_t b) {
    return (vec2_t) { 
        (a.x < b.x) ? a.x : b.x,
        (a.y < b.y) ? a.y : b.y,
    };
}

static vec2_t vec2_max(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x > b.x) ? a.x : b.x,
        (a.y > b.y) ? a.y : b.y,
    };
}

static vec2_t vec2_perpendicular(const vec2_t a) {
    return (vec2_t) {
        -(a.y),
        a.x,
    };
}

static scalar_t vec2_cross(const vec2_t a, const vec2_t b) {
    return (scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x));
}

static scalar_t vec2_magnitude_squared(const vec2_t a) {
    return (scalar_mul(a.x, a.x) + scalar_mul(a.y, a.y));
}

static vec2_t vec2_normalize(const vec2_t a) {
    const scalar_t magnitude_squared = vec2_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude == 0) {
        return vec2_from_int32s(0, 0);
    }
    const vec2_t a_normalized = vec2_div(a, vec2_from_scalar(magnitude));
    return a_normalized;
}

static vec2_t vec2_shift_right(const vec2_t a, const int amount) {
    return (vec2_t) {
        a.x >> amount,
        a.y >> amount,
    };
}

static vec2_t vec2_neg(const vec2_t a) {
    return (vec2_t) {
        -a.x,
        -a.y,
    };
}

static scalar_t vec2_magnitude(const vec2_t a) {
    return scalar_sqrt(vec2_magnitude_squared(a));
}

static void vec2_debug(const vec2_t a) {
    print_fixed_point(a.x);
    printf(", ");
    print_fixed_point(a.y);
    printf("\n");
}

#pragma GCC diagnostic pop

#endif // VEC2_H
