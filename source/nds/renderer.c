#include "renderer.h"
#include "structs.h"

#include <nds.h>

vec3_t camera_dir;
int tex_level_start = 0;
int tex_entity_start = 0;
int tex_weapon_start = 0;
int res_x = 256;
int vsync_enable = 1;
int is_pal = 0;

void renderer_init(void) {
    videoSetMode(MODE_0_3D);
    glInit();
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);
    glViewport(0, 0, 255, 191);
    vramSetBankA(VRAM_A_TEXTURE);
    gluPerspective(90, 256.0 / 192.0, 0.1, 40);
}

int16_t angle_to_16(int angle) {
    return (angle >> 1) & 0xFFFF;
}

void renderer_begin_frame(const transform_t* camera_transform) {
    // Set view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotateXi(angle_to_16(camera_transform->rotation.x));
    glRotateYi(angle_to_16(-camera_transform->rotation.y));
    glRotateZi(angle_to_16(camera_transform->rotation.z));
    glTranslatef32(-camera_transform->position.x, -camera_transform->position.y, -camera_transform->position.z);

    // Fetch view matrix
    int view_matrix[9];
    glGetFixed(GL_GET_MATRIX_VECTOR, &view_matrix[0]);
    camera_dir.x = view_matrix[2]; // todo: verify that these are correct
    camera_dir.y = view_matrix[5];
    camera_dir.z = view_matrix[8];
}

void renderer_end_frame(void) {
    // todo: update delta time
    glFlush(0);
}

void renderer_draw_mesh_shaded(const mesh_t* mesh, const transform_t* model_transform, int local, int tex_id_offset) {
    TODO()
}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
    TODO()
}

void renderer_apply_fade(int fade_level) {
    TODO()
}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {
    TODO()
}

void renderer_upload_texture(const texture_cpu_t* texture, uint8_t index) {
    TODO()
}

void renderer_upload_8bit_texture_page(const texture_cpu_t* texture, const uint8_t index) {
    TODO()
}

void renderer_set_video_mode(int is_pal) {
    TODO()
}

int renderer_get_delta_time_raw(void) {
    TODO()
}

int renderer_get_delta_time_ms(void) {
    TODO()
}

int renderer_convert_dt_raw_to_ms(int dt_raw) {
    TODO()
}

int renderer_should_close(void) {
    TODO()
}

void renderer_set_depth_bias(int bias) {
    TODO()
}