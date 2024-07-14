# Flan's Level Metadata File Format

## Level file (.lvl)
| Type            | Name              | Description                                                                                  |
| --------------- | ----------------- | -------------------------------------------------------------------------------------------- |
| u32 | file_magic            | File magic: "FLVL"                                                                           |
| u32 | path_music_offset     | Offset to string containing the path to the music sequence file (.dss) to play in this level |
| u32 | path_bank_offset      | Offset to string containing the path to the soundbank file (.sbk) to use in this level       |
| u32 | path_texture_offset   | Offset to string containing the path to the level texture collection file (.txc) to load in this level    |
| u32 | path_collision_offset | Offset to string containing the path to the collision model file (.col) to load in this level    |
| u32 | path_vislist_offset   | Offset to string containing the path to the collision model file (.col) to load in this level    |
| u32 | path_model_offset     | Offset to string containing the path to the level model file (.msh) to load in this level    |
| u32 | path_model_lod_offset | Optional offset to string containing the path to the level model file (.msh) to load in this level. Set to 0 if not using LODs. |
| u32 | entity_types_offset | Offset to the u8 array of entity types  |
| u32 | entity_pool_offset | Offset to the entity pool, which contains all the entity data, ready to be copied into RAM as is | 
| u32 | level_name_offset     | Offset to string containing display name of the level as shown in game to the player         |
| i16[3] | player_spawn       | Where in the map the player spawns, in model space units                                     |
| u16 | n_entities            | Number of predefined entities in this map                                                    |

All offsets are relative to the start of the binary section, which is located right after the header.

## Entity list entry
| Type   | Name     | Description                                                       |
| ------ | -------- | ----------------------------------------------------------------- |
| u16    | type     | Type of entity. Determines the size of the struct that follows it |
| i16[3] | position | Spawn position of the entity, in model space                      |
| i32[3] | rotation | Spawn rotation of the entity, 0x20000 = 360 degrees               |

After this entry header, the raw entity data after the header as defined in `/source/entities/*.h` follows. Each entity is aligned to a grid where each grid cell is the size of the biggest entity struct