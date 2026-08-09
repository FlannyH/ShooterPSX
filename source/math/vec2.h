#ifndef VEC2_H
#define VEC2_H

#include "scalar.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// todo(vec2_gte_opt): desc: psx gte-accelerated svec2_t math
// todo(vec2_inline): desc: is inlining the functions actually faster?

typedef struct {
    scalar_t x, y;
} vec2_t;

typedef struct {
    int16_t x, y; // 2D position
} svec2_t;

ALWAYS_INLINE vec2_t vec2_from_floats(const float x, const float y) {
    return (vec2_t) {
        scalar_from_float(x),
        scalar_from_float(y),
    };
}

static void vec2_debug(const vec2_t a) {
    print_fixed_point(a.x);
    printf(", ");
    print_fixed_point(a.y);
    printf("\n");
}

static vec2_t vec2_from_scalar(const scalar_t a) {
    return (vec2_t){ a, a };
}

static vec2_t vec2_from_scalars(const scalar_t x, const scalar_t y) {
    return (vec2_t){ x, y };
}

// todo: this one may not work in float mode yet
static vec2_t vec2_from_int32s(int32_t x, int32_t y) {
    return (vec2_t){ (scalar_t)x, (scalar_t)y };
}

ALWAYS_INLINE vec2_t vec2_from_svec2(svec2_t vec) {
    return (vec2_t) {
        (scalar_t)vec.x * ONE,
        (scalar_t)vec.y * ONE,
    };
}

ALWAYS_INLINE svec2_t svec2_from_vec2(vec2_t vec) {
    return (svec2_t) {
        (int16_t)(vec.x / ONE),
        (int16_t)(vec.y / ONE),
    };
}

static vec2_t vec2_add(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x + b.x),
        (a.y + b.y),
    };
}

static vec2_t vec2_adds(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        (a.x + b),
        (a.y + b),
    };
}

static vec2_t vec2_sub(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x - b.x),
        (a.y - b.y),
    };
}

static vec2_t vec2_subs(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        (a.x - b),
        (a.y - b),
    };
}

static vec2_t vec2_mul(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b.x),
        scalar_mul(a.y, b.y),
    };
}

static vec2_t vec2_muls(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
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

static scalar_t vec2_magnitude_squared(const vec2_t a) {
    return (scalar_mul(a.x, a.x) + scalar_mul(a.y, a.y));
}

static scalar_t vec2_magnitude(const vec2_t a) {
    return scalar_sqrt(vec2_magnitude_squared(a));
}

static vec2_t vec2_normalize(const vec2_t a) {
    const scalar_t magnitude = vec2_magnitude(a);
    if (magnitude == 0) {
        return vec2_from_int32s(0, 0);
    }
    const vec2_t a_normalized = vec2_div(a, vec2_from_scalar(magnitude));
    return a_normalized;
}

static scalar_t vec2_cross(const vec2_t a, const vec2_t b) {
    return (scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x));
}

static vec2_t vec2_shift_right(const vec2_t a, const int amount) {
    return (vec2_t) {
        scalar_shift_right(a.x, amount),
        scalar_shift_right(a.y, amount),
    };
}

static vec2_t vec2_shift_left(const vec2_t a, const int amount) {
    return (vec2_t) {
        scalar_shift_left(a.x, amount),
        scalar_shift_left(a.y, amount),
    };
}

static vec2_t vec2_neg(const vec2_t a) {
    return (vec2_t) {
        -a.x,
        -a.y,
    };
}

#pragma GCC diagnostic pop

#endif // VEC2_H
