#ifndef COMMON_H
#define COMMON_H
#ifdef _DEBUG
#include <stdlib.h>
#endif

//#define PAL
#define DEBUG_CAMERA
#define _DEBUG

#ifdef _WIN32
#define ALWAYS_INLINE inline
#else
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

static void panic_if(const char* error_if_false, const int condition, const int line, const char* file) {
#ifdef _DEBUG
    if (condition) {
        printf("%s @ %i: assert failed! %s\n", file, line, error_if_false);
    }
#else
    (void)error_if_false;
    (void)condition;
    (void)line;
    (void)file;
#endif
}

static void warn_if(const char* error_if_false, const int condition, const int line, const char* file) {
#ifdef _DEBUG
    if (condition) {
        printf("%s @ %i: assert failed! %s\n", file, line, error_if_false);
    }
#else
    (void)error_if_false;
    (void)condition;
    (void)line;
    (void)file;
#endif
}

#define WARN_IF(error_if_false, condition) (warn_if(error_if_false, condition, __LINE__, __FILE__))
#define PANIC_IF(error_if_false, condition) (panic_if(error_if_false, condition, __LINE__, __FILE__))

#ifdef _WIN32
#define PROFILE(name, function, timer) function
#else
#ifdef _DEBUG
#define PROFILE(name, function, timer) { \
    TIMER_CTRL(timer) = 0b0100000000;\
    TIMER_VALUE(timer) = 0;\
    function; \
    FntPrint(-1, "%s: %i\n", name, TIMER_VALUE(timer) & 0xFFFF); \
} \

#endif
#endif

#endif