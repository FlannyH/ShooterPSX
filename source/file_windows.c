#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int file_read(const char* path, char** destination, size_t* size) {
    FILE* file;

    // Modify file name to not be CD based
    size_t length = strlen(path) + 1; // Include the null terminator
    char* new_path = malloc(length + 1 - 2); // Length from strlen + 1 for the period at the start, -2 for the ;1 at the end, and +1 for the null terminator
    new_path[0] = '.';
    memcpy_s(&new_path[1], length - 2, path, length - 2);
    new_path[length - 2] = 0;

    // Open file
    fopen_s(&file, new_path, "r");
    if (file == NULL) {
        printf("[ERROR] Failed to load file %s\n", new_path);
        return 0;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the data
    *destination = (char*)malloc((*size) + 1);
    memset(*destination, 0, (*size) + 1);

    // Read the data
    fread_s(*destination, *size, sizeof(char), *size, file);
    (*destination)[(*size)] = 0;
    printf("[INFO] Loaded file %s\n", new_path);
    return 1;
}
