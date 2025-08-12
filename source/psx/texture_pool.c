#include <stdio.h>
#include <psxgpu.h>
#include <assert.h>

#include "texture_pool.h"
#include "renderer.h"
#include "../renderer.h"
#include "../common.h"
#include "../memory.h"

#define MAX_RES_SHIFTS 9
#define MAX_RES (1<<MAX_RES_SHIFTS)
#define MAX_TEXTURE_POOL_COUNT 8

typedef struct {
    uint16_t top, left;
    uint32_t resolution;
    RECT* textures; // if a texture's width or height are 0, it's unallocated
    uint32_t n_occupancy_maps;
    uint32_t** occupancy_maps;
} texture_pool_t;

static texture_pool_t texture_pools[MAX_TEXTURE_POOL_COUNT] = {0};

#define GET_BIT_INT(v, bit) ((v) & (1u << (bit)))
#define SET_BIT_ARR(a, bit) ((a)[((bit) >> 5)]) |= (1u << ((bit) & 31));
#define GET_BIT_ARR(a, bit) (( ((a)[((bit) >> 5)]) >> ((bit) & 31) ) & 1)
#define RESET_BIT_ARR(a, bit) ((a)[((bit) >> 5)]) &= ~(1u << ((bit) & 31));

void texture_pool_init(uint32_t pool_index, uint16_t left, uint16_t top, uint32_t resolution) {
    if (resolution > MAX_RES) {
        printf("[ERROR] Texture pool too big (%ix%i), should be max (%ix%i)\n", resolution, resolution, MAX_RES, MAX_RES);
        return;
    }
    
    if (pool_index >= MAX_TEXTURE_POOL_COUNT) {
        printf("[ERROR] Texture pool index (%i) out of bounds, should be between 0 and %i\n", pool_index, MAX_TEXTURE_POOL_COUNT - 1);
        return;
    }

    texture_pools[pool_index].top = top;
    texture_pools[pool_index].left = left;
    texture_pools[pool_index].resolution = resolution;
    texture_pools[pool_index].occupancy_maps = mem_stack_alloc(sizeof(uint32_t*) * MAX_RES_SHIFTS, STACK_PERSISTENT);
    texture_pools[pool_index].textures = mem_stack_alloc(sizeof(RECT) * MAX_TEXTURE_COUNT, STACK_PERSISTENT);

    uint32_t level_count = 0;
    for (uint32_t r = resolution; r > 0; r >>= 1) {
        // allocate occupancy map
        const uint32_t bit_count = r * r;
        const uint32_t word_count = ((bit_count + 31) / 32);
        uint32_t* data = mem_stack_alloc(word_count * sizeof(uint32_t), STACK_PERSISTENT);
        texture_pools[pool_index].occupancy_maps[level_count++] = data;

        // clear the memory to 0 (free block)
        for (size_t i = 0; i < word_count; ++i) data[i] = 0;
    }

    for (uint32_t i = 0; i < MAX_TEXTURE_COUNT; ++i) {
        texture_pools[pool_index].textures[i] = (RECT){-1, -1, -1, -1};
    }

    texture_pools[pool_index].n_occupancy_maps = level_count;

#ifdef _DEBUG
    renderer_psx_clear_vram((svec2_t){left, top}, (svec2_t){resolution, resolution}, (pixel32_t){255, 0, 0, 0});
#endif
}

int texture_pool_alloc(uint32_t pool_index, uint32_t width, uint32_t height) {
    if(pool_index >= MAX_TEXTURE_POOL_COUNT) return -2;
    if (texture_pools[pool_index].textures == NULL) return -3;
    if (texture_pools[pool_index].occupancy_maps == NULL) return -4;
    if (texture_pools[pool_index].resolution < width) return -5;
    if (texture_pools[pool_index].resolution < height) return -6;
    
    int texture_id = -1;
    for (uint32_t i = 0; i < MAX_TEXTURE_COUNT; ++i) {
        if (texture_pools[pool_index].textures[i].w <= 0 || texture_pools[pool_index].textures[i].h <= 0) {
            texture_id = i;
            break;
        }
    }
    if (texture_id == -1) { 
        printf("could not find empty texture in pool %i\n", pool_index);
        return -1;
    }

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
                    const uint32_t bit_index = (y * curr_res) + x;
                    const uint32_t word_index = bit_index >> 5u;
                    const uint32_t mask = 1u << (bit_index & 31u);
                    const uint32_t bit_set = occ_map[word_index] & mask;
                    if (bit_set != 0) ++collisions;
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
    printf("for pool index %i, allocated texture %i (%ix%i) at location (%i, %i)\n", pool_index, texture_id, width, height, left, top);
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

void texture_pool_free(uint32_t pool_index, int texture_id) {
    RECT* texture = &texture_pools[pool_index].textures[texture_id];

    if (texture->w <= 0 || texture->h <= 0) {
        return;
    }

    // Free in pixel map
    uint32_t r = texture_pools[pool_index].resolution;
    uint32_t start_x = texture->x;
    uint32_t start_y = texture->y;
    uint32_t end_x = texture->x + texture->w;
    uint32_t end_y = texture->y + texture->h;

    uint32_t* occ_map = texture_pools[pool_index].occupancy_maps[0];
    for (uint32_t y = start_y; y < end_y; ++y){
        for (uint32_t x = start_x; x < end_x; ++x){
            RESET_BIT_ARR(occ_map, (y * r) + x);
        }
    }

    // Downsample pixel map, marking as occupied if any of the 4 sampled pixels are occupied
    uint32_t res_read = texture_pools[pool_index].resolution;
    uint32_t res_write = res_read >> 1;
    for (uint32_t occ_map_index = 0; occ_map_index < texture_pools[pool_index].n_occupancy_maps - 1; occ_map_index++) {
        uint32_t* occ_map_read = texture_pools[pool_index].occupancy_maps[occ_map_index];
        uint32_t* occ_map_write = texture_pools[pool_index].occupancy_maps[occ_map_index + 1];
        r >>= 1;
        for (uint32_t y = 0; y < res_write; ++y){
            for (uint32_t x = 0; x < res_write; ++x){
                uint32_t index1 = (((y*2)+0) * res_read) + (x*2)+0;
                uint32_t index2 = (((y*2)+0) * res_read) + (x*2)+1;
                uint32_t index3 = (((y*2)+1) * res_read) + (x*2)+0;
                uint32_t index4 = (((y*2)+1) * res_read) + (x*2)+1;
                uint32_t sample1 = GET_BIT_ARR(occ_map_read, index1);
                uint32_t sample2 = GET_BIT_ARR(occ_map_read, index2);
                uint32_t sample3 = GET_BIT_ARR(occ_map_read, index3);
                uint32_t sample4 = GET_BIT_ARR(occ_map_read, index4);
                if (sample1 | sample2 | sample3 | sample4) {
                    SET_BIT_ARR(occ_map_write, (y * res_write) + x);
                }
                else {
                    RESET_BIT_ARR(occ_map_write, (y * res_write) + x);
                }
            }
        }
        res_read = res_write;
        res_write = res_read >> 1;
    }

    // Mark as free by setting resolution to 0x0
    texture->w = 0;
    texture->h = 0;
}
