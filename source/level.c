#include "level.h"
#include "texture.h"
#include "renderer.h"

level_t level_load(const char* path) {
    // Read the file 
    uint32_t* file_data;
    size_t size;
    mem_stack_release(STACK_TEMP);
    file_read(path, &file_data, &size, 1, STACK_TEMP);

    // Get header data
    level_header_t* level_header = (level_header_t*)file_data;

    // Ensure FMSH header is valid
    if (level_header->file_magic != MAGIC_FLVL) { // "FLVL"
        printf("[ERROR] Error loading level '%s', file header is invalid!\n", path);
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

    // Load level textures
	texture_cpu_t *tex_level;
    tex_level_start = 0;
	const uint32_t n_level_textures = texture_collection_load(path_textures, &tex_level, 1, STACK_TEMP);
    for (uint8_t i = 0; i < n_level_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i + tex_level_start);
	}

    // Load entity textures
	texture_cpu_t *entity_textures;
    tex_entity_start = tex_level_start + n_level_textures;
	const uint32_t n_entity_textures = texture_collection_load("\\ASSETS\\MODELS\\ENTITY.TXC;1", &entity_textures, 1, STACK_TEMP);
	for (uint8_t i = 0; i < n_entity_textures; ++i) {
	    renderer_upload_texture(&entity_textures[i], i + tex_entity_start);
	}

    level_t level = (level_t) {
        .graphics = model_load(path_graphics, 1, STACK_LEVEL),
        .collision_mesh_debug = model_load_collision_debug(path_collision, 1, STACK_LEVEL),
        .collision_mesh = model_load_collision(path_collision, 1, STACK_LEVEL),
        .transform = (transform_t){{0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096}},
        .vislist = vislist_load(path_vislist, 1, STACK_LEVEL),
    };
    bvh_from_model(&level.collision_bvh, level.collision_mesh);

    return level;
}