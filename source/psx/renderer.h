#ifndef RENDERER_PSX_H
#define RENDERER_PSX_H

#include <stdint.h>

#include "../texture.h"
#include "../common.h"
#include "../vec2.h"

void renderer_psx_clear_vram(svec2_t top_left, svec2_t size, pixel32_t color);

#endif
