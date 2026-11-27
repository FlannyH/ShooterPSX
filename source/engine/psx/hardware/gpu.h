#ifndef GPU_H
#define GPU_H

#ifdef __cplusplus
extern "C" {
#endif

// Registers
#define GPU_GP0  (*(volatile uint32_t*)0x1F801810)
#define GPU_GP1  (*(volatile uint32_t*)0x1F801814)
#define GPU_READ GPU_GP0

// Command types
#define GPU_CMD_GP0_MISC              0
#define GPU_CMD_GP0_POLY              1
#define GPU_CMD_GP0_LINE              2
#define GPU_CMD_GP0_RECT              3
#define GPU_CMD_GP0_VRAM_TO_VRAM_BLIT 4
#define GPU_CMD_GP0_CPU_TO_VRAM_BLIT  5
#define GPU_CMD_GP0_VRAM_TO_CPU_BLIT  6
#define GPU_CMD_GP0_ENV               7

#define GPU_CMD_GP1_RESET_GPU          0x00
#define GPU_CMD_GP1_RESET_COMMANDS     0x01
#define GPU_CMD_GP1_ACK_IRQ1           0x02
#define GPU_CMD_GP1_DISPLAY_MASK       0x03
#define GPU_CMD_GP1_DMA_DIRECTION      0x04
#define GPU_CMD_GP1_DISPLAY_AREA_START 0x05
#define GPU_CMD_GP1_DISPLAY_RANGE_X    0x06
#define GPU_CMD_GP1_DISPLAY_RANGE_Y    0x07
#define GPU_CMD_GP1_DISPLAY_MODE       0x08
#define GPU_CMD_GP1_SET_VRAM_SIZE2     0x09
#define GPU_CMD_GP1_READ_GPU_REGISTER  0x10
#define GPU_CMD_GP1_SET_VRAM_SIZE1     0x20

#define CMD_MISC_CLEAR_CACHE 1
#define CMD_MISC_FILL_RECT   2

#define CMD_ATTRIBUTE_TEX_PAGE               1
#define CMD_ATTRIBUTE_TEX_WINDOW             2
#define CMD_ATTRIBUTE_DRAW_AREA_TOP_LEFT     3
#define CMD_ATTRIBUTE_DRAW_AREA_BOTTOM_RIGHT 4
#define CMD_ATTRIBUTE_DRAW_OFFSET            5
#define CMD_ATTRIBUTE_MASK_BIT               6

#define CMD_DMA_DIRECTION_OFF       0
#define CMD_DMA_DIRECTION_FIFO      1
#define CMD_DMA_DIRECTION_GP0_WRITE 2
#define CMD_DMA_DIRECTION_GP0_READ  3

#define GPU_RES_256 0
#define GPU_RES_320 1
#define GPU_RES_512 2
#define GPU_RES_640 3
#define GPU_RES_368 4

typedef struct {
    unsigned long tex_page_x_base_by_64   : 4;
    unsigned long tex_page_y_base_by_256  : 1;
    unsigned long semi_transparency       : 2; // 0: average, 1: additive, 2: subtractive, 3: 25% additive
    unsigned long tex_pagecolor_depth     : 2; // 0: 4-bit indexed, 1: 8-bit indexed, 2: 15-bit direct, 3: ???
    unsigned long dither_enabled          : 1;
    unsigned long draw_to_display_area    : 1;
    unsigned long enable_mask_test        : 1; // 0: ignore mask bit, 1: only draw if bit
    unsigned long write_mask              : 1;
    unsigned long interlace_field         : 1;
    unsigned long flip_horizontally       : 1; // only works on v1 GPU
    unsigned long tex_page_y_base_by_512  : 1; // only for 2 MB VRAM
    unsigned long res_x1                  : 2; // if res_x2 is 0,  0: 256, 1=320, 2=512, 3=640
    unsigned long res_x2                  : 1; // 0: use res_x1, 1: width = 368
    unsigned long res_y                   : 1; // 0: single-line (e.g. 240p), 1: double-line (e.g. 480p)
    unsigned long is_pal                  : 1; // 0: NTSC 60 Hz, 1: PAL 50 Hz
    unsigned long color_depth             : 1; // 0: 15-bit, 1: 24-bit
    unsigned long interlaced              : 1;
    unsigned long display_enabled         : 1;
    unsigned long interrupt_request       : 1;
    unsigned long data_request            : 1;
    unsigned long ready_for_command       : 1;
    unsigned long ready_for_vram_to_cpu   : 1;
    unsigned long ready_for_receiving_dma : 1;
    unsigned long dma_direction           : 2;
    unsigned long drawing_even_or_odd     : 1;
} gpu_stat_t;

#define GPU_STAT ((volatile gpu_stat_t*)0x1F801814)

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color24_t;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char _;
} color24_x8_t;

