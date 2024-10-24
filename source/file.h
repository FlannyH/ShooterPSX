#ifndef FILE_H
#define FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "memory.h"

#include <stdint.h>
#include <stddef.h>

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack);

#ifdef __cplusplus
}
#endif
#endif
