#include "vislist.h"
#include "file.h"

#define MAGIC_FVIS 0x53495646
typedef struct {
    uint32_t file_magic; // File magic: "FVIS"
    uint32_t offset_vis_bvh; // The number of sections in this vislist header
    uint32_t offset_vis_lists; // The number of sections in this vislist header
} vislist_header_t;

vislist_t vislist_load(const char* path, int on_stack, stack_t stack) {
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Get header data
    vislist_header_t* vislist_header = (vislist_header_t*)file_data;

    // Ensure FMSH header is valid
    vislist_t vislist = {0};
    if (vislist_header->file_magic != MAGIC_FVIS) { // "FVIS"
        printf("[ERROR] Error loading vislist '%s', file header is invalid!\n", path);
        return vislist;
    }

    // Get pointers to data
    intptr_t binary_section = (intptr_t)(vislist_header + 1);
    vislist.bvh_root = (visbvh_node_t*)(binary_section + vislist_header->offset_vis_bvh);
    vislist.vislists = (visfield_t*)(binary_section + vislist_header->offset_vis_lists);
    return vislist;
}
