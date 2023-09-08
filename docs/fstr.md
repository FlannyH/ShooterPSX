# Flan's String Table Format

## Level file (.lvl)
| Type    | Name                      | Description                                                                               |
| ------- | ------------------------- | ----------------------------------------------------------------------------------------- |
| char[4] | file_magic                | File magic: "FSTR"                                                                        |
| u32| n_strings| Number of text strings in the file
|string_table_entry_t[n_strings]|entries| A table that contains the offsets to the string name and string data
After this header, raw binary data follows with the string names and values

## String table entry
|Type | Name| Description|
|--|--|--|
|u32|name_offset| Offset in binary section to the internal name of the string|
|u32|value_offset| Offset in binary section to the actual string data this name corresponds to|