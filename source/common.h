#ifndef COMMON_H
#define COMMON_H
#ifdef _DEBUG
#include <stdlib.h>
#endif

//#define PAL

static void panic_if(const char* error_if_false, const int condition, const int line, const char* file) {
#ifdef _DEBUG
    if (condition) {
        printf("%s @ %i: assert failed! %s\n", file, line, error_if_false);
        exit(1);
    }
#endif
}

static void warn_if(const char* error_if_false, const int condition, const int line, const char* file) {
#ifdef _DEBUG
    if (condition) {
        printf("%s @ %i: assert failed! %s\n", file, line, error_if_false);
    }
#endif
}

#define WARN_IF(error_if_false, condition) (warn_if(error_if_false, condition, __LINE__, __FILE__))
#define PANIC_IF(error_if_false, condition) (panic_if(error_if_false, condition, __LINE__, __FILE__))

#endif
