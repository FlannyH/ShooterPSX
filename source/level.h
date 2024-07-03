#ifndef LEVEL_H
#define LEVEL_H
#include <stdint.h>
#include "structs.h"

// Level file header
#define MAGIC_FTXC 0x43585446
typedef struct {
    uint32_t file_magic;            // File magic: "FLVL"                                                                          
    uint32_t music_path_offset;     // Offset to string containing the path to the music sequence file (.dss) to play in this level
    uint32_t bank_path_offset;      // Offset to string containing the path to the soundbank file (.sbk) to use in this level      
    uint32_t model_path_offset;     // Offset to string containing the path to the level model file (.msh) to load in this level   
    uint32_t model_lod_path_offset; // Optional offset to string containing the path to the level model file (.msh) to load in this level. Set to 0 if not using LODs.
    uint32_t level_name_offset;     // Offset to string containing display name of the level as shown in game to the player        
    svec3_t player_spawn;           // Where in the map the player spawns, in model space units                                    
    uint16_t n_entities;            // Number of predefined entities in this map                                                   
} level_header_t;                                            

#endif