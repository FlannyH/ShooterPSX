#include "file.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack) {
    // Modify file name to not be CD based
    const size_t length = strlen(path) + 1; // Include the null terminator
    char* new_path = mem_alloc(length + 1 - 2, MEM_CAT_UNDEFINED); // Length from strlen + 1 for the period at the start, -2 for the ;1 at the end, and +1 for the null terminator
    new_path[0] = '.';
    memcpy(&new_path[1], path, length - 2);
    new_path[length - 2] = 0;

    // Convert slashes to forward slashes, Linux build will cry otherwise
    for (size_t x = 0; x < length - 2; ++x) {
        if (new_path[x] == '\\') {
            new_path[x] = '/';
        }
    }

    // Open file
    FILE* file = fopen(new_path, "rb");
    if (file == NULL) {
        printf("[ERROR] Failed to load file %s\n", new_path);
        return 0;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    *size = ((*size) + 3) & ~0x03; // align to 4 bytes
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the data
	*destination = (uint32_t*)mem_alloc(*size, MEM_CAT_FILE);
    memset(*destination, 0, (*size) + 1);

    // Read the data
    fread(*destination, sizeof(char), *size, file);
    printf("[INFO] Loaded file %s\n", new_path);
    return 1;
}
