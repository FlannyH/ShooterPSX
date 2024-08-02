#ifndef PSX_H
#define PSX_H

#include "psx.h"

#include <math.h>

#define PI 3.14159265358979

int hisin(const int a) {
    // Bring angle from [0, 4194304] -> [0, 2pi]
    const double angle_radians = (double)a * (2.0 * PI / 131072.0);

    // Calculate the sine and convert it from [0.0, 1.0] to [0, 4096]
    const int sine = (int)(sin(angle_radians) * 4096.0);
    return sine;
}

int hicos(const int a) {
    // Bring angle from [0, 4194304] -> [0, 2pi]
    const double angle_radians = ((double)a) * (2.0 * PI / 131072.0);

    // Calculate the cosine and convert it from [0.0, 1.0] to [0, 4096]
    const int cosine = (int)(cos(angle_radians) * 4096.0);
    return cosine;
}

int isin(const int a) {
    // Bring angle from [0, 4096] -> [0, 2pi]
    const double angle_radians = (double)a * (2.0 * PI / 4096.0);

    // Calculate the sine and convert it from [0.0, 1.0] to [0, 4096]
    const int sine = (int)(sin(angle_radians) * 4096.0);
    return sine;
}

int icos(const int a) {
    // Bring angle from [0, 4096] -> [0, 2pi]
    const double angle_radians = ((double)a) * (2.0 * PI / 4096.0);

    // Calculate the cosine and convert it from [0.0, 1.0] to [0, 4096]
    const int cosine = (int)(cos(angle_radians) * 4096.0);
    return cosine;
}

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