#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int file_read(const char* path, uint32_t** destination, size_t* size) {
    FILE* file;

    // Modify file name to not be CD based
    const size_t length = strlen(path) + 1; // Include the null terminator
    char* new_path = malloc(length + 1 - 2); // Length from strlen + 1 for the period at the start, -2 for the ;1 at the end, and +1 for the null terminator
    new_path[0] = '.';
    memcpy(&new_path[1], path, length - 2);
    new_path[length - 2] = 0;

    // Convert slashes to forward slashes, Linux build will cry otherwise
    for (int x = 0; x < length - 2; ++x) {
        if (new_path[x] == '\\') {
            new_path[x] = '/';
        }
    }

    // Open file
    file = fopen(new_path, "rb");
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
    *destination = (uint32_t*)malloc((*size) + 1);
    memset(*destination, 0, (*size) + 1);

    // Read the data - in 2048 byte segments because for some reason reading the whole file did not work. typical
    for (size_t offset = 0; offset < *size; offset += 2048) {
        fread(*destination + (offset / 4), sizeof(char), 2048, file);
    }
    printf("[INFO] Loaded file %s\n", new_path);
    return 1;
}
