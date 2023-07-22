# Flan Collision Model
## File header
| Type    | Name       | Description                                   |
| ------- | ---------- | --------------------------------------------- |
| char[4] | file_magic | File magic: "FVIS"                            |
| u32     | n_sections | The number of section in this visibility list | 

## Section Visibility List
| Type | Name             | Description                                                                           |
| ---- | ---------------- | ------------------------------------------------------------------------------------- |
| u128 | visible_sections | Bitfield of 128 sections, where 0 means not visible, and 1 means visible. Low Endian. |