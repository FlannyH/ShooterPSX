#include "level.h"

#include <entity.h>
#include <string.h>

#include "renderer.h"
#include "texture.h"
#include "music.h"
#include "file.h"

extern uint8_t entity_types[ENTITY_LIST_LENGTH];
extern uint8_t* entity_pool;

level_t level_load(const char* level_path) {
    // Read the file 
    uint32_t* file_data = NULL;
    size_t size = 0;
    mem_stack_release(STACK_TEMP);
    mem_stack_release(STACK_LEVEL);
    mem_stack_release(STACK_ENTITY);
    entity_init();

    file_read(level_path, &file_data, &size, 1, STACK_TEMP);

    // Get header data
    level_header_t* level_header = (level_header_t*)file_data;

    // Ensure FMSH header is valid
    if (level_header && (level_header->file_magic != MAGIC_FLVL)) { // "FLVL"
        printf("[ERROR] Error loading level '%s', file header is invalid!\n", level_path);
        return (level_t) { 0 };
    }

    // Find the data sections
    const uintptr_t binary_section = (const uintptr_t)&level_header[1];
    const void* entity_data = (const void*)(binary_section);
    const char* path_music = (const char*)((binary_section + level_header->path_music_offset));
    const char* path_bank = (const char*)((binary_section + level_header->path_bank_offset));
    const char* path_textures = (const char*)((binary_section + level_header->path_texture_offset));
    const char* path_collision = (const char*)((binary_section + level_header->path_collision_offset));
    const char* path_vislist = (const char*)((binary_section + level_header->path_vislist_offset));
    const char* path_graphics = (const char*)((binary_section + level_header->path_model_offset));
    const char* path_graphics_lod = (const char*)((binary_section + level_header->path_model_lod_offset));
    const char* level_entity_pool = (const char*)((binary_section + level_header->entity_pool_offset));
    const char* level_entity_types = (const char*)((binary_section + level_header->entity_types_offset));

    // We gotta do some specific memory management if we want to fit as much into the temporary stack as we can
    size_t marker = mem_stack_get_marker(STACK_TEMP);

    // Load level textures
	texture_cpu_t *tex_level;
    tex_level_start = tex_alloc_cursor;
	const uint32_t n_level_textures = texture_collection_load(path_textures, &tex_level, 1, STACK_TEMP);
    for (uint8_t i = 0; i < n_level_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i + tex_level_start);
	}
	tex_alloc_cursor += n_level_textures;
    mem_stack_reset_to_marker(STACK_TEMP, marker);

    level_t level = (level_t) {
#ifdef _LEVEL_EDITOR
        .collision_mesh_debug = model_load_collision_debug(path_collision, 0, 0),
#else
        .collision_mesh_debug = NULL,
#endif
        .transform = (transform_t){{0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096}},
        .vislist = vislist_load(path_vislist, 1, STACK_LEVEL),
        .collision_bvh = bvh_from_file(path_collision, 1, STACK_LEVEL),
        .n_level_textures = n_level_textures,
        .player_spawn_position = level_header->player_spawn_position,
        .player_spawn_rotation = level_header->player_spawn_rotation,
    };
    mem_stack_reset_to_marker(STACK_TEMP, marker);
    level.graphics = model_load(path_graphics, 1, STACK_LEVEL, tex_level_start);
    mem_stack_reset_to_marker(STACK_TEMP, marker);
    level.collision_mesh = model_load_collision(path_collision, 1, STACK_LEVEL);
    mem_stack_reset_to_marker(STACK_TEMP, marker);

    // Load entities
    intptr_t level_entity_pool_stride = entity_pool_stride - sizeof(entity_header_t) + sizeof(entity_header_serialized_t);
    
    // Deserialize entity data
    for (int i = 0; i < level_header->n_entities; ++i) {
        // Find where data needs to be read
        entity_header_serialized_t* src_entity_header = (entity_header_serialized_t*)(level_entity_pool + (i * level_entity_pool_stride));
        uint8_t* src_entity_data = (uint8_t*)&src_entity_header[1]; // right after the entity header

        // Find where data needs to be written
        entity_header_t* dst_entity_header = (entity_header_t*)(entity_pool + (i * entity_pool_stride));
        uint8_t* dst_entity_data = (uint8_t*)&dst_entity_header[1]; // right after the entity header
        
        // Write entity header
        dst_entity_header->position = src_entity_header->position;
        dst_entity_header->rotation = src_entity_header->rotation;
        dst_entity_header->scale = src_entity_header->scale;
        dst_entity_header->mesh = NULL;

        // Copy entity data - we just need to copy everything after the header, so subtract the header size
        memcpy(dst_entity_data, src_entity_data, entity_pool_stride - sizeof(entity_header_t));
    }
    entity_sanitize();

    memcpy(entity_types, level_entity_types, level_header->n_entities);

    // Start new music
    music_stop();
    mem_stack_release(STACK_MUSIC);
    if (path_music[0] != 0 && path_bank[0] != 0) {
        music_load_soundbank(path_bank);
        music_load_sequence(path_music);
        music_play_sequence(0);
        music_set_volume(255);
    }
    mem_stack_release(STACK_TEMP);

    return level;
}