typedef struct {
    short x;
    short y;
} gpu_vertex_t;

typedef struct {
    unsigned char u; // pixels
    unsigned char v; // pixels
    union {
        unsigned short clut; // for vertex 0
        unsigned short page; // for vertex 1
    };
} gpu_uv_t;

typedef struct {
    unsigned short left_per_16;  // x coordinate of the rectangle's left edge, in steps of 16 pixels;
    unsigned short top;          // pixels
    unsigned short width_per_16; // rectangle's width, in steps of 16 pixels;
    unsigned short height;       // pixels
} gpu_params_rect_fill_t;

typedef struct {
    unsigned short x_by_64                : 4; // per 64 pixels
    unsigned short y_by_256               : 1; // per 256 pixels
    unsigned short semi_transparency      : 2; // 0: average, 1: additive, 2: subtractive, 3: 25% additive
    unsigned short color_depth            : 2; // 0: 4-bit indexed, 1: 8-bit indexed, 2: 15-bit direct, 3: ???
    unsigned short enable_dither          : 1; // only used in separate texpage primitive
    unsigned short draw_to_display_buffer : 1; // only used in separate texpage primitive
    unsigned short y_by_512               : 1; // only useful if you have 2 MB VRAM
    unsigned short flip_x                 : 1; // only used in separate texpage primitive
    unsigned short flip_y                 : 1; // only used in separate texpage primitive
    unsigned short unused                 : 2; // should be 0
} gpu_tex_page_t;

typedef struct {
    color24_t color;
    unsigned char misc_type : 5; // CMD_MISC_*
    unsigned char gp0_type  : 3; // should be GPU_CMD_GP0_MISC (0)
} gpu_cmd_gp0_misc_t;

typedef struct {
    color24_t color;
    unsigned char ignore_vertex_color : 1; // 0: use vertex color, 1: ignore vertex color
    unsigned char semi_transparent    : 1; // 0: opaque,           1: semi-transparent
    unsigned char textured            : 1; // 0: untextured,       1: textured
    unsigned char quad                : 1; // 0: triangle,         1: quad
    unsigned char gouraud_shaded      : 1; // 0: flat shaded,      1: gouraud shaded
    unsigned char gp0_type            : 3; // should be GPU_CMD_GP0_POLY (1)
} gpu_cmd_gp0_polygon_t;

typedef struct {
    color24_t color;
    unsigned char _1               : 1; // unused
    unsigned char semi_transparent : 1; // 0 = opaque,      1 = semi-transparent
    unsigned char _0               : 1; // unused
    unsigned char polyline         : 1; // 0 = single line, 1 = polyline
    unsigned char gouraud_shaded   : 1; // 0 = flat shaded, 1 = gouraud shaded
    unsigned char gp0_type         : 3; // should be GPU_CMD_GP0_LINE (2)
} gpu_cmd_gp0_line_t;

