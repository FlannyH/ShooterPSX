#ifdef _PSX
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <psxcd.h> // Disc IO

int file_read(const char* path, uint32_t** destination, size_t* size) {
    // Search for the file on disc - return false if it does not exist
    CdlFILE file;
    if (!CdSearchFile(&file, path)) {
        printf("Can't find file %s!", path);
        return 0;
    }

    // Snap file size to 2048 byte grid
    size_t file_size = (file.size | 2047) + 1;
    
    // Return the actual file size (not rounded upwards)
    *size = file.size;

    // Allocate enough bytes for the destination buffer
    *destination = (uint32_t*)malloc(file_size);

    // Seek to the file and read it
    CdControl(CdlSetloc, &file.pos, 0);
    CdRead(file_size / 2048, *destination, CdlModeSpeed);

    // Wait for it to finish reading
    CdReadSync(0, 0);

    return 1;
}

#endif