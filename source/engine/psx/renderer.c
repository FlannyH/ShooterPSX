#include "../renderer.h"
#include "renderer.h"
#include "hardware/gpu.h"

#define FRAMEBUFFER_COUNT 2

typedef struct {
    gpu_vertex_t top_left;
    gpu_vertex_t size;
} framebuffer_t;

int res_x = 0;
int res_y = 0;
int is_pal = 0;

framebuffer_t framebuffers[FRAMEBUFFER_COUNT] = {0};
int curr_bound_framebuffer  = 0;

#define NOP() __asm__ volatile("");

uint32_t as_u32(const void* value) {
    return *(uint32_t*)value;
}

void wait_for_gp0_ready(void) {
    while (!(GPU_STAT->ready_for_command)) NOP();
}

void reset_gpu(void) {
    GPU_GP1 = as_u32(&(gpu_cmd_gp1_reset_gpu_t) {
        .gp1_type = GPU_CMD_GP1_RESET_GPU
    });
}

void set_video_output(int16_t new_res_x, int16_t new_res_y, int set_24_bit, int set_pal) {
    is_pal     = set_pal;
    res_x = res_x;
    res_y = res_y;

    // Set video output mode based on resolution
    gpu_cmd_gp1_display_mode_t display_mode_params = {
        .is_pal            = set_pal,
        .flip_horizontally = 0,
        .gp1_type          = GPU_CMD_GP1_DISPLAY_MODE,
    };

    const int32_t center_x = 0x760;                // i love magic numbers
    const int32_t center_y = is_pal ? 0xa3 : 0x88; // i love magic numbers
    int32_t offset_x       = 0;
    int32_t offset_y       = 0;

    if (res_x <= 256) {
        display_mode_params.res_x1 = 0;
        display_mode_params.res_x2 = 0;
        offset_x                   = (res_x * 10) / 2;
    } else if (res_x <= 320) {
        display_mode_params.res_x1 = 1;
        display_mode_params.res_x2 = 0;
        offset_x                   = (res_x * 8) / 2;
    } else if (res_x <= 368) {
        display_mode_params.res_x1 = 0;
        display_mode_params.res_x2 = 1;
        offset_x                   = (res_x * 7) / 2;
    } else if (res_x <= 512) {
        display_mode_params.res_x1 = 2;
        display_mode_params.res_x2 = 0;
        offset_x                   = (res_x * 5) / 2;
    } else if (res_x <= 640) {
        display_mode_params.res_x1 = 3;
        display_mode_params.res_x2 = 0;
        offset_x                   = (res_x * 4) / 2;
    } else {
        printf("Invalid res_x %i - res_x must be 640 or lower", res_x);
    }

    if ((set_pal && res_y <= 288) || (!set_pal && res_y <= 240)) {
        display_mode_params.interlaced = 0;
        display_mode_params.res_y      = 0;
        offset_y                       = res_y / 2;
    } else if ((set_pal && res_y <= 576) || (!set_pal && res_y <= 480)) {
        display_mode_params.interlaced = 1;
        display_mode_params.res_y      = 1;
        offset_y                       = res_y / 4;
    } else {
        printf("invalid res_y %i - res_y must be 576 or lower for PAL, and 480 or lower for NTSC\n");
    }

    GPU_GP1 = as_u32(&(gpu_cmd_gp1_display_range_x_t) {
        .x1       = center_x - offset_x,
        .x2       = center_x + offset_x,
        .gp1_type = GPU_CMD_GP1_DISPLAY_RANGE_X,
    });

    GPU_GP1 = as_u32(&(gpu_cmd_gp1_display_range_y_t) {
        .y1       = center_y - offset_y,
        .y2       = center_y + offset_y,
        .gp1_type = GPU_CMD_GP1_DISPLAY_RANGE_Y,
    });

    GPU_GP1 = as_u32(&(gpu_cmd_gp1_display_area_start_t) {
        .x        = 0,
        .y        = 0,
        .gp1_type = GPU_CMD_GP1_DISPLAY_AREA_START,
    });

    wait_for_gp0_ready();

    GPU_GP1 = as_u32(&(gpu_cmd_gp0_tex_page_t) {
        .tex_page =
            {
                .enable_dither          = 1,
                .draw_to_display_buffer = 0,
            },
        .attribute_type = CMD_ATTRIBUTE_TEX_PAGE,
        .gp0_type       = GPU_CMD_GP0_ENV,
    });
}

void set_framebuffer(int index, gpu_vertex_t top_left, gpu_vertex_t size) {
    if (index >= FRAMEBUFFER_COUNT) {
        printf("invalid framebuffer index %i - must be between 0 and %i\n", index, FRAMEBUFFER_COUNT);
        return;
    }
    framebuffers[index].size.x     = size.x;
    framebuffers[index].size.y     = size.y;
    framebuffers[index].top_left.x = top_left.x;
    framebuffers[index].top_left.y = top_left.y;
}

void set_enable_display(int enable) {
    GPU_GP1 = as_u32(&(gpu_cmd_gp1_display_mask_t) {
        .enable = !enable,
        .gp1_type = GPU_CMD_GP1_DISPLAY_MASK
    });
}

void renderer_init(void) {
    res_x = 512;
    res_y = 240;
    reset_gpu();
    set_video_output(res_x, res_y, 0, 0);
    set_framebuffer(0, (gpu_vertex_t){0, 0}, (gpu_vertex_t){res_x, res_y});
    set_framebuffer(1, (gpu_vertex_t){res_x, 0}, (gpu_vertex_t){res_x, res_y});
    set_enable_display(1);

    // todo: set initial video mode
}

void renderer_begin_frame(void) {

}

void renderer_end_frame(void) {
    // wait for prev gpu command list to finish
    // wait for vsync if enabled
    // start gpu command dma
    // swap buffers
}

void renderer_set_view(const transform_t* view_transform) {

}

void renderer_draw_mesh_shaded(mesh_t* mesh, const transform_t* model_transform, int local, int facing_camera) {

}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, texture_category_t category) {

}

void renderer_draw_2d_line(vec2_t v0, vec2_t v1, pixel32_t color, int depth) {

}

void renderer_apply_fade(scalar_t fade_level) {

}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {

}

void renderer_upload_texture(const texture_cpu_t* texture, int index, texture_category_t category) {

}

void renderer_set_video_mode(video_mode_t new_video_mode) {

}

video_mode_t renderer_get_video_mode(void) {
    return (video_mode_t){0};
}

scalar_t renderer_delta_time(dt_flags_t flags) {
    return 0;
}

int renderer_should_close(void) {
    return 0;
}

void renderer_set_depth_bias(int bias) {

}

int renderer_n_meshes_drawn(void) {
    return 0;
}

int renderer_width(void) {
    return 0;
}

int renderer_height(void) {
    return 0;
}

texture_entry_t* renderer_get_texture_entry(texture_category_t category, int texture_id) {
    return NULL;
}

void renderer_psx_clear_vram(svec2_t top_left, svec2_t size, pixel32_t color) {

}
