#ifndef RENDERER_PSX_H
#define RENDERER_PSX_H

#include <stdint.h>

#include "../texture.h"
#include "../common.h"
#include "../vec2.h"

typedef struct ALIGN(4) {
    uint16_t tpage;
    uint16_t clut;
    uint8_t offset_u;
    uint8_t offset_v;
    uint8_t texture_pool_id;
    uint8_t texture_entry_id;
    uint8_t palette_pool_id;
    uint8_t palette_entry_id;
    uint8_t allocated;
    pixel32_t average_color;
} texture_entry_t;

void renderer_psx_clear_vram(svec2_t top_left, svec2_t size, pixel32_t color);

#endif
