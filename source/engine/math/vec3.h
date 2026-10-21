#ifndef VEC3_H
#define VEC3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scalar.h"
#include <stddef.h>
#include <assert.h>

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
        SCALAR(x),
        SCALAR(y),
        SCALAR(z),
    };
}

static inline void vec3_debug(const vec3_t a) {
    print_scalar(a.x);
    printf(", ");
    print_scalar(a.y);
    printf(", ");
    print_scalar(a.z);
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

/// doc: desc: Calculate the dot product between `a` and `b`, and return the result
static inline scalar_t vec3_dot(const vec3_t a, const vec3_t b) {
    #ifdef _FLOAT
    return scalar_mul(a.x, b.x)
    +      scalar_mul(a.y, b.y)
    +      scalar_mul(a.z, b.z);
    #else
    int64_t result = 0;
    result += (int64_t)a.x * (int64_t)b.x;
    result += (int64_t)a.y * (int64_t)b.y;
    result += (int64_t)a.z * (int64_t)b.z;
    return (scalar_t)(result >> FRAC_BITS);
    #endif
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
    int64_t x2_one = a.x * a.x;
    int64_t y2_one = a.y * a.y;
    int64_t z2_one = a.z * a.z;
    int64_t result_one = x2_one + y2_one + z2_one;
    int32_t result = (result_one >> FRAC_BITS);
    return result;
}

static inline scalar_t vec3_magnitude(vec3_t a) {
    if (a.x == 0 && a.y == 0 && a.z == 0) {
        return 0;
    }

    #ifdef _FLOAT
    return sqrtf((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
    #else
    if (is_infinity(a.x) || is_infinity(a.y) || is_infinity(a.z)) {
        return INT32_MAX;
    }
    int64_t sum = 0;
    sum += ((int64_t)a.x * (int64_t)a.x);
    sum += ((int64_t)a.y * (int64_t)a.y);
    sum += ((int64_t)a.z * (int64_t)a.z);

    int zeros_x = (a.x != 0) ? leading_zeros(a.x) : 32;
    int zeros_y = (a.y != 0) ? leading_zeros(a.y) : 32;
    int zeros_z = (a.z != 0) ? leading_zeros(a.z) : 32;
    int min_zeros = 32;
    if (zeros_x < min_zeros) min_zeros = zeros_x;
    if (zeros_y < min_zeros) min_zeros = zeros_y;
    if (zeros_z < min_zeros) min_zeros = zeros_z;

    int sum_shift_right = 0;
    if (min_zeros < 32) {
        int zeros_sum = (min_zeros * 2);
        sum_shift_right = 36 - zeros_sum;
    }

    int sum_shifted;
    if (sum_shift_right >= 0) sum_shifted = sum >> sum_shift_right;
    if (sum_shift_right < 0) sum_shifted = sum << -sum_shift_right;
    scalar_t sqrt = int_sqrt(sum_shifted);
    if (sum_shift_right >= 0) sqrt <<= (sum_shift_right/2);
    if (sum_shift_right < 0) sqrt >>= -(sum_shift_right/2);
    return sqrt;
    #endif
}

static inline scalar_t vec3_distance(vec3_t a, vec3_t b) {
    return vec3_magnitude(vec3_sub(a, b));
}

static inline vec3_t vec3_neg(vec3_t a) {
    return (vec3_t) {
        -a.x,
        -a.y,
        -a.z,
    };
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    #if _FLOAT
    return (vec3_t) {
        scalar_mul(a.y, b.z) - scalar_mul(a.z, b.y),
        scalar_mul(a.z, b.x) - scalar_mul(a.x, b.z),
        scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x),
    };
    #else
    return (vec3_t) {
        (((int64_t)a.y * (int64_t)b.z) - ((int64_t)a.z * (int64_t)b.y)) / FIXED_ONE,
        (((int64_t)a.z * (int64_t)b.x) - ((int64_t)a.x * (int64_t)b.z)) / FIXED_ONE,
        (((int64_t)a.x * (int64_t)b.y) - ((int64_t)a.y * (int64_t)b.x)) / FIXED_ONE,
    };
    #endif
}

static inline vec3_t vec3_cross_lh(vec3_t a, vec3_t b) {
    return vec3_neg(vec3_cross(a, b));
}

static inline vec3_t vec3_normalize(vec3_t a) {
    scalar_t magnitude = vec3_magnitude(a);
    if (magnitude == 0) {
        return vec3_from_int32s(0, 0, 0);
    }
    if (magnitude < 4096) {
        a = vec3_shift_left(a, 8);
        magnitude = vec3_magnitude(a);
    }

    const vec3_t a_normalized = vec3_divs(a, magnitude);
    return a_normalized;
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

static inline size_t serialize_vec3(void* destination, vec3_t vec) {
    assert(destination);
    intptr_t dest_i = (intptr_t)destination;
    serialize_scalar((void*)(dest_i + offsetof(vec3_t, x)), vec.x);
    serialize_scalar((void*)(dest_i + offsetof(vec3_t, y)), vec.y);
    serialize_scalar((void*)(dest_i + offsetof(vec3_t, z)), vec.z);
    return sizeof(vec);
}

static inline size_t deserialize_vec3(const void *const source, vec3_t* vec) {
    assert(source);
    assert(vec);
    intptr_t src_i = (intptr_t)source;
    deserialize_scalar((void*)(src_i + offsetof(vec3_t, x)), &vec->x);
    deserialize_scalar((void*)(src_i + offsetof(vec3_t, y)), &vec->y);
    deserialize_scalar((void*)(src_i + offsetof(vec3_t, z)), &vec->z);
    return sizeof(*vec);
}

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif

#endif // VEC3_H
