#ifndef TEXTURE_POOL_H
#define TEXTURE_POOL_H

#include <stdint.h>
#include "structs.h"

#define MAX_TEXTURE_POOL_COUNT 8

void texture_pool_init(uint32_t pool_index, uint16_t left, uint16_t top, uint32_t resolution);
int texture_pool_alloc(uint32_t pool_index, uint32_t width, uint32_t height); // returns texture id within the pool
rect_t texture_pool_rect(uint32_t pool_index, int texture_id);
void texture_pool_free(uint32_t pool_index, int* texture_ids, int texture_count);

#endif
