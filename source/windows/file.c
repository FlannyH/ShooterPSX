#include "file.h"

#include "memory.h"

#include <string.h>
#include <stdio.h>

#define MAGIC_FSFA 0x41465346
typedef struct {
    uint32_t file_magic; // Must be equal to MAGIC_FSFA
    uint32_t n_items; // How many items does this file contain
    uint32_t items_offset; // Offset (bytes) to the items list, relative to the start of the file
    uint32_t data_offset; // Offset (bytes) to the data section, relative to the start of the file
} fsfa_header_t;

typedef enum {
    FSFA_ITEM_TYPE_FOLDER = 0,
    FSFA_ITEM_TYPE_FILE = 1,
} fsfa_item_type_t;

typedef struct {
    uint8_t type; // value of type `fsfa_item_type_t`
    char name[12]; // The item's name. This string is null terminated if the length is below 12, and not null terminated if the length is equal to 12
    char extension[3]; // The item's file extension. This field is ignored for folders.
    uint32_t offset; // Folders: First child index. Files: Offset to file contents, relative to the start of the binary section.
    uint32_t size; // Folders: how many items are stored in the folder. Files: file size in bytes
} fsfa_item_t;

FILE* file_archive = NULL;
fsfa_item_t* item_table = NULL;

void file_init(const char* path) {
    // Modify file name to not be CD based
    const size_t length = strlen(path) + 2; // Include the null terminator
    char* new_path = mem_stack_alloc(length, STACK_TEMP);

    if (path[length - 3] == ';') {    
        new_path = mem_stack_alloc(length, STACK_TEMP);
        new_path[0] = '.';
        memcpy(&new_path[1], path, length - 2);
        new_path[length - 2] = 0;
    }
    else if (path[0] == '\\' || path[0] == '/') {
        new_path = mem_stack_alloc(length, STACK_TEMP);
        strcpy(new_path, path + 1); // Get rid of the leading '/'
    }
    else {
        strcpy(new_path, path);
    }

    // Convert slashes to forward slashes, Linux build will cry otherwise
    for (size_t x = 0; x < length - 2; ++x) {
        if (new_path[x] == '\\') {
            new_path[x] = '/';
        }
    }

    // Open file
    file_archive = fopen(new_path, "rb");
    if (file_archive == NULL) {
        printf("[ERROR] Failed to load file archive \"%s\"\n", new_path);
        return 0;
    }

    // Read file table
    fsfa_header_t header;
    fread(&header, sizeof(header), 1, file_archive) < sizeof(header);
    item_table = mem_alloc(header.n_items * sizeof(fsfa_item_t), MEM_CAT_FILE);

    fseek(item_table, header.items_offset, SEEK_SET);
    fread(item_table, sizeof(fsfa_item_t), header.n_items, file_archive);
}

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack) {
    // Find file in item table
    size_t queue[16];
    queue[0] = 0;
    size_t queue_cursor = 0;
    size_t queue_end = 1;
    size_t name_start = 0;
    size_t name_end = 0;

    if (path[0] == '\0') {
        printf("[ERROR] Empty file path\n");
    }

    if (item_table[0].type != FSFA_ITEM_TYPE_FOLDER) {
        printf("[ERROR] Failed to read file archive: first entry in item table is not a folder\n");
    }

    for (int i = 0; i < item_table[0].size; ++i) {
        queue[i] = i + 1;
        ++queue_end;
    }

    while (queue_cursor < queue_end) {
        if (path[name_start] == '0') return 0;

        const fsfa_item_t* const item = &item_table[queue[queue_cursor]];

        // find next name in path string
        name_end = name_start;
        while (1) {
            if (path[name_end] == '0') break;
            if (path[name_end] == '/') break;
            if (path[name_end] == '\\') break;
            if (path[name_end] == '.') break;
            ++name_end;
        }

        for (int i = name_start; i < name_end; ++i) printf("%c", path[i]);
        printf("\n");

        // check if it matches the name of this queue entry's file
        int match = 1;
        for (size_t i = 0; i < sizeof(item->name); ++i) {
            if ((name_start + i) >= name_end) break;
            if (item->name[i] == '0') break;
            printf("    %c - %c\n", path[name_start + i], item->name[i]);
            if (path[name_start + i] != item->name[i]) { match = 0; break; }
        }
        printf("match: %i\n", match);

        if (!match) goto next;

        // todo: if folder, add subitems to queue
        while (1) {}

        next:
        ++queue_cursor;
    }

    // Allocate memory for the data
	if (on_stack) *destination = (uint32_t*)mem_stack_alloc(*size, stack);
	else *destination = (uint32_t*)mem_alloc(*size, MEM_CAT_FILE);

    // Read the data
    fread(*destination, sizeof(char), *size, file_archive);
    printf("[INFO] Loaded file %s\n", path);
    return 1;
}
