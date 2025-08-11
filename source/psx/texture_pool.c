#include <stdio.h>
#include <psxgpu.h>

#include "texture_pool.h"
#include "../memory.h"

#define MAX_RES_SHIFTS 8
#define MAX_RES (1<<MAX_RES_SHIFTS)
#define MAX_TEXTURE_POOL_COUNT 8
#define MAX_TEXTURE_COUNT 128

typedef struct {
    uint16_t top, left;
    uint32_t resolution;
    uint32_t** occupancy_maps;
    RECT* textures;
    uint32_t n_textures;
} texture_pool_t;

static texture_pool_t texture_pools[MAX_TEXTURE_POOL_COUNT] = {0};

#define GET_BIT_INT(v, bit) ((v) & (1 << (bit)))
#define GET_BIT_ARR(a, bit) (( ((a)[((bit) >> 5)]) >> ((bit) & 31) ) & 1)
#define SET_BIT_ARR(a, bit) ((a)[((bit) >> 5)]) |= (1 << ((bit) & 31));

void texture_pool_init(uint32_t pool_index, uint16_t left, uint16_t top, uint32_t resolution) {
    if (resolution > MAX_RES) {
        printf("[ERROR] Texture pool too big (%ix%i), should be max (%ix%i)\n", resolution, resolution, MAX_RES, MAX_RES);
        return;
    }
    
    if (pool_index > MAX_TEXTURE_POOL_COUNT) {
        printf("[ERROR] Texture pool index (%i) out of bounds, should be between 0 and %i\n", pool_index, MAX_TEXTURE_POOL_COUNT - 1);
        return;
    }

    texture_pools[pool_index].top = top;
    texture_pools[pool_index].left = left;
    texture_pools[pool_index].resolution = resolution;
    texture_pools[pool_index].occupancy_maps = mem_stack_alloc(sizeof(uint32_t*) * MAX_RES_SHIFTS, STACK_PERSISTENT);
    texture_pools[pool_index].textures = mem_stack_alloc(sizeof(RECT) * 128, STACK_PERSISTENT);

    size_t level_count = 0;
    for (uint32_t r = resolution; r > 0; r >>= 1) {
        // allocate occupancy map
        const uint32_t n_bits = r * r;
        const uint32_t bits_per_unit = sizeof(uint32_t) * 8;
        const uint32_t alloc_size_ceil = (n_bits + bits_per_unit - 1) / 8;
        uint32_t* data = mem_stack_alloc(alloc_size_ceil, STACK_PERSISTENT);
        texture_pools[pool_index].occupancy_maps[level_count++] = data;

        // clear the memory to 0 (free block)
        for (size_t i = 0; i < alloc_size_ceil / sizeof(uint32_t); ++i) data[i] = 0;
    }
}

int texture_pool_alloc(uint32_t pool_index, int texture_id, uint32_t width, uint32_t height) {
    // find occupancy map
    uint32_t curr_res = texture_pools[pool_index].resolution;
    uint32_t block_size = 1;
    uint32_t occupancy_map_index = 0;
    while (block_size < width && block_size < height) {
        curr_res /= 2;
        block_size *= 2;
        ++occupancy_map_index;
    }
    uint32_t* occ_map = texture_pools[pool_index].occupancy_maps[occupancy_map_index];

    // what shape do we need to check
    uint32_t alloc_width = (width + block_size - 1) / block_size;
    uint32_t alloc_height = (height + block_size - 1) / block_size;

    // todo: consider optimizing this
    // find a spot - lovely nested for loop to fit the shape in there somewhere
    // for every block in this map layer
    uint32_t occ_top = 0;
    uint32_t occ_left = 0;
    for (uint32_t dst_top = 0; dst_top < curr_res; ++dst_top) {
        for (uint32_t dst_left = 0; dst_left < curr_res; ++dst_left) {
            // for every block in the texture to allocate
            int collisions = 0;
            for (uint32_t offset_y = 0; offset_y < alloc_height; ++offset_y) {
                for (uint32_t offset_x = 0; offset_x < alloc_width; ++offset_x) {
                    // find this occupancy map layer's coordinates
                    const uint32_t x = dst_left + offset_x;
                    const uint32_t y = dst_top + offset_y;

                    // out of bounds counts as collision
                    if (x >= curr_res || y >= curr_res) {
                        ++collisions;
                        continue;
                    }

                    // so does an occupied block
                    const uint32_t index = (y * curr_res) + x;
                    const uint32_t bit_set = GET_BIT_ARR(occ_map, index);
                    if (bit_set) ++collisions;
                }
            }
            
            if (collisions == 0) {
                occ_top = dst_top;
                occ_left = dst_left;
                goto done;
            }
        }
    }

    // if we get here, there's no space for this texture in this pool
    printf("[ERROR] No free space for %ix%i texture in pool %i, not allocating\n", width, height, pool_index);
    return -1;
    
    done:
    // update occupancy maps
    uint32_t top = occ_top * block_size;
    uint32_t left = occ_left * block_size;
    uint32_t start_x = left;
    uint32_t start_y = top;
    uint32_t end_x = start_x + width - 1;
    uint32_t end_y = start_y + height - 1;
    size_t i = 0;
    for (uint32_t r = texture_pools[pool_index].resolution; r > 0; r >>= 1) {
        uint32_t* curr_occ_map = texture_pools[pool_index].occupancy_maps[i];

        for (uint32_t y = start_y; y <= end_y; ++y) {
            for (uint32_t x = start_x; x <= end_x; ++x) {
                SET_BIT_ARR(curr_occ_map, (y * r) + x);
            }
        }
        start_x >>= 1;
        start_y >>= 1;
        end_x >>= 1;
        end_y >>= 1;
        ++i;
    }
#ifdef _DEBUG_VERBOSE
    printf("for pool index %i, allocated texture %i (%ix%i) at location (%i, %i)\n", pool_index, texture_pools[pool_index].n_textures, width, height, left, top);
#endif
    texture_pools[pool_index].textures[texture_id] = (RECT){
        .x = (int16_t)left,
        .y = (int16_t)top,
        .w = (int16_t)width,
        .h = (int16_t)height,
    };
    return texture_id;
}

RECT texture_pool_rect(uint32_t pool_index, int texture_id) {
#ifdef _DEBUG
    assert(pool_index < MAX_TEXTURE_POOL_COUNT);
    assert(texture_id < MAX_TEXTURE_COUNT);
#endif
    return (RECT) {
        .x = texture_pools[pool_index].textures[texture_id].x + texture_pools[pool_index].left,
        .y = texture_pools[pool_index].textures[texture_id].y + texture_pools[pool_index].top,
        .w = texture_pools[pool_index].textures[texture_id].w,
        .h = texture_pools[pool_index].textures[texture_id].h,
    };
}