typedef struct {
    color24_t color;
    unsigned char ignore_vertex_color : 1; // 0 = use vertex color, 1 = ignore vertex color
    unsigned char semi_transparent    : 1; // 0 = opaque,           1 = semi-transparent
    unsigned char textured            : 1; // 0 = untextured,       1 = textured
    unsigned char size                : 2; // 0 = variable size, 1 = color 1x1, 2 = texture 8x8, 3 = texture 16x16
    unsigned char gp0_type            : 3; // should be GPU_CMD_GP0_RECT (3)
} gpu_cmd_gp0_rectangle_t;

typedef struct {
    unsigned long _        : 29; // unused
    unsigned long gp0_type : 3;  // should be GPU_CMD_GP0_VRAM_TO_VRAM_BLIT (4)
    unsigned short source_x;
    unsigned short source_y;
    unsigned short destination_x;
    unsigned short destination_y;
    unsigned short size_x;
    unsigned short size_y;
} gpu_cmd_gp0_vram_to_vram_blit_t;

typedef struct {
    unsigned long _        : 29; // unused
    unsigned long gp0_type : 3;  // should be GPU_CMD_GP0_CPU_TO_VRAM_BLIT (5)
    unsigned short destination_x;
    unsigned short destination_y;
    unsigned short size_x;
    unsigned short size_y;
    // DMA the raw data to VRAM after this struct
} gpu_cmd_gp0_cpu_to_vram_blit_t;

typedef struct {
    unsigned long _        : 29; // unused
    unsigned long gp0_type : 3;  // should be GPU_CMD_GP0_VRAM_TO_CPU_BLIT (6)
    unsigned short source_x;
    unsigned short source_y;
    unsigned short size_x;
    unsigned short size_y;
    // DMA the raw data to the CPU after this struct
} gpu_cmd_gp0_vram_to_cpu_blit_t;

typedef struct {
    gpu_tex_page_t tex_page;
    unsigned char _;                  // should be 0
    unsigned char attribute_type : 5; // should be CMD_ATTRIBUTE_TEX_PAGE (1)
    unsigned char gp0_type       : 3; // should be GPU_CMD_GP0_ENV (7)
} gpu_cmd_gp0_tex_page_t;

typedef struct {
    unsigned long mask_x_per_8   : 5; // per 8 pixels
    unsigned long mask_y_per_8   : 5; // per 8 pixels
    unsigned long offset_x_per_8 : 5; // per 8 pixels
    unsigned long offset_y_per_8 : 5; // per 8 pixels
    unsigned long _              : 4; // unused
    unsigned long attribute_type : 5; // should be CMD_ATTRIBUTE_TEX_WINDOW (2)
    unsigned long gp0_type       : 3; // should be GPU_CMD_GP0_ENV (7)
} gpu_cmd_gp0_tex_window_t;

typedef struct {
    unsigned long x              : 10; // x coordinate in pixel
    unsigned long y              : 10; // y coordinate in pixel
    unsigned long _              : 4;  // unused
    unsigned long attribute_type : 5;  // should be CMD_ATTRIBUTE_DRAW_AREA_TOP_LEFT (3), or CMD_ATTRIBUTE_DRAW_AREA_BOTTOM_RIGHT (4)
    unsigned long gp0_type       : 3;  // should be GPU_CMD_GP0_ENV (7)
} gpu_cmd_gp0_draw_area_t;

typedef struct {
    long x                       : 11; // pixels
    long y                       : 11; // pixels
    unsigned long _              : 2;  // unused
    unsigned long attribute_type : 5;  // should be CMD_ATTRIBUTE_DRAW_OFFSET (5)
    unsigned long gp0_type       : 3;  // should be GPU_CMD_GP0_ENV (7)
} gpu_cmd_gp0_draw_offset_t;

