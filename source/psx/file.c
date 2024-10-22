#ifdef _PSX
#include "file.h"

#include <string.h>
#include <stdio.h>
#include <psxcd.h> // Disc IO

#include "memory.h"

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack) {
    // Convert to upper case
    const size_t length = strlen(path) + 1; // Include the null terminator
    char* new_path = mem_stack_alloc(length, STACK_TEMP);
    for (size_t i = 0; i < length; ++i) {
        if (path[i] >= 'a' && path[i] <= 'z') {
            new_path[i] = path[i] - ('a' - 'A');
        }
        else {
            new_path[i] = path[i];
        }
    }

    // Search for the file on disc - return false if it does not exist
    CdlFILE file;
    if (!CdSearchFile(&file, new_path)) {
        printf("Can't find file %s!", new_path);
        return 0;
    }

    // Snap file size to 2048 byte grid
    size_t file_size = (file.size | 2047) + 1;
    
    // Return the actual file size (not rounded upwards)
    *size = file.size;

    // Allocate enough bytes for the destination buffer
	if (on_stack) *destination = (uint32_t*)mem_stack_alloc(file_size, stack);
	else *destination = (uint32_t*)mem_alloc(file_size, MEM_CAT_FILE);

    // Seek to the file and read it
    CdControl(CdlSetloc, &file.pos, 0);
    CdRead(file_size / 2048, *destination, CdlModeSpeed);

    // Wait for it to finish reading
    CdReadSync(0, 0);

    return 1;
}

#endif
