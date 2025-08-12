#ifndef RENDERER_PSX_H
#define RENDERER_PSX_H

#include <stdint.h>

#include "../texture.h"
#include "../common.h"

typedef struct ALIGN(4) {
    uint16_t tpage;
    uint16_t clut;
    uint8_t offset_u;
    uint8_t offset_v;
    uint8_t pool_id;
    uint8_t texture_id;
    pixel32_t average_color;
} texture_entry_t;

#endif
