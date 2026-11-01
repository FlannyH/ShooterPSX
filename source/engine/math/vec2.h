#ifndef VEC2_H
#define VEC2_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include "scalar.h"
#include <assert.h>
#include <stddef.h>

// todo(vec2_gte_opt): desc: psx gte-accelerated svec2_t math
// todo(vec2_inline): desc: is inlining the functions actually faster?

typedef struct {
    scalar_t x, y;
} vec2_t;

typedef struct {
    int16_t x, y; // 2D position
} svec2_t;

/// doc: desc: Construct a `vec2_t` from two float values, converting each component to a scalar value in the process
static inline vec2_t vec2_from_floats(const float x, const float y) {
    return (vec2_t) {
        SCALAR(x),
        SCALAR(y),
    };
}

/// doc: desc: Print a formatted `vec2_t` using `print_scalar`
static inline void vec2_debug(const vec2_t a) {
    print_scalar(a.x);
    printf(", ");
    print_scalar(a.y);
    printf("\n");
}

/// doc: desc: Construct a `vec2_t`, setting each component to the same scalar value `a`
static inline vec2_t vec2_from_scalar(const scalar_t a) {
    return (vec2_t){ a, a };
}

/// doc: desc: Construct a `vec2_t` from two scalar values
static inline vec2_t vec2_from_scalars(const scalar_t x, const scalar_t y) {
    return (vec2_t){ x, y };
}

/// doc: desc: Create a `vec2_t` from two integers, converting them into scalar values
static inline vec2_t vec2_from_ints(int x, int y) {
    return (vec2_t){ SCALAR(x), SCALAR(y) };
}

/// doc: desc: Construct a `vec2_t` from the two 16-bit integers in `vec`
static inline vec2_t vec2_from_svec2(svec2_t vec) {
    return (vec2_t) {
        (scalar_t)vec.x * ONE,
        (scalar_t)vec.y * ONE,
    };
}

/// doc: desc: Construct an `svec2_t` from a fixed point `vec2_t`
static inline svec2_t svec2_from_vec2(vec2_t vec) {
    return (svec2_t) {
        (int16_t)(vec.x / ONE),
        (int16_t)(vec.y / ONE),
    };
}

/// doc: desc: Add each component from `a` and `b` together separately , and return the result
static inline vec2_t vec2_add(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x + b.x),
        (a.y + b.y),
    };
}

/// doc: desc: Add a single value `b` to each component of `a`
static inline vec2_t vec2_adds(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        (a.x + b),
        (a.y + b),
    };
}

/// doc: desc: Subtract each component from `a` and `b` from eachother, and return the result
static inline vec2_t vec2_sub(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        (a.x - b.x),
        (a.y - b.y),
    };
}

/// doc: desc: Subtract a single value `b` from each component of `a`, and return the result
static inline vec2_t vec2_subs(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        (a.x - b),
        (a.y - b),
    };
}

/// doc: desc: Multiply each component from `a` and `b` together separately , and return the result
static inline vec2_t vec2_mul(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b.x),
        scalar_mul(a.y, b.y),
    };
}

/// doc: desc: Multiply each component from `a` with a single value `b`, and return the result
static inline vec2_t vec2_muls(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        scalar_mul(a.x, b),
        scalar_mul(a.y, b),
    };
}

/// doc: desc: Divide each component from `a` with each component from `b`, and return the result
static inline vec2_t vec2_div(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_div(a.x, b.x),
        scalar_div(a.y, b.y),
    };
}

/// doc: desc: Divide each component from `a` with a single value `b`, and return the result
static inline vec2_t vec2_divs(const vec2_t a, const scalar_t b) {
    return (vec2_t) {
        scalar_div(a.x, b),
        scalar_div(a.y, b),
    };
}

/// doc: desc: Calculate the dot product between `a` and `b`, and return the result
static inline scalar_t vec2_dot(const vec2_t a, const vec2_t b) {
    return scalar_mul(a.x, b.x)
    +      scalar_mul(a.y, b.y);
}

/// doc: desc: Get the lowest value for each separate component of `a` and `b`
static inline vec2_t vec2_min(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_min(a.x, b.x),
        scalar_min(a.y, b.y),
    };
}

/// doc: desc: Get the highest value for each separate component of `a` and `b`
static inline vec2_t vec2_max(const vec2_t a, const vec2_t b) {
    return (vec2_t) {
        scalar_max(a.x, b.x),
        scalar_max(a.y, b.y),
    };
}

/// doc: desc: Shift each component of `a` left by `amount` bits. Equivalent to `a * (1 << shift)`
static inline vec2_t vec2_shift_left(const vec2_t a, const int amount) {
    return (vec2_t) {
        scalar_shift_left(a.x, amount),
        scalar_shift_left(a.y, amount),
    };
}

/// doc: desc: Shift each component `a` right by `amount` bits. Equivalent to `a / (1 << shift)`
static inline vec2_t vec2_shift_right(const vec2_t a, const int amount) {
    return (vec2_t) {
        scalar_shift_right(a.x, amount),
        scalar_shift_right(a.y, amount),
    };
}

