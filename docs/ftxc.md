# Flan's PSX Texture Collection file specification
[Back to main page.](../README.md)

## Texture Collection file (.txc)
The file header is as follows
| Type    | Name                      | Description                                                                                                                                                                                                                                                                                                        |
| ------- | ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| char[4] | file_magic                | File identifier magic, always "FTXC"                                                                                                                                                                                                                                                                               |
| u32     | n_texture_cell            | Number of texture cells in this file. This is also the number of palettes in the file, as we assume each texture has its own 16 color palette.                                                                                                                                                                     |
| u32     | offset_texture_cell_descs | Offset into the binary section to the start of the array of TextureCellDesc structs.                                                                                                                                                                                                                               |
| u32     | offset_palettes           | Offset to the color palettes section, relative to the end of this header. Each color is a 16-bit depth color.                                                                                                                                                                                                      |
| u32     | offset_textures           | Offset to the raw texture data section                                                                                                                                                                                                                                                                             |
| u32     | offset_name_table         | Offset to an array of offsets into the binary section. The offsets point to null-terminated strings. The names are stored in the same order as the texture cells, so the same index can be used for both arrays. Used for debugging, and this value should be 0xFFFFFFFF if the table is not included in the file. |

All offsets are relative to the start of this binary section.

## TextureCellDesc
| Type | Name                  | Description                                           |
| ---- | --------------------- | ----------------------------------------------------- |
| u8   | sector_offset_texture | Offset (in bytes*2048) into raw texture data section. |
| u8   | palette_index         | Palette index.                                        |
| u8   | texture_width         | Texture width in pixels. If this is 0, treat as 256   |
| u8   | texture_height        | Texture height in pixels. If this is 0, treat as 256  |
| u32  | avg_color             | Average value of every pixel                          |