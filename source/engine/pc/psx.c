#ifndef PSX_H
#define PSX_H

#include "psx.h"
#include "../common.h"

#include <math.h>

#define PI 3.14159265358979


char* FntFlush(int id) {
    (void)id;
    return 0;
}

int FntPrint(int id, const char* fmt, ...) {
    (void)id;
    (void)fmt;
    return 0;
}

#endif
