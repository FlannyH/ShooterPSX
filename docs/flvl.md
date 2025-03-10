# Flan's Level Metadata File Format

## Level file (.lvl)
| Type            | Name              | Description                                                                                  |
| --------------- | ----------------- | -------------------------------------------------------------------------------------------- |
| u32 | file_magic            | File magic: "FLVL"                                                                           |
| u32 | path_music_offset     | Offset to string containing the path to the music sequence file (.dss) to play in this level |
| u32 | path_bank_offset      | Offset to string containing the path to the soundbank file (.sbk) to use in this level       |
| u32 | path_texture_offset   | Offset to string containing the path to the level texture collection file (.txc) to load in this level    |
| u32 | path_collision_offset | Offset to string containing the path to the collision model file (.col) to load in this level    |
| u32 | path_vislist_offset   | Offset to string containing the path to the visibility list file (.col) to load in this level    |
| u32 | path_model_offset     | Offset to string containing the path to the level model file (.msh) to load in this level    |
| u32 | path_model_lod_offset | Optional offset to string containing the path to the lower detail level model file (.msh) to load in this level. Used for level sections further from the camera. Set to 0xFFFFFFFF if not using LODs. |
| u32 | entity_types_offset | Offset to the u8 array of entity types  |
| u32 | entity_pool_offset | Offset to the entity pool, which contains all the entity data, ready to be copied into RAM as is | 
| u32 | light_data_offset | Optional offset to an array of lights. Set to 0xFFFFFFFF if there are no lights | 
| u32 | level_name_offset     | Offset to string containing display name of the level as shown in game to the player         |
| u32 | text_offset     | Offset to text data for entities.         |
| u32 | n_text_entries     | How many text entries there are.         |
| i16[3] | player_spawn_position       | Where in the map the player spawns, in model space units                                     |
| i32[3] | player_spawn_rotation       | Player spawn rotation, 0x20000 = 360 degrees                                   |
| u16 | n_entities            | Number of predefined entities in this map                                                    |

All offsets are relative to the start of the binary section, which is located right after the header.

## Entity list

Entity data is stored in 2 arrays: an entity type array, and an entity data array. Entity data is stored as a serialized header, followed by its corresponding entity data, `n_entities` times.

### Entity type array entry
| Type   | Name     | Description                                                       |
| ------ | -------- | ----------------------------------------------------------------- |
| u8     | type     | Type of entity as defined in entity.h `entity_type_t`             |

### Serialized entity header array entry
| Type   | Name     | Description                                                       |
| ------ | -------- | ----------------------------------------------------------------- |
| i16[3] | position | Spawn position of the entity, in model space                      |
| i32[3] | rotation | Spawn rotation of the entity, 0x20000 = 360 degrees               |
| i32[3] | scale    | Spawn scale of the entity, 4096 = 1.0                             |

After this entry header, the raw entity data after the header as defined in `/source/entities/*.h` follows. Each entity is aligned to a grid where each grid cell is the size of the biggest entity struct

### Light array entry
| Type   | Name      | Description                                                       |
| ------ | --------- | ----------------------------------------------------------------- |
| i16[3] | direction_position | If this is a directional light, this is the direction vector, where the -32767 = -1.0 and +32767 = 1.0. If this is a point light, this is the position in model space |
| i16    | intensity | 8.8 fixed point number representing the brightness of the light   |
| u8[3]  | color     | 8-bit RGB values, which are then multiplied by the intensity when applying the light |
| u8     | type      | What type of light this is. 0 = directional light, 1 = point light | 
