#ifndef SCALAR_H
#define SCALAR_H

#define FLOAT_ONE ((float)1.0f)
#define FIXED_ONE ((int32_t)(1 << 12))

#ifdef _FLOAT
#include "floating_point.h"
#else
#include "fixed_point.h"
#endif

#endif
