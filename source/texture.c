#include "texture.h"

#include "file.h"
#include "memory.h"

#include <stdio.h>

uint32_t texture_collection_load(const char* path, texture_cpu_t** out_textures, int on_stack, stack_t stack) { // returns number of textures loaded
    if (out_textures == NULL) return 0;

    // Read the file
    uint32_t* file_data;
    size_t size;
    file_read(path, &file_data, &size, on_stack, stack);

    // Read the texture collection header
    tex_col_header_t* tex_col_hdr = (tex_col_header_t*)file_data;

    // Verify file magic
    if (tex_col_hdr->file_magic != MAGIC_FTXC) {
        printf("[ERROR] Error loading texture collection '%s', file header is invalid!\n", path);
        return 0;
    }

    // Allocate space for TextureCPU structs
	if (on_stack) *out_textures = mem_stack_alloc(sizeof(texture_cpu_t) * tex_col_hdr->n_texture_cell, stack);
	else *out_textures = mem_alloc(sizeof(texture_cpu_t) * tex_col_hdr->n_texture_cell, MEM_CAT_TEXTURE);

    // Find the data sections
    const void* binary_section = &tex_col_hdr[1];
    const texture_cell_desc_t* texture_cell_descs = (texture_cell_desc_t*)((intptr_t)binary_section + tex_col_hdr->offset_texture_cell_descs);
    pixel16_t* palette_data = (pixel16_t*)((intptr_t)binary_section + tex_col_hdr->offset_palettes);
    uint8_t* texture_data = (uint8_t*)((intptr_t)binary_section + tex_col_hdr->offset_textures);

    // Create a TextureCPU object for each
    for (size_t i = 0; i < tex_col_hdr->n_texture_cell; ++i) {
        const texture_cell_desc_t* tex_cell_desc = &texture_cell_descs[i];
        texture_cpu_t* texture_cpu = &(*out_textures)[i];

        // Get data
        texture_cpu->palette = &palette_data[(size_t)tex_cell_desc->palette_index * (16 * 16)];
        texture_cpu->data = &texture_data[(size_t)tex_cell_desc->sector_offset_texture * 2048];
        texture_cpu->width = tex_cell_desc->texture_width;
        texture_cpu->height = tex_cell_desc->texture_height;
        texture_cpu->avg_color = tex_cell_desc->avg_color;
    }

    if (!on_stack) {
        mem_delayed_free(file_data);
        mem_delayed_free(*out_textures);
    }

    return tex_col_hdr->n_texture_cell;
}
