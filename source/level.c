#include "level.h"

#include "renderer.h"
#include "texture.h"
#include "music.h"
#include "file.h"

#include <entity.h>
#include <string.h>

level_t level_load(const char* level_path) {
#ifdef _PSX
    // Wait until done rendering (it uses the temporary stack), then clear the memory stacks
    DrawSync(0);
#endif

    mem_stack_release(STACK_TEMP);
    mem_stack_release(STACK_LEVEL);
    mem_stack_release(STACK_ENTITY);
    entity_init();

    // Read the file 
    uint32_t* file_data = NULL;
    size_t size = 0;
    file_read(level_path, &file_data, &size, 1, STACK_TEMP);

    if (!file_data) {
        return (level_t) { 0 };
    }

    // Get header data
    const level_header_t* level_header = (level_header_t*)file_data;

    // Ensure FMSH header is valid
    if (level_header && (level_header->file_magic != MAGIC_FLVL)) { // "FLVL"
        printf("[ERROR] Error loading level '%s', file header is invalid!\n", level_path);
        return (level_t) { 0 };
    }

    // Find the data sections
    const uintptr_t binary_section = (const uintptr_t)&level_header[1];
    const char* path_music = (const char*)((binary_section + level_header->path_music_offset));
    const char* path_bank = (const char*)((binary_section + level_header->path_bank_offset));
    const char* path_textures = (const char*)((binary_section + level_header->path_texture_offset));
    const char* path_collision = (const char*)((binary_section + level_header->path_collision_offset));
    const char* path_vislist = (const char*)((binary_section + level_header->path_vislist_offset));
    const char* path_graphics = (const char*)((binary_section + level_header->path_model_offset));
    const char* level_entity_pool = (const char*)((binary_section + level_header->entity_pool_offset));
    const uint8_t* level_entity_types = (const uint8_t*)((binary_section + level_header->entity_types_offset));
    const light_t* lights = (const light_t*)((binary_section + level_header->light_data_offset));
    const char* text = (const char*)((binary_section + level_header->text_offset));

    // We gotta do some specific memory management if we want to fit as much into the temporary stack as we can
    const size_t marker = mem_stack_get_marker(STACK_TEMP);

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
    level.graphics = model_load(path_graphics, 1, STACK_LEVEL, tex_level_start, 1);
    mem_stack_reset_to_marker(STACK_TEMP, marker);

    // Load entities
    const intptr_t level_entity_pool_stride = entity_get_pool_stride() - sizeof(entity_header_t) + sizeof(entity_header_serialized_t);
    
    // Deserialize entity data
    for (int i = 0; i < level_header->n_entities; ++i) {
        // Find where data needs to be read
        const entity_header_serialized_t* src_entity_header = (const entity_header_serialized_t*)(level_entity_pool + (i * level_entity_pool_stride));
        entity_deserialize_and_write_slot(i, src_entity_header);
    }
    entity_sanitize();

    const int n_entities = level_header->n_entities;
    for (int i = 0; i < n_entities; ++i) {
        entity_set_type(i, level_entity_types[i]);
    }

    // Load text data
    level.n_text_entries = 0;
    if (level_header->text_offset && level_header->n_text_entries > 0) {
        level.n_text_entries = level_header->n_text_entries;
#ifdef _LEVEL_EDITOR
        level.text_entries = malloc(level.n_text_entries * sizeof(char**));
#else
        level.text_entries = mem_stack_alloc(level.n_text_entries * sizeof(char**), STACK_LEVEL);
#endif

        for (int i = 0; i < (int)level_header->n_text_entries; ++i) {
            const uint8_t n_chars = *(uint8_t*)(text++);
#ifdef _LEVEL_EDITOR
            level.text_entries[i] = malloc(255);
#else
            level.text_entries[i] = mem_stack_alloc(n_chars + 1, STACK_LEVEL);
#endif
            level.text_entries[i][n_chars] = 0;
            for (int j = 0; j < n_chars; ++j) {
                level.text_entries[i][j] = *text++;
            }
        }
    }

    // Load lights
    level.n_lights = level_header->n_lights;
#ifdef _LEVEL_EDITOR
    level.lights = malloc(MAX_LIGHT_COUNT * sizeof(light_t));
    memset(level.lights, 0, MAX_LIGHT_COUNT * sizeof(light_t));
#else
    level.lights = mem_stack_alloc(level.n_lights * sizeof(light_t), STACK_LEVEL);
#endif
    memcpy(level.lights, lights, level.n_lights * sizeof(light_t));

    // Start new music and load sfx data
    music_stop();
    mem_stack_release(STACK_MUSIC);
    if (path_music[0] != 0 && path_bank[0] != 0) {
        audio_load_soundbank(path_bank, SOUNDBANK_TYPE_MUSIC); mem_stack_reset_to_marker(STACK_TEMP, marker);
        audio_load_soundbank("audio/sfx.sbk", SOUNDBANK_TYPE_SFX); mem_stack_reset_to_marker(STACK_TEMP, marker);
        music_load_sequence(path_music);
        music_play_sequence(0);
        music_set_volume(255);
    }
    mem_stack_release(STACK_TEMP);

    return level;
}
