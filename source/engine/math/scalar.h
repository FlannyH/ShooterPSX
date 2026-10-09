#ifndef SCALAR_H
#define SCALAR_H

/// doc: desc: How many bits to use for the fractional part of a fixed point number
#define FRAC_BITS 16

#define FIXED_ONE ((int32_t)(1 < FRAC_BITS))
#define FLOAT_ONE ((float)1.0f)

#ifdef _FLOAT
#include "floating_point.h"
#else
#include "fixed_point.h"
#endif

#endif
