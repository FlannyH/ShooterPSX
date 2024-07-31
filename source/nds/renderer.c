#include "renderer.h"
#include "structs.h"

#include <nds.h>

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
