#ifndef PSX_H
#define PSX_H
#include <math.h>

#define PI 3.14159265358979

int hisin(int a) {
    // Bring angle from [0, 4194304] -> [0, 2pi]
    double angle_radians = (double)a * (2.0f * PI / 1048576.0f);

    // Calculate the sine and convert it from [0.0, 1.0] to [0, 4096]
    int sine = (int)(sin(angle_radians) * 4096.0f);
    return sine;
}

int hicos(int a) {
    // Bring angle from [0, 4194304] -> [0, 2pi]
    float angle_radians = ((float)a) * (2.0f * PI / 1048576.0f);

    // Calculate the cosine and convert it from [0.0, 1.0] to [0, 4096]
    int cosine = (int)(cos(angle_radians) * 4096.0f);
    return cosine;
}

/*
int FntPrint(int id, const char* fmt, ...) {
    return 0;
}*/

char* FntFlush(int id) {
    return 0;
}

#endif