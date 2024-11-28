# Flan's Streaming File Archive specification

This file format contains any arbitrary files that may be used in a game, designed specifically so we only need one disc read to figure out where each file is.

The idea is to have a single list containing items. An item is either a folder or a file. Each item has a name, as well as offsets to their contents. This list of items may be cached to RAM at runtime, which allows files to be searched.

The very first item in the list must be a folder with name `root`. If the first item's name is not `root`, the file is invalid and should be rejected.

## Header
| Type | Name | Description |
|------|------|-------------|
| char[4] | file_magic | File magic: "FSFA" |
| u32 | n_items | How many items does this file contain |

## Item
| Type | Name | Description |
|------|------|-------------|
| u8  | type | 0 = folder, 1 = file
| char[12] | name | The item's name. This is used to search for a file |
| char[3] | extension | The item's extension. This field is ignored for folders.
| u32 | offset | Where the item's data is located, relative to the start of the archive's header.
| u32 | size | Folders: how many items are stored in the folder. Files: file size in bytes

For CD-based systems like the PS1, it may be worth aligning the start of the item's data to a multiple of the sector size in bytes (e.g. 2048 bytes). This way files are better aligned for streaming.

If an item's name is shorter than 12 characters, or the extension is shorter than 3 characters, there should be at least one null character right after the name to denote the end of the file name. Any characters after the null terminator will be ignored. An example of a file name and extension could be `"document\0"`, `"txt"`

If an item's name is exactly 12 characters, or the extension is exactly 3 characters, the string will not be null terminated. The program parsing the archive must stop reading after 12 characters for the name, and it must stop after 3 characters for the extension.

Handling of names or extensions that exceed the size limits is left to the archive creation tool. As long as the name fits in the 12-byte array and the extension in the 3-byte array, they are valid.
