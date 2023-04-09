#ifndef TEXTURE_H
#define TEXTURE_H
#include <stdint.h>

// Texture Collection file header
typedef struct {
    uint32_t file_magic;               // File identifier magic, always "FTXC" or 0x46545843
    uint32_t n_texture_cell;           // Number of texture cells in this file. This is also the number of palettes in the file, as we assume each texture has its own 16 color palette.
    uint32_t offset_texture_cell_descs;// Offset into the binary section to the start of the array of TextureCellDesc structs.
    uint32_t offset_palettes;          // Offset to the color palettes section, relative to the end of this header. Each color is a 16-bit depth color.
    uint32_t offset_textures;          // Offset to the raw texture data section.
    uint32_t offset_name_table;        // Offset to an array of offsets into the binary section. The offsets point to null-terminated strings. The names are stored in the same order as the texture cells, so the same index can be used for both arrays. Used for debugging, and this value should be 0xFFFFFFFF if the table is not included in the file.
} tex_col_header_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} pixel32_t;
static const pixel32_t red = { 255,0,0,255 };
static const pixel32_t green = { 0,255,0,255 };
static const pixel32_t blue = { 0,0,255,255 };
static const pixel32_t pink = { 255,0,255,255 };
static const pixel32_t white = { 255, 0, 255, 255 };

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint16_t r : 5;
    uint16_t g : 5;
    uint16_t b : 5;
    uint16_t a : 1;
} pixel16_t;

// TextureCellDesc
typedef struct {
    uint8_t sector_offset_texture;     // Offset into raw texture data section.
    uint8_t palette_index;             // Palette index.
    uint8_t texture_width;             // Texture width in pixels.
    uint8_t texture_height;            // Texture height in pixels.
    pixel32_t avg_color;
} texture_cell_desc_t;

typedef struct {
    pixel16_t* palette;
    uint8_t* data;
    uint8_t width;
    uint8_t height;
    pixel32_t avg_color;
} texture_cpu_t;
#pragma pack(pop)

uint32_t texture_collection_load(const char* path, texture_cpu_t** out_textures);

#endif