#include "renderer.h"
#include "structs.h"
#include "vec2.h"

#include <gl2d.h>
#include <nds.h>

vec3_t camera_dir;
int tex_level_start = 0;
int tex_entity_start = 0;
int tex_weapon_start = 0;
int res_x = 256;
int vsync_enable = 1;
int is_pal = 0;
int textures[256] = {0};
int texture_pages[8] = {0};

void renderer_init(void) {
    videoSetMode(MODE_0_3D);
    // Setup layer 1 as console layer
    consoleDemoInit();
    glInit();
    glEnable(GL_TEXTURE_2D);
    glFlush(0);
    glClearColor(0, 0, 0, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);
    glViewport(0, 0, 255, 191);
    vramSetBankA(VRAM_A_TEXTURE);    
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankF(VRAM_F_TEX_PALETTE);
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
    TODO_SOFT()
}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
    // Reset all matrices
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Map 512x240 to 256x192, in fixed point 20.12
    tl.x = ((tl.x * 2) / 512) - ONE;
    tr.x = ((tr.x * 2) / 512) - ONE;
    bl.x = ((bl.x * 2) / 512) - ONE;
    br.x = ((br.x * 2) / 512) - ONE;
    tl.y = ONE - ((tl.y * 2) / 240);
    tr.y = ONE - ((tr.y * 2) / 240);
    bl.y = ONE - ((bl.y * 2) / 240);
    br.y = ONE - ((br.y * 2) / 240);

    // Render quad
    glPolyFmt(POLY_ALPHA(color.a >> 3) | POLY_CULL_NONE);
    if (is_page) glBindTexture(0, texture_pages[texture_id]);
    else glBindTexture(0, textures[texture_id]);
    glColor3b(
        scalar_clamp((int)color.r * 2, 0, 255),
        scalar_clamp((int)color.g * 2, 0, 255),
        scalar_clamp((int)color.b * 2, 0, 255)
    );
    glBegin(GL_QUADS);
        glTexCoord2i(uv_tl.x / ONE, uv_tl.y / ONE); // v1
        glVertex3v16(tl.x, tl.y, depth);
        glTexCoord2i(uv_br.x / ONE, uv_tl.y / ONE); // v2
        glVertex3v16(tr.x, tr.y, depth);
        glTexCoord2i(uv_br.x / ONE, uv_br.y / ONE); // v3
        glVertex3v16(br.x, br.y, depth);
        glTexCoord2i(uv_tl.x / ONE, uv_br.y / ONE); // v4
        glVertex3v16(bl.x, bl.y, depth);
    glEnd();

    // Put the matrices back
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix(1);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(1);
}

void renderer_apply_fade(int fade_level) {
    TODO_SOFT()
}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {
    TODO()
}

void renderer_upload_texture(const texture_cpu_t* texture, uint8_t index) {
    if (textures[index] != 0) glDeleteTextures(1, &textures[index]);
    glGenTextures(1, &textures[index]);
    glBindTexture(0, textures[index]);
    if (glTexImage2D(0, 0, GL_RGB16, 64, 64, 0, TEXGEN_OFF | ((texture->palette[0].a == 0) ? GL_TEXTURE_COLOR0_TRANSPARENT : 0), texture->data) == 0) {
        printf("Error loading texture %i pixels\n", index);
    }
    if (glColorTableEXT(0, 0, 16, 0, 0, texture->palette) == 0) {
        printf("Error loading texture %i palette\n", index);
    }
}

void renderer_upload_8bit_texture_page(const texture_cpu_t* texture, const uint8_t index) {
    if (texture_pages[index] != 0) glDeleteTextures(1, &texture_pages[index]);
    glGenTextures(1, &texture_pages[index]);
    glBindTexture(0, texture_pages[index]);
    if (glTexImage2D(0, 0, GL_RGB256, 256, 256, 0, TEXGEN_OFF | ((texture->palette[0].a == 0) ? GL_TEXTURE_COLOR0_TRANSPARENT : 0), texture->data) == 0) {
        printf("Error loading texture page %i pixels\n", index);
    }
    if (glColorTableEXT(0, 0, 256, 0, 0, texture->palette) == 0) {
        printf("Error loading texture page %i palette\n", index);
    }
}

void renderer_set_video_mode(int is_pal) {
    TODO()
}

int renderer_get_delta_time_raw(void) {
    return 1; // todo: return actual number of vblanks this frame took, probably with a vblank interrupt
}

int renderer_get_delta_time_ms(void) {
    TODO()
}

int renderer_convert_dt_raw_to_ms(int dt_raw) {
    return (1666 * dt_raw) / 100;
}

int renderer_should_close(void) {
    return 0;
}

void renderer_set_depth_bias(int bias) {
    TODO_SOFT()
}