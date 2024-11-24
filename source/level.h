#ifndef LEVEL_H
#define LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "structs.h"
#include "vislist.h"
#include "vec3.h"

#include <stdint.h>

// Level file header
#define MAGIC_FLVL 0x4C564C46
typedef struct {
    uint32_t file_magic;            // File magic: "FLVL"
    uint32_t path_music_offset;     // Offset to string containing the path to the music sequence file (.dss) to play in this level
    uint32_t path_bank_offset;      // Offset to string containing the path to the soundbank file (.sbk) to use in this level
    uint32_t path_texture_offset;   // Offset to string containing the path to the level texture collection file (.txc) to load in this level
    uint32_t path_collision_offset; // Offset to string containing the path to the collision model file (.col) to load in this level
    uint32_t path_vislist_offset;   // Offset to string containing the path to the vislist file (.vis) to load in this level
    uint32_t path_model_offset;     // Offset to string containing the path to the level model file (.msh) to load in this level
    uint32_t path_model_lod_offset; // Offset to string containing the path to the level model file (.msh) to load in this level. Set to 0 if not using LODs.
    uint32_t entity_types_offset;   // Offset to entity_types data, which gets copied into `entity_types` directly
    uint32_t entity_pool_offset;    // Offset to entity_pool data, which gets copied into `entity_pool` directly
    uint32_t level_name_offset;     // Offset to string containing display name of the level as shown in game to the player
    uint32_t text_offset;           // Offset to text data for entities
    uint32_t n_text_entries;        // How many text entries there are
    svec3_t player_spawn_position;  // Where in the map the player spawns, in model space units
    vec3_t player_spawn_rotation;   // Player spawn rotation, 0x20000 = 360 degrees
    uint16_t n_entities;            // Number of predefined entities in this map
} level_header_t;

typedef struct {
    model_t* graphics;
    model_t* collision_mesh_debug;
    collision_mesh_t* collision_mesh;
    transform_t transform;
    vislist_t vislist;
    level_collision_t collision_bvh;
    svec3_t player_spawn_position;
    vec3_t player_spawn_rotation;
    char** text_entries;
    int n_text_entries;
    int n_level_textures;
} level_t;

level_t level_load(const char* level_path); // Load level from path, including .LVL;1 extension.


#ifdef __cplusplus
}
#endif
#endif