/// doc: desc: Calculate the squared magnitude/length of `a`. If we don't need the exact distance, but just need something to compare distances, this is more efficient to use than `vec2_magnitude`, as we can avoid the square root at the end
static inline scalar_t vec2_magnitude_squared(const vec2_t a) {
    const scalar_t x2 = scalar_mul(a.x, a.x);
    const scalar_t y2 = scalar_mul(a.y, a.y);

    if (is_infinity(x2) || is_infinity(y2)) {
        return INT32_MAX;
    }

    return x2 + y2;
}

/// doc: desc: Calculate the magnitude/length of `a`. This involves a square root operation, so use this wisely, and consider `vec2_magnitude_squared`
static inline scalar_t vec2_magnitude(vec2_t a) {
    if (a.x == 0 && a.y == 0) {
        return 0;
    }

    #ifdef _FLOAT
    return sqrtf((a.x * a.x) + (a.y * a.y));
    #else
    if (is_infinity(a.x) || is_infinity(a.y)) {
        return INT32_MAX;
    }
    int64_t sum = 0;
    sum += ((int64_t)a.x * (int64_t)a.x);
    sum += ((int64_t)a.y * (int64_t)a.y);

    int zeros_x = (a.x != 0) ? leading_zeros(a.x) : 32;
    int zeros_y = (a.y != 0) ? leading_zeros(a.y) : 32;
    int min_zeros = 32;
    if (zeros_x < min_zeros) min_zeros = zeros_x;
    if (zeros_y < min_zeros) min_zeros = zeros_y;

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

/// doc: desc: Calculate the distance between two vectors, by calling `vec2_magnitude` on `b - a`
static inline scalar_t vec2_distance(vec2_t a, vec2_t b) {
    return vec2_magnitude(vec2_sub(a, b));
}

/// doc: desc: Return `-a`, as in `a` with all its components negated
static inline vec2_t vec2_neg(vec2_t a) {
    return (vec2_t) {
        -a.x,
        -a.y,
    };
}

/// doc: desc: Calculate the right-handed cross product
static inline scalar_t vec2_cross(const vec2_t a, const vec2_t b) {
    return (scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x));
}

/// doc: desc: Calculate the left-handed cross product
static inline scalar_t vec2_cross_lh(const vec2_t a, const vec2_t b) {
    return (scalar_mul(a.x, b.y) - scalar_mul(a.y, b.x));
}

/// doc: desc:  Return a vector with the same direction, but length 1.0. by calculating `a / vec2_magnitude(a)`, correcting for fixed point overflows
static inline vec2_t vec2_normalize(vec2_t a) {
    // avoid overflows
    while ((scalar_abs(a.x) > (2*ONE)) || (scalar_abs(a.y) > (2*ONE))) {
        a = vec2_shift_right(a, 2);
    }

    scalar_t magnitude = vec2_magnitude_squared(a);
    if (magnitude == 0) {
        return vec2_from_ints(0, 0);
    }
    magnitude = scalar_sqrt(magnitude);

    const vec2_t a_normalized = vec2_divs(a, magnitude);
    return a_normalized;
}

/// doc: desc: Clamp each component between min and max separately, see `scalar_clamp`
static inline vec2_t vec2_clamp(vec2_t a, vec2_t min, vec2_t max) {
    return (vec2_t) {
        scalar_clamp(a.x, min.x, max.x),
        scalar_clamp(a.y, min.y, max.y),
    };
}

/// doc: desc: Linearly interpolate each component separately, see `scalar_lerp`
static inline vec2_t vec2_lerp(vec2_t a, vec2_t b, scalar_t t) {
    return (vec2_t) {
        a.x + scalar_mul(b.x - a.x, t),
        a.y + scalar_mul(b.y - a.y, t),
    };
}

/// doc: desc: Returns 1 if the two vectors are equal, 0 otherwise
static inline int vec2_equal(vec2_t a, vec2_t b) {
    return (a.x == b.x) && (a.y == b.y);
}

/// doc: desc: Serializes and writes a `vec2_t` to `destination`
/// doc: desc: Returns number of bytes written
/// doc: param(void* destination): Where to store the serialized vector
/// doc: param(vec2_t vec): Which vector to serialize
static inline size_t serialize_vec2(void* destination, vec2_t vec) {
    intptr_t dest_i = (intptr_t)destination;
    serialize_scalar((void*)(dest_i + offsetof(vec2_t, x)), vec.x);
    serialize_scalar((void*)(dest_i + offsetof(vec2_t, y)), vec.y);
    return sizeof(vec);
}

/// doc: desc: Reads and deserializes a scalar, and then returns that scalar
/// doc: param(void* source): Where to read the serialized vector from
/// doc: param(vec2_t* vec): Where to store the deserialized vector to
static inline size_t deserialize_vec2(const void *const source, vec2_t* vec) {
    assert(source);
    assert(vec);
    intptr_t src_i = (intptr_t)source;
    deserialize_scalar((void*)(src_i + offsetof(vec2_t, x)), &vec->x);
    deserialize_scalar((void*)(src_i + offsetof(vec2_t, y)), &vec->y);
    return sizeof(*vec);
}

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif

#endif
