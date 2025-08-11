#include <stdint.h>

void texture_pool_init(uint32_t pool_index, uint16_t left, uint16_t top, uint32_t resolution);
int texture_pool_alloc(uint32_t pool_index, int texture_id, uint32_t width, uint32_t height); // returns texture id within the pool
RECT texture_pool_rect(uint32_t pool_index, int texture_id);
