#ifndef VEC3_H
#define VEC3_H

#include "scalar.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// todo(vec3_gte_opt): desc: psx gte-accelerated svec3_t math
// todo(vec3_inline): desc: is inlining the functions actually faster?

typedef struct {
    scalar_t x, y, z;
} vec3_t;

typedef struct {
    int16_t x, y, z; // 3D position
} svec3_t;

static inline vec3_t vec3_from_floats(const float x, const float y, const float z) {
    return (vec3_t) {
        scalar_from_float(x),
        scalar_from_float(y),
        scalar_from_float(z),
    };
}

static inline void vec3_debug(const vec3_t a) {
    print_fixed_point(a.x);
    printf(", ");
    print_fixed_point(a.y);
    printf(", ");
    print_fixed_point(a.z);
    printf("\n");
}

static inline vec3_t vec3_from_scalar(const scalar_t a) {
    return (vec3_t){ a, a, a };
}

static inline vec3_t vec3_from_scalars(const scalar_t x, const scalar_t y, const scalar_t z) {
    return (vec3_t){ x, y, z };
}

// todo: this one may not work in float mode yet
static inline vec3_t vec3_from_int32s(int32_t x, int32_t y, int32_t z) {
    return (vec3_t) {
        (scalar_t)x,
        (scalar_t)y,
        (scalar_t)z,
    };
}

static inline vec3_t vec3_from_svec3(svec3_t vec) {
    return (vec3_t) {
        (scalar_t)vec.x * ONE,
        (scalar_t)vec.y * ONE,
        (scalar_t)vec.z * ONE,
    };
}

static inline svec3_t svec3_from_vec3(vec3_t vec) {
    return (svec3_t) {
        (int16_t)(vec.x / ONE),
        (int16_t)(vec.y / ONE),
        (int16_t)(vec.z / ONE),
    };
}

static inline vec3_t vec3_add(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    };
}

static inline vec3_t vec3_adds(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        a.x + b,
        a.y + b,
        a.z + b,
    };
}

static inline vec3_t vec3_sub(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    };
}

static inline vec3_t vec3_subs(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        a.x - b,
        a.y - b,
        a.z - b,
    };
}

static inline vec3_t vec3_mul(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        scalar_mul(a.x, b.x),
        scalar_mul(a.y, b.y),
        scalar_mul(a.z, b.z),
    };
}

static inline vec3_t vec3_muls(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
        scalar_mul(a.z, b),
    };
}

static inline vec3_t vec3_div(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        scalar_div(a.x, b.x),
        scalar_div(a.y, b.y),
        scalar_div(a.z, b.z),
    };
}

static inline vec3_t vec3_divs(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        scalar_div(a.x, b),
        scalar_div(a.y, b),
        scalar_div(a.z, b),
    };
}

static inline scalar_t vec3_dot(const vec3_t a, const vec3_t b) {
    return scalar_mul(a.x, b.x)
    +      scalar_mul(a.y, b.y)
    +      scalar_mul(a.z, b.z);
}

static inline vec3_t vec3_min(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        scalar_min(a.x, b.x),
        scalar_min(a.y, b.y),
        scalar_min(a.z, b.z),
    };
}

static inline vec3_t vec3_max(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        scalar_max(a.x, b.x),
        scalar_max(a.y, b.y),
        scalar_max(a.z, b.z),
    };
}

static inline vec3_t vec3_shift_left(vec3_t a, int amount) {
    return (vec3_t) {
        scalar_shift_left(a.x, amount),
        scalar_shift_left(a.y, amount),
        scalar_shift_left(a.z, amount),
    };
}

static inline vec3_t vec3_shift_right(vec3_t a, int amount) {
    return (vec3_t) {
        scalar_shift_right(a.x, amount),
        scalar_shift_right(a.y, amount),
        scalar_shift_right(a.z, amount),
    };
}

static inline scalar_t vec3_magnitude_squared(const vec3_t a) {
    const scalar_t x2 = scalar_mul(a.x, a.x);
    const scalar_t y2 = scalar_mul(a.y, a.y);
    const scalar_t z2 = scalar_mul(a.z, a.z);

    if (is_infinity(x2) || is_infinity(y2) || is_infinity(z2)) {
        return INT32_MAX;
    }

    return x2 + y2 + z2;
}

static inline scalar_t vec3_magnitude(vec3_t a) {
    // avoid overflows
    while ((scalar_abs(a.x) > (4*ONE)) || (scalar_abs(a.y) > (4*ONE)) || (scalar_abs(a.z) > (4*ONE))) {
        a = vec3_shift_right(a, 2);
    }

    scalar_t magnitude = vec3_magnitude_squared(a);
    if (magnitude == 0) return 0;
    return scalar_sqrt(magnitude);
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t) {
        scalar_mul(a.y, b.z) - scalar_mul(a.z, b.y),
        scalar_mul(a.z, b.x) - scalar_mul(a.x, b.z),
        scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x),
    };
}

static inline vec3_t vec3_normalize(vec3_t a) {
    // avoid overflows
    while ((scalar_abs(a.x) > (2*ONE)) || (scalar_abs(a.y) > (2*ONE)) || (scalar_abs(a.z) > (2*ONE))) {
        a = vec3_shift_right(a, 2);
    }

    scalar_t magnitude = vec3_magnitude_squared(a);
    if (magnitude == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    magnitude = scalar_sqrt(magnitude);

    const vec3_t a_normalized = vec3_divs(a, magnitude);
    return a_normalized;
}

static inline vec3_t vec3_neg(vec3_t a) {
    return (vec3_t) {
        -a.x,
        -a.y,
        -a.z,
    };
}

// per-component scalar_clamp
static inline vec3_t vec3_clamp(vec3_t a, vec3_t min, vec3_t max) {
    return (vec3_t) {
        scalar_clamp(a.x, min.x, max.x),
        scalar_clamp(a.y, min.y, max.y),
        scalar_clamp(a.z, min.z, max.z),
    };
}

static inline vec3_t vec3_lerp(vec3_t a, vec3_t b, scalar_t t) {
    return (vec3_t) {
        a.x + scalar_mul(b.x - a.x, t),
        a.y + scalar_mul(b.y - a.y, t),
        a.z + scalar_mul(b.z - a.z, t),
    };
}

static inline int vec3_equal(vec3_t a, vec3_t b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

#pragma GCC diagnostic pop

#endif // VEC3_H
