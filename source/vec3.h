#ifndef VEC3_H
#define VEC3_H

#include "fixed_point.h"

typedef struct {
    fixed20_12_t x, y, z;
} vec3_t;

typedef struct {
    int16_t x, y, z; // 3D position
} svec3_t;

// Let's hope and cry that this will be compile-time evaluated
ALWAYS_INLINE vec3_t vec3_from_floats(const float x, const float y, const float z) {
    return (vec3_t) {
        scalar_from_float(x),
        scalar_from_float(y),
        scalar_from_float(z),
    };
}

// Debug
ALWAYS_INLINE void vec3_debug(const vec3_t a) {
    print_fixed_point(a.x);
    printf(", ");
    print_fixed_point(a.y);
    printf(", ");
    print_fixed_point(a.z);
    printf("\n");
}

ALWAYS_INLINE vec3_t vec3_from_scalar(const scalar_t a) {
    return (vec3_t){ a, a, a };
}

ALWAYS_INLINE vec3_t vec3_from_scalars(const scalar_t x, const scalar_t y, const scalar_t z) {
    return (vec3_t){ x, y, z };
}

ALWAYS_INLINE vec3_t vec3_from_int32s(int32_t x, int32_t y, int32_t z) {
    return (vec3_t) {
        x,
        y,
        z,
    };
}

ALWAYS_INLINE vec3_t vec3_from_svec3(svec3_t vec) {
    return (vec3_t) {
        (scalar_t)vec.x * ONE,
        (scalar_t)vec.y * ONE,
        (scalar_t)vec.z * ONE,
    };
}

ALWAYS_INLINE svec3_t svec3_from_vec3(vec3_t vec) {
    return (svec3_t) {
        (int16_t)(vec.x / ONE),
        (int16_t)(vec.y / ONE),
        (int16_t)(vec.z / ONE),
    };
}

ALWAYS_INLINE vec3_t vec3_add(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    };
}

ALWAYS_INLINE vec3_t vec3_sub(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    };
}

ALWAYS_INLINE static vec3_t vec3_mul(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        scalar_mul(a.x, b.x),
        scalar_mul(a.y, b.y),
        scalar_mul(a.z, b.z),
    };
}

ALWAYS_INLINE static vec3_t vec3_muls(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
        scalar_mul(a.z, b),
    };
}

ALWAYS_INLINE vec3_t vec3_div(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        scalar_div(a.x, b.x),
        scalar_div(a.y, b.y),
        scalar_div(a.z, b.z),
    };
}

ALWAYS_INLINE vec3_t vec3_divs(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        scalar_div(a.x, b),
        scalar_div(a.y, b),
        scalar_div(a.z, b),
    };
}

ALWAYS_INLINE static scalar_t vec3_dot(const vec3_t a, const vec3_t b) {
    return scalar_mul(a.x, b.x)
    +      scalar_mul(a.y, b.y)
    +      scalar_mul(a.z, b.z);
}
ALWAYS_INLINE static vec3_t vec3_scale(const vec3_t a, const scalar_t b) {
    return (vec3_t) {
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
        scalar_mul(a.z, b),
    };
}

ALWAYS_INLINE vec3_t vec3_min(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        (a.x < b.x) ? a.x : b.x,
        (a.y < b.y) ? a.y : b.y,
        (a.z < b.z) ? a.z : b.z,
    };
}

ALWAYS_INLINE vec3_t vec3_max(const vec3_t a, const vec3_t b) {
    return (vec3_t) {
        (a.x > b.x) ? a.x : b.x,
        (a.y > b.y) ? a.y : b.y,
        (a.z > b.z) ? a.z : b.z,
    };
}

ALWAYS_INLINE static scalar_t vec3_magnitude_squared(const vec3_t a) {
    const scalar_t x2 = scalar_mul(a.x, a.x);
    const scalar_t y2 = scalar_mul(a.y, a.y);
    const scalar_t z2 = scalar_mul(a.z, a.z);

    if (is_infinity(x2) || is_infinity(y2) || is_infinity(z2)) {
        return INT32_MAX;
    }

    return x2 + y2 + z2;
}

ALWAYS_INLINE static vec3_t vec3_normalize(const vec3_t a) {
    const scalar_t magnitude_squared = vec3_magnitude_squared(a);
    const scalar_t magnitude = scalar_sqrt(magnitude_squared);
    if (magnitude == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    const vec3_t a_normalized = vec3_divs(a, magnitude);
    return a_normalized;
}

ALWAYS_INLINE static vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t) {
        scalar_mul(a.y, b.z) - scalar_mul(a.z, b.y),
        scalar_mul(a.z, b.x) - scalar_mul(a.x, b.z),
        scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x),
    };
}

ALWAYS_INLINE vec3_t vec3_shift_right(vec3_t a, int amount) {
    return (vec3_t) {
        a.x >> amount,
        a.y >> amount,
        a.z >> amount,
    };
}

ALWAYS_INLINE vec3_t vec3_shift_left(vec3_t a, int amount) {
    return (vec3_t) {
        a.x << amount,
        a.y << amount,
        a.z << amount,
    };
}

ALWAYS_INLINE vec3_t vec3_neg(vec3_t a) {
    return (vec3_t) {
        -a.x,
        -a.y,
        -a.z,
    };
}
#endif // VEC3_H
