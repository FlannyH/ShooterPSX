#ifndef FILE_H
#define FILE_H
#include <stdint.h>
#include <stddef.h>
#include "memory.h"

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack);

#endif