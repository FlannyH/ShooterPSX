# Flan's Streaming File Archive specification

This file format contains any arbitrary files that may be used in a game.

An archive consists of a header, a single list containing items, followed by raw data used by the items in that list. An item is either a folder or a file. Each item has a name, as well as offsets to their contents. This list of items may be cached to RAM at runtime, which may allow files to be located on disc without any additional disc accesses.

The very first item in the list must be a folder named `root`. If the first item is not a folder named `root`, the archive is malformed and should be rejected. This name only exists to check the validity of the archive, and should not be considered part of the path.

## Header
| Type | Name | Description |
|------|------|-------------|
| char[4] | file_magic | File magic: "FSFA" |
| u32 | n_items | How many items does this file contain |

## Item
| Type | Name | Description |
|------|------|-------------|
| u8  | type | 0 = folder, 1 = file
| char[12] | name | The item's name. This string is null terminated if the length is below 12, and not null terminated if the length is equal to 12 |
| char[3] | extension | The item's file extension. This field is ignored for folders.
| u32 | offset | Where the item's data is located, relative to the start of the archive's header.
| u32 | size | Folders: how many items are stored in the folder. Files: file size in bytes

`name` and `extension` are used to traverse the file system and find specific files.

For files, `offset` points to the start of the file contents. For folders, `offset` points to an array of `u16` values, where each value is an index into the items array. This means the max number of items in a folder is 65535, which should be more than enough for most purposes.

For CD-based systems like the PS1, it may be worth aligning the start of the item's data to a multiple of the sector size in bytes (e.g. 2048 bytes) if the item is big enough, like with big files. For smaller files you might want to pack multiple together into one sector.

If an item's name is shorter than 12 characters, or the extension is shorter than 3 characters, its string should be null terminated. Any characters after the null terminator will be ignored. An example of a file name and extension could be `"document\0"`, `"txt"`.

If an item's name is exactly 12 characters, or the extension is exactly 3 characters, its string will not be null terminated. The program parsing the archive must make sure to only read up to 12 characters for the name, and up to 3 characters for the extension.

Handling of names or extensions that exceed the size limits is left to the archive creation tool. As long as the name fits in the 12-byte array and the extension in the 3-byte array, they are valid.

## Example
Let's imagine a simple example archive with 2 files in the root folder: a file named "text.txt" containing the text "hello world!", and a file named "Example Text.txt" containing "example".

Header (offset 0x00):
- file_magic = "FSFA"
- n_items = 3

Item list (offset 0x08):
- 0: (root folder)
  - type = 0 (folder)
  - name = "root\0"
  - extension = "\0" (empty null terminated string)
  - offset = 0x80 
  - size = 2 (items)
- 1: "text.txt"
  - type = 1 (file)
  - name = "text\0" 
  - extension = "txt" (without null terminator)
  - offset = 0x84
  - size = 12 (bytes)
- 2: "Example Text.txt"
  - type = 1 (file)
  - name = "Example Text" (without null terminator)
  - extension = "txt" (without null terminator)
  - offset = 0x90
  - size = 12 (bytes)

Data (offset 0x80):
- folder 0 data (0x80): `u16[] = {1, 2}`
- file 1 data (0x84): `char[] = "hello world!"` (without null terminator)
- file 2 data (0x90): `char[] = "example"` (without null terminator)