typedef struct {
    unsigned long write_mask       : 1;
    unsigned long enable_mask_test : 1;  // 0: ignore mask bit, 1: only draw if bit
    unsigned long _                : 22; // unused
    unsigned long attribute_type   : 5;  // should be CMD_ATTRIBUTE_MASK_BIT (6)
    unsigned long gp0_type         : 3;  // should be GPU_CMD_GP0_ENV (7)
} gpu_cmd_gp0_mask_settings_t;

typedef struct {
    unsigned long _        : 24; // unused
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_RESET_GPU (0x00)
} gpu_cmd_gp1_reset_gpu_t;

typedef struct {
    unsigned long _        : 24; // unused
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_RESET_COMMANDS (0x01)
} gpu_cmd_gp1_reset_commands_t;

typedef struct {
    unsigned long _        : 24; // unused
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_ACK_IRQ1 (0x02)
} gpu_cmd_gp1_ack_irq1_t;

typedef struct {
    unsigned long enable   : 1; // 0 = render as usual, 1 = blank
    unsigned long _        : 23; // unused
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_DISPLAY_MASK (0x03)
} gpu_cmd_gp1_display_mask_t;

typedef struct {
    unsigned long dma_direction : 2;
    unsigned long _             : 22; // unused
    unsigned long gp1_type      : 8;  // should be GPU_CMD_GP1_DMA_DIRECTION (0x04)
} gpu_cmd_gp1_dma_direction_t;

typedef struct {
    unsigned long x        : 10; // pixels
    unsigned long y        : 9;  // pixels
    unsigned long _        : 5;  // unused
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_DISPLAY_AREA_START (0x05)
} gpu_cmd_gp1_display_area_start_t;

typedef struct {
    unsigned long x1       : 12; // video clock cycles since HSYNC
    unsigned long x2       : 12; // video clock cycles since HSYNC
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_DISPLAY_RANGE_X (0x06)
} gpu_cmd_gp1_display_range_x_t;

typedef struct {
    unsigned long y1       : 10; // scanlines since VSYNC
    unsigned long y2       : 10; // scanlines since VSYNC
    unsigned long _        : 4;  // unused
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_DISPLAY_RANGE_Y (0x07)
} gpu_cmd_gp1_display_range_y_t;

typedef struct {
    unsigned long res_x1            : 2; // if res_x2 is 0,  0: 256, 1=320, 2=512, 3=640
    unsigned long res_y             : 1; // 0: single-line (e.g. 240p), 1: double-line (e.g. 480p)
    unsigned long is_pal            : 1; // 0: NTSC 60 Hz, 1: PAL 50 Hz
    unsigned long color_depth       : 1; // 0: 15-bit, 1: 24-bit
    unsigned long interlaced        : 1;
    unsigned long res_x2            : 1;  // 0: use res_x1, 1: width = 368
    unsigned long flip_horizontally : 1;  // only works on v1 GPU
    unsigned long _                 : 16; // unused
    unsigned long gp1_type          : 8;  // should be GPU_CMD_GP1_DISPLAY_MODE (0x08)
} gpu_cmd_gp1_display_mode_t;

typedef struct {
    unsigned long register_index : 24; // which you can read immediately afterwards from GPU_READ
    unsigned long gp1_type       : 8;  // should be GPU_CMD_GP1_READ_GPU_REGISTER (0x10)
} gpu_cmd_gp1_read_register_t;

typedef struct {
    unsigned long allow_2mb_vram_usage : 1;  // 0: wrap y=0-511, 1: allow Y coordinates in 512-1023 range
    unsigned long _                    : 23; // unused
    unsigned long gp1_type             : 8;  // should be GPU_CMD_GP1_SET_VRAM_SIZE2 (0x09)
} gpu_cmd_gp1_set_vram_size2_t;

typedef struct {
    unsigned long unknown  : 24; // docs were very unclear on this one
    unsigned long gp1_type : 8;  // should be GPU_CMD_GP1_SET_VRAM_SIZE1 (0x20)
} gpu_cmd_gp1_set_vram_size1_t;

#ifdef __cplusplus
}
#endif

#endif
