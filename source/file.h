#ifndef FILE_H
#define FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "memory.h"

#include <stdint.h>
#include <stddef.h>

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

void file_init(const char* path);
int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack);

#ifdef __cplusplus
}
#endif
#endif
