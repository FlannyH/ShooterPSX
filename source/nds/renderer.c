#include "renderer.h"
#include "structs.h"

#include <nds.h>

vec3_t camera_dir;

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