#ifndef FILE_H
#define FILE_H
#include <stddef.h>

int file_read(const char* path, char** destination, size_t* size);

#endif