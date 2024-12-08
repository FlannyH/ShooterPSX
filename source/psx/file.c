#ifdef _PSX
#include "file.h"

#include "memory.h"

#include <string.h>
#include <stdio.h>
#include <psxcd.h> // Disc IO
#include <common.h>

int file_archive_sector = -1;
fsfa_header_t archive_header;
fsfa_item_t* item_table = NULL;

void seek_in_archive(int offset_bytes) {
#ifdef _DEBUG
    if (offset_bytes % 2048 != 0) {
        printf("[ERROR] Misaligned disc read\n");
        while(1){};
    }
#endif

    CdlLOC loc;
    CdIntToPos((offset_bytes / 2048) + file_archive_sector, &loc);
    CdControl(CdlSetloc, &loc, 0);
}

void file_init(const char* path) {
    int test = 0;
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
        printf("[ERROR] Failed to load file %s\n", path);
        return;
    }
    file_archive_sector = CdPosToInt(&file.pos);

    // Fetch the header
    mem_stack_release(STACK_TEMP);
    uint32_t* temp_header_storage = (uint32_t*)mem_stack_alloc(64 * KiB, STACK_TEMP);
    CdControl(CdlSetloc, &file.pos, 0);
    CdRead(1, temp_header_storage, CdlModeSpeed);
    CdReadSync(0, 0);

    const fsfa_header_t* const header = (fsfa_header_t*)temp_header_storage;

    // Verify file magic
    if (header->file_magic != MAGIC_FSFA) {
        printf("[ERROR] Failed to load file archive \"%s\": incorrect file magic\n", new_path);
        return;
    }

    // Copy file table to a more permanent location in RAM
    memcpy(&archive_header, header, sizeof(fsfa_header_t));
    size_t item_table_size = archive_header.n_items * sizeof(fsfa_item_t);
    item_table_size = (item_table_size | 2047) + 1; // align to next multiple of 2048 (disc sector size)
    uint32_t* item_table_storage = mem_alloc(item_table_size, MEM_CAT_FILE);
    seek_in_archive(archive_header.items_offset & ~2047);
    CdRead(item_table_size / 2048, item_table_storage, CdlModeSpeed);
    CdReadSync(0, 0);
    item_table = (fsfa_item_t*)(item_table_storage + (archive_header.items_offset / sizeof(uint32_t)));
}

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack) {
    if (path[0] == '\0') {
        printf("[ERROR] Empty file path\n");
    }

    if (item_table[0].type != FSFA_ITEM_TYPE_FOLDER) {
        printf("[ERROR] Failed to read file archive: first entry in item table is not a folder\n");
    }

    size_t subitem_cursor = 1;
    size_t subitem_counter = item_table[0].size;
    size_t name_start = 0;
    size_t name_end = 0;

    int find_next_name = 1;

    while (subitem_counter > 0) {
        if (path[name_start] == '0') return 0;

        const fsfa_item_t* const item = &item_table[subitem_cursor];

        if (find_next_name) {
            find_next_name = 0;
            name_end = name_start;
            while (1) {
                if (path[name_end] == '0') break;
                if (path[name_end] == '/') break;
                if (path[name_end] == '\\') break;
                if (path[name_end] == '.') break;
                ++name_end;
            }
        }

        for (uint32_t i = name_start; i < name_end; ++i) printf("%c", path[i]);
        printf("\n");

        // compare item name to current path name
        int match = 1;
        for (size_t i = 0; i < sizeof(item->name); ++i) {
            if ((name_start + i) >= name_end) break;
            if (item->name[i] == '0') break;
            printf("    %c - %c\n", path[name_start + i], item->name[i]);
            if (path[name_start + i] != item->name[i]) { match = 0; break; }
        }
        printf("match: %i\n", match);

        if (!match) {
            --subitem_counter;
            ++subitem_cursor;
            continue;
        }

        // if we matched a folder, clear the queue and fill it with its subitems
        if (item->type == FSFA_ITEM_TYPE_FOLDER) {
            subitem_counter = item->size;
            subitem_cursor = item->offset;
            name_start = name_end + 1;
            find_next_name = 1;
            printf("moving into subfolder\n");
            continue;
        }

        // otherwise verify extension and then load file data
        if (item->extension[0] != '\0') {
            int extension_match = 1;
            const size_t extension_start = name_end + 1; // if it has an extension, this will be right after the dot
            printf(".%s\n", &path[extension_start]);
            for (size_t i = 0; i < sizeof(item->extension); ++i) {
                printf("    %c - %c\n", path[extension_start + i], item->extension[i]);
                if (item->extension[i] != path[extension_start + i]) { extension_match = 0; break; }
                if (item->extension[i] == '\0') break;
                if (path[extension_start + i] == '\0') break;
            }
            printf("extension_match: %i\n", extension_match);
            if (!extension_match) {
                --subitem_counter;
                ++subitem_cursor;
                continue;
            };
        }

        const size_t item_size_aligned = (item->size | 2047) + 1;
        if (on_stack) *destination = (uint32_t*)mem_stack_alloc(item_size_aligned, stack);
        else *destination = (uint32_t*)mem_alloc(item_size_aligned, MEM_CAT_FILE);
        *size = item->size;
        printf("size: %i\n", *size);
        printf("item_size_aligned: %i\n", item_size_aligned);
        seek_in_archive(item->offset + archive_header.data_offset);
        CdRead(item_size_aligned / 2048, *destination, CdlModeSpeed);
        CdReadSync(0, 0);

        printf("file loaded!\n");
        char* a = (char*)*destination;
        printf("first 4 bytes: %c%c%c%c\n", a[0], a[1], a[2], a[3]);

        return 1;
    }

    return 0;
}

#endif
