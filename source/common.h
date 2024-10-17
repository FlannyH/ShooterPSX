#ifndef COMMON_H
#define COMMON_H
#include <stdlib.h>
#include <stdio.h>

#ifndef _NDS
//#define PAL
//#define _DEBUG_CAMERA
//#define _DEBUG
#endif

// #define BENCHMARK_MODE
#define FPS_COUNTER

#define ALWAYS_INLINE __attribute__((always_inline)) inline

#define TODO()  printf("%s:%i: todo!\n", __FILE__, __LINE__); \
                while(1) {;;}
#define TODO_SOFT()  printf("%s:%i: todo!\n", __FILE__, __LINE__);
#define REACH_TEST() printf("%s:%i: ok\n", __FILE__, __LINE__);

#define KiB (1024)
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)
#define ONE (1 << 12)

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

#ifdef _WINDOWS
    #define PROFILE(name, function, timer) function
#elif defined(_NDS)
    #define PROFILE(name, function, timer) function
#else
    #ifdef _DEBUG
        #define PROFILE(name, function, timer) { \
            TIMER_CTRL(timer) = 0b0100000000;\
            TIMER_VALUE(timer) = 0;\
            function; \
            FntPrint(-1, "%s: %i\n", name, TIMER_VALUE(timer) & 0xFFFF); \
        } \

    #else
        #define PROFILE(name, function, timer) function
    #endif
#endif

#ifdef _PSX
    #define SCRATCHPAD ((void*)0x1F800000)
#endif

#endif