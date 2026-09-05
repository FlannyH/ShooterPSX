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

static inline vec2_t vec2_from_floats(const float x, const float y) {
    return (vec2_t) {
        scalar_from_float(x),
        scalar_from_float(y),
    };
}

static inline void vec2_debug(const vec2_t a) {
    print_fixed_point(a.x);
    printf(", ");
    print_fixed_point(a.y);
    printf("\n");
}

static inline vec2_t vec2_from_scalar(const scalar_t a) {
    return (vec2_t){ a, a };
}

static inline vec2_t vec2_from_scalars(const scalar_t x, const scalar_t y) {
    return (vec2_t){ x, y };
}

// todo: this one may not work in float mode yet
static inline vec2_t vec2_from_int32s(int32_t x, int32_t y) {
    return (vec2_t){ (scalar_t)x, (scalar_t)y };
}

static inline vec2_t vec2_from_svec2(svec2_t vec) {
    return (vec2_t) {
        (scalar_t)vec.x * ONE,
        (scalar_t)vec.y * ONE,
    };
}

static inline svec2_t svec2_from_vec2(vec2_t vec) {
    return (svec2_t) {
        (int16_t)(vec.x / ONE),
        (int16_t)(vec.y / ONE),
    };
}

static inline vec2_t vec2_add(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x + b.x),
        (a.y + b.y),
    };
}

static inline vec2_t vec2_adds(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        (a.x + b),
        (a.y + b),
    };
}

static inline vec2_t vec2_sub(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x - b.x),
        (a.y - b.y),
    };
}

static inline vec2_t vec2_subs(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        (a.x - b),
        (a.y - b),
    };
}

static inline vec2_t vec2_mul(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b.x),
        scalar_mul(a.y, b.y),
    };
}

static inline vec2_t vec2_muls(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
    };
}

static inline vec2_t vec2_div(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_div(a.x, b.x),
        scalar_div(a.y, b.y),
    };
}

static inline vec2_t vec2_divs(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        scalar_div(a.x, b),
        scalar_div(a.y, b),
    };
}

static inline scalar_t vec2_dot(const vec2_t a, const vec2_t b) {
    return scalar_mul(a.x, b.x)
    +      scalar_mul(a.y, b.y);
}

static inline vec2_t vec2_min(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_min(a.x, b.x),
        scalar_min(a.y, b.y),
    };
}

static inline vec2_t vec2_max(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_max(a.x, b.x),
        scalar_max(a.y, b.y),
    };
}

static inline vec2_t vec2_shift_left(const vec2_t a, const int amount) {
    return (vec2_t) {
        scalar_shift_left(a.x, amount),
        scalar_shift_left(a.y, amount),
    };
}

static inline vec2_t vec2_shift_right(const vec2_t a, const int amount) {
    return (vec2_t) {
        scalar_shift_right(a.x, amount),
        scalar_shift_right(a.y, amount),
    };
}

static inline scalar_t vec2_magnitude_squared(const vec2_t a) {
    const scalar_t x2 = scalar_mul(a.x, a.x);
    const scalar_t y2 = scalar_mul(a.y, a.y);

    if (is_infinity(x2) || is_infinity(y2)) {
        return INT32_MAX;
    }

    return x2 + y2;
}

static inline scalar_t vec2_magnitude(vec2_t a) {
    // avoid overflows
    while ((scalar_abs(a.x) > (2*ONE)) || (scalar_abs(a.y) > (2*ONE))) {
        a = vec2_shift_right(a, 2);
    }

    scalar_t magnitude = vec2_magnitude_squared(a);
    if (magnitude == 0) return 0;
    return scalar_sqrt(magnitude);
}

static inline scalar_t vec2_cross(const vec2_t a, const vec2_t b) {
    return (scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x));
}

static inline vec2_t vec2_normalize(vec2_t a) {
    // avoid overflows
    while ((scalar_abs(a.x) > (2*ONE)) || (scalar_abs(a.y) > (2*ONE))) {
        a = vec2_shift_right(a, 2);
    }

    scalar_t magnitude = vec2_magnitude_squared(a);
    if (magnitude == 0) {
        return vec2_from_int32s(0, 0);
    }
    magnitude = scalar_sqrt(magnitude);

    const vec2_t a_normalized = vec2_divs(a, magnitude);
    return a_normalized;
}

static inline vec2_t vec2_neg(vec2_t a) {
    return (vec2_t) {
        -a.x,
        -a.y,
    };
}

// per-component scalar_clamp
static inline vec2_t vec2_clamp(vec2_t a, vec2_t min, vec2_t max) {
    return (vec2_t) {
        scalar_clamp(a.x, min.x, max.x),
        scalar_clamp(a.y, min.y, max.y),
    };
}

static inline vec2_t vec2_lerp(vec2_t a, vec2_t b, scalar_t t) {
    return (vec2_t) {
        a.x + scalar_mul(b.x - a.x, t),
        a.y + scalar_mul(b.y - a.y, t),
    };
}

static inline int vec2_equal(vec2_t a, vec2_t b) {
    return (a.x == b.x) && (a.y == b.y);
}

#pragma GCC diagnostic pop

#endif
