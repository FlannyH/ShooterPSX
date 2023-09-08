# Flan's Level Metadata File Format

## Level file (.lvl)
| Type    | Name                      | Description                                                                               |
| ------- | ------------------------- | ----------------------------------------------------------------------------------------- |
| char[4] | file_magic                | File magic: "FLVL"                                                                        |
| u32     | music_name_offset | Offset to string containing the path to the music sequence file (.dss) to play in this level
| u32     | bank_name_offset | Offset to string containing the path to the soundbank file (.sbk) to play in this level
| u32     | model_name_offset | Offset to string containing the path to the level model file (.msh) to play in this level
| u32     | level_name_offset | Offset to string containing display name of the level as shown in game to the player
| i16[3]  | player_spawn              | Where in the map the player spawns, in model space units                |
| u16 | n_entities | Number of predefined entities in this map
| u32[n_entities] | entity_offsets | List of offsets to all the enemy data.

## Entity list entry
| Type | Name | Description |
|--|--|--|
|u16|type|Type of entity. Determines the size of the struct that follows it
|i16[3]|position|Spawn position of the entity, in model space |
|i32[3]|rotation|Spawn rotation of the entity, 0x20000 = 360 degrees
After this entry header, the raw entity data after the header as defined in `/source/entities/*.h` follows.