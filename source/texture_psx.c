#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "texture.h"

uint32_t texture_collection_load(const char* path, TextureCPU** out_textures) { // returns number of textures loaded
    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size);

    // Read the texture collection header
    TextureCollectionHeader* tex_col_hdr = (TextureCollectionHeader*)file_data;

    // Verify file magic
    if (tex_col_hdr->file_magic != 0x43585446) {
        printf("[ERROR] Error loading texture collection '%s', file header is invalid!\n", path);
        return 0;
    }

    // Allocate space for TextureCPU structs
    *out_textures = malloc(sizeof(TextureCPU) * tex_col_hdr->n_texture_cell);

    // Find the data sections
    const void* binary_section = &tex_col_hdr[1];
    const TextureCellDesc* texture_cell_descs = (TextureCellDesc*)((intptr_t)binary_section + tex_col_hdr->offset_texture_cell_descs);
    Pixel16* palette_data = (Pixel16*)((intptr_t)binary_section + tex_col_hdr->offset_palettes);
    uint8_t* texture_data = (uint8_t*)((intptr_t)binary_section + tex_col_hdr->offset_textures);

    // Create a TextureCPU object for each
    for (size_t i = 0; i < tex_col_hdr->n_texture_cell; ++i) {
        const TextureCellDesc* tex_cell_desc = &texture_cell_descs[i];
        TextureCPU* texture_cpu = &(*out_textures)[i];

        // Get data
        texture_cpu->palette = &palette_data[(size_t)tex_cell_desc->palette_index * 16];
        texture_cpu->data = &texture_data[(size_t)tex_cell_desc->sector_offset_texture * 2048];
        texture_cpu->width = tex_cell_desc->texture_width;
        texture_cpu->height = tex_cell_desc->texture_height;
        texture_cpu->avg_color = tex_cell_desc->avg_color;
    }

    return tex_col_hdr->n_texture_cell;
}