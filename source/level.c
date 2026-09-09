#include "level.h"

#include "memory.h"
#include "renderer.h"
#include "texture.h"
#include "music.h"
#include "mesh.h"
#include "file.h"

#include <assert.h>
#include <entity.h>
#include <string.h>

void serialize_shape(uint8_t* shapes, size_t* cursor, const shape_t* shape) {
    // align to word
    while (*cursor % 4 != 0) {
        (*cursor)++;
    }

    void* where_to = &shapes[*cursor];

    switch (shape->type) {
    case SHAPE_NONE:
        *(uint32_t*)(&shapes[*cursor]) = SHAPE_NONE;                  *cursor += sizeof(uint32_t);
        break;
    case SHAPE_SPHERE:
        *(uint32_t*)(&shapes[*cursor]) = SHAPE_SPHERE;                *cursor += sizeof(uint32_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->sphere.center;          *cursor += sizeof(vec3_t);
        *(scalar_t*)(&shapes[*cursor]) = shape->sphere.radius;        *cursor += sizeof(scalar_t);
        break;
    case SHAPE_CAPSULE:
        *(uint32_t*)(&shapes[*cursor]) = SHAPE_CAPSULE;               *cursor += sizeof(uint32_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->capsule.a;              *cursor += sizeof(vec3_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->capsule.b;              *cursor += sizeof(vec3_t);
        *(scalar_t*)(&shapes[*cursor]) = shape->capsule.radius;       *cursor += sizeof(scalar_t);
        break;
    case SHAPE_TRIANGLE:
        *(uint32_t*)(&shapes[*cursor]) = SHAPE_TRIANGLE;              *cursor += sizeof(uint32_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->triangle.v0;            *cursor += sizeof(vec3_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->triangle.v1;            *cursor += sizeof(vec3_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->triangle.v2;            *cursor += sizeof(vec3_t);
        break;
    case SHAPE_AABB:
        *(uint32_t*)(&shapes[*cursor]) = SHAPE_AABB;                  *cursor += sizeof(uint32_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->aabb.min;               *cursor += sizeof(vec3_t);
        *(vec3_t*)(&shapes[*cursor]) = shape->aabb.max;               *cursor += sizeof(vec3_t);
        break;
    case SHAPE_CONVEX_HULL:
        *(uint32_t*)(&shapes[*cursor]) = SHAPE_CONVEX_HULL;           *cursor += sizeof(uint32_t);
        *(uint32_t*)(&shapes[*cursor]) = shape->convex_hull.n_points; *cursor += sizeof(uint32_t);
        for (size_t i = 0; i < shape->convex_hull.n_points; ++i) {
            *(vec3_t*)(&shapes[*cursor]) = shape->convex_hull.points[i];
            *cursor += sizeof(vec3_t);
        }
        break;
    }
}

void deserialize_shape(const uint8_t* data, size_t* offset, shape_t* shape) {
    assert(shape != NULL);
    assert((*offset % 4) == 0);

    const uint32_t type = *(uint32_t*)(&data[*offset]);                *offset += sizeof(uint32_t);
    shape->type = (uint8_t)type;

    switch(type) {
    case SHAPE_NONE:
        break;
    case SHAPE_SPHERE:
        shape->sphere.center = *(vec3_t*)(&data[*offset]);             *offset += sizeof(vec3_t);
        shape->sphere.radius = *(scalar_t*)(&data[*offset]);           *offset += sizeof(scalar_t);
        break;
    case SHAPE_CAPSULE:
        shape->capsule.a = *(vec3_t*)(&data[*offset]);                 *offset += sizeof(vec3_t);
        shape->capsule.b = *(vec3_t*)(&data[*offset]);                 *offset += sizeof(vec3_t);
        shape->capsule.radius = *(scalar_t*)(&data[*offset]);          *offset += sizeof(scalar_t);
        break;
    case SHAPE_TRIANGLE:
        shape->triangle.v0 = *(vec3_t*)(&data[*offset]);               *offset += sizeof(vec3_t);
        shape->triangle.v1 = *(vec3_t*)(&data[*offset]);               *offset += sizeof(vec3_t);
        shape->triangle.v2 = *(vec3_t*)(&data[*offset]);               *offset += sizeof(vec3_t);
        break;
    case SHAPE_AABB:
        shape->aabb.min = *(vec3_t*)(&data[*offset]);                  *offset += sizeof(vec3_t);
        shape->aabb.max = *(vec3_t*)(&data[*offset]);                  *offset += sizeof(vec3_t);
        break;
    case SHAPE_CONVEX_HULL:
        shape->convex_hull.n_points = *(uint32_t*)(&data[*offset]);    *offset += sizeof(uint32_t);
#ifdef _LEVEL_EDITOR
        shape->convex_hull.points = mem_stack_alloc(shape->convex_hull.n_points * sizeof(vec3_t), STACK_LEVEL);
#else
        shape->convex_hull.points = mem_stack_alloc(shape->convex_hull.n_points * sizeof(vec3_t), STACK_LEVEL);
#endif
        for (size_t i = 0; i < shape->convex_hull.n_points; ++i) {
            shape->convex_hull.points[i] = *(vec3_t*)(&data[*offset]); *offset += sizeof(vec3_t);
        }
        break;
    }
}

level_t level_load(const char* level_path, const uint32_t flags) {
#ifdef _PSX
    // Wait until done rendering (it uses the temporary stack), then clear the memory stacks
    DrawSync(0);
#endif

    mem_stack_release(STACK_TEMP);
    mem_stack_release(STACK_LEVEL);

	renderer_free_texture_category(TEX_CAT_LEVEL);

    // Read the file
    uint32_t* file_data = NULL;
    size_t size = 0;
    file_read(level_path, &file_data, &size, 1, STACK_TEMP);

    if (!file_data) {
        printf("[ERROR] Error loading level '%s', file could not be read!\n", level_path);
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
    const uint8_t* shapes = (const uint8_t*)((binary_section + level_header->shape_data_offset));
    const char* text = (const char*)((binary_section + level_header->text_offset));

    // We gotta do some specific memory management if we want to fit as much into the temporary stack as we can
    const size_t marker = mem_stack_get_marker(STACK_TEMP);
    uint32_t n_level_textures = 0;

    if (flags & LEVEL_LOAD_TEXTURES) {
        // Load level textures
        texture_cpu_t *tex_level;
        n_level_textures = texture_collection_load(path_textures, &tex_level, 1, STACK_TEMP);
        for (uint8_t i = 0; i < n_level_textures; ++i) {
            renderer_upload_texture(&tex_level[i], i, TEX_CAT_LEVEL);
        }
        mem_stack_reset_to_marker(STACK_TEMP, marker);
    }

    level_t level = (level_t) {
#ifdef _LEVEL_EDITOR
        .collision_mesh_debug = model_load_collision_debug(path_collision, 0, 0),
#else
        .collision_mesh_debug = NULL,
#endif
        .transform = (transform_t){{0, 0, 0}, {0, 0, 0}, {ONE, ONE, ONE}},
        .vislist = vislist_load(path_vislist, 1, STACK_LEVEL),
        // .collision_bvh = bvh_from_file(path_collision, 1, STACK_LEVEL),
        .collision_bvh = NULL, // todo
        .n_level_textures = n_level_textures,
        .player_spawn_position = level_header->player_spawn_position,
        .player_spawn_rotation = level_header->player_spawn_rotation,
    };
    mem_stack_reset_to_marker(STACK_TEMP, marker);

    level.graphics = model_load(path_graphics, 1, STACK_LEVEL, TEX_CAT_LEVEL, 1);
    mem_stack_reset_to_marker(STACK_TEMP, marker);

    if (flags & LEVEL_LOAD_COLLISION) {
        // Load shapes
        level.n_shapes = level_header->n_shapes;
#ifdef _LEVEL_EDITOR
        level.shapes = malloc(MAX_SHAPE_COUNT * sizeof(shape_t));
        memset(level.shapes, 0, MAX_SHAPE_COUNT * sizeof(shape_t));
#else
        level.shapes = mem_stack_alloc(level.n_shapes * sizeof(shape_t), STACK_LEVEL);
#endif
        size_t offset = 0;
        for (size_t i = 0; i < level.n_shapes; ++i) {
            deserialize_shape(shapes, &offset, &level.shapes[i]);
        }
    }

    if (flags & LEVEL_LOAD_ENTITIES) {
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
    }

    if (flags & LEVEL_LOAD_TEXT) {
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
    }

    if (flags & LEVEL_LOAD_LIGHTS) {
        // Load lights
        level.n_lights = level_header->n_lights;
#ifdef _LEVEL_EDITOR
        level.lights = malloc(MAX_LIGHT_COUNT * sizeof(light_t));
        memset(level.lights, 0, MAX_LIGHT_COUNT * sizeof(light_t));
#else
        level.lights = mem_stack_alloc(level.n_lights * sizeof(light_t), STACK_LEVEL);
#endif
        memcpy(level.lights, lights, level.n_lights * sizeof(light_t));
    }

    if (flags & LEVEL_LOAD_MUSIC) {
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
    }
    mem_stack_release(STACK_TEMP);

    return level;
}
