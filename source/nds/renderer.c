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
int n_rendered_triangles = 0;
int n_rendered_quads = 0;

void renderer_init(void) {
    videoSetMode(MODE_0_3D);
    // Setup layer 1 as console layer
    consoleDemoInit();
    glInit();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glFlush(0);
    glClearColor(2, 2, 4, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);
    glViewport(0, 0, 255, 191);
    vramSetBankA(VRAM_A_TEXTURE);    
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankD(VRAM_D_TEXTURE);
    vramSetBankF(VRAM_F_TEX_PALETTE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90, 256.0 / 192.0, 0.01, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int16_t angle_to_16(int angle) {
    return (angle >> 2) & 0xFFFF;
}

void renderer_begin_frame(const transform_t* camera_transform) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90, 256.0 / 192.0, 0.01, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef32(ONE, -ONE, -ONE);
    glRotateXi(angle_to_16(camera_transform->rotation.x));
    glRotateYi(angle_to_16(camera_transform->rotation.y));
    glRotateZi(angle_to_16(camera_transform->rotation.z));
    glTranslatef32(-camera_transform->position.x >> 12, -camera_transform->position.y >> 12, -camera_transform->position.z >> 12);
    
    n_rendered_triangles = 0;
    n_rendered_quads = 0;
}

void renderer_end_frame(void) {
    // todo: update delta time
    glFlush(0);
    for (int i = 0; i < vsync_enable; ++i) {
        swiWaitForVBlank();
    }
}

void renderer_draw_mesh_shaded(const mesh_t* mesh, const transform_t* model_transform, int local, int tex_id_offset) {
    // Set up model view matrix
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (local) {
        glLoadIdentity();
        glTranslatef32(model_transform->position.x, -model_transform->position.y, -model_transform->position.z);
        glScalef32(-model_transform->scale.x, -model_transform->scale.y, -model_transform->scale.z);
        glRotateXi(angle_to_16(model_transform->rotation.x));
        glRotateYi(angle_to_16(-model_transform->rotation.y));
        glRotateZi(angle_to_16(model_transform->rotation.z));
    }
    else {
        glTranslatef32(model_transform->position.x, model_transform->position.y, model_transform->position.z);
        glScalef32(model_transform->scale.x, model_transform->scale.y, model_transform->scale.z);
        glRotateZi(angle_to_16(model_transform->rotation.z));
        glRotateYi(angle_to_16(model_transform->rotation.y));
        glRotateXi(angle_to_16(model_transform->rotation.x));
    }

    // todo - display lists
    // Draw triangles
    size_t vert_idx = 0;
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < mesh->n_triangles; ++i) {
        uint8_t tex_id = mesh->vertices[vert_idx].tex_id;
        if (tex_id != 255) glBindTexture(0, textures[(int)tex_id + tex_id_offset]);
        else glBindTexture(0, 0);
        const vertex_3d_t v0 = mesh->vertices[vert_idx];
        const vertex_3d_t v1 = mesh->vertices[vert_idx + 2];
        const vertex_3d_t v2 = mesh->vertices[vert_idx + 1];
        glColor3b(v0.r, v0.g, v0.b);
        glTexCoord2i(v0.u >> 2, v0.v >> 2);
        glVertex3v16(v0.x, v0.y, v0.z);
        glColor3b(v1.r, v1.g, v1.b);
        glTexCoord2i(v1.u >> 2, v1.v >> 2);
        glVertex3v16(v1.x, v1.y, v1.z);
        glColor3b(v2.r, v2.g, v2.b);
        glTexCoord2i(v2.u >> 2, v2.v >> 2);
        glVertex3v16(v2.x, v2.y, v2.z);
        vert_idx += 3;
        n_rendered_triangles++;
    }
    glEnd();
    // Draw quads
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT);
    glBegin(GL_QUADS);
    for (size_t i = 0; i < mesh->n_quads; ++i) {
        uint8_t tex_id = mesh->vertices[vert_idx].tex_id;
        if (tex_id != 255) glBindTexture(0, textures[(int)tex_id + tex_id_offset]);
        else glBindTexture(0, 0);
        const vertex_3d_t v0 = mesh->vertices[vert_idx];
        const vertex_3d_t v1 = mesh->vertices[vert_idx + 1];
        const vertex_3d_t v2 = mesh->vertices[vert_idx + 3];
        const vertex_3d_t v3 = mesh->vertices[vert_idx + 2];
        glColor3b(v0.r, v0.g, v0.b);
        glTexCoord2i(v0.u >> 2, v0.v >> 2);
        glVertex3v16(v0.x, v0.y, v0.z);
        glColor3b(v1.r, v1.g, v1.b);
        glTexCoord2i(v1.u >> 2, v1.v >> 2);
        glVertex3v16(v1.x, v1.y, v1.z);
        glColor3b(v2.r, v2.g, v2.b);
        glTexCoord2i(v2.u >> 2, v2.v >> 2);
        glVertex3v16(v2.x, v2.y, v2.z);
        glColor3b(v3.r, v3.g, v3.b);
        glTexCoord2i(v3.u >> 2, v3.v >> 2);
        glVertex3v16(v3.x, v3.y, v3.z);
        vert_idx += 4;
        n_rendered_quads++;
    }
    glEnd();

    // Revert matrix
    glPopMatrix(1);
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
    tl.y = ONE - (((tl.y * 2) - (32 * ONE)) / 240);
    tr.y = ONE - (((tr.y * 2) - (32 * ONE)) / 240);
    bl.y = ONE - (((bl.y * 2) - (32 * ONE)) / 240);
    br.y = ONE - (((br.y * 2) - (32 * ONE)) / 240);

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
    if (fade_level <= 0) return; 

    // Reset all matrices
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Draw transparent quad
    glBindTexture(0, texture_pages[4]);
    glPolyFmt(POLY_ALPHA(fade_level >> 3) | POLY_CULL_NONE);
    glColor3b(0, 0, 0);

    glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3v16(-ONE, -ONE, 0);
        glTexCoord2i(0, 0);
        glVertex3v16(ONE, -ONE, 0);
        glTexCoord2i(0, 0);
        glVertex3v16(ONE, ONE, 0);
        glTexCoord2i(0, 0);
        glVertex3v16(-ONE, ONE, 0);
    glEnd();
    
    // Put the matrices back
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix(1);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(1);
}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {
    (void)v0;
    (void)v1;
    (void)color;
    (void)model_transform;
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
    (void)is_pal;
    TODO()
}

int renderer_get_delta_time_raw(void) {
    return vsync_enable; // todo: return actual number of vblanks this frame took, probably with a vblank interrupt
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
    (void)bias;
    TODO_SOFT()
}