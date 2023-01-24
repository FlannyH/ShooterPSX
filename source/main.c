#include "main.h"
#include "input.h"
#include "renderer.h"
#include "music.h"
#ifdef _PSX
#include <psxspu.h>
#include <psxsio.h>
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#else
#include "win/psx.h"
#endif
#include "camera.h"
#include <stdio.h>

int main() {
    renderer_init();
    input_init();
    init();

    uint32_t frame_index = 0;

    // Camera transform
    Transform camera_transform;
    camera_transform.position.vx = 0;
    camera_transform.position.vy = 0;
    camera_transform.position.vz = 0;
    camera_transform.rotation.vx = 0;
    camera_transform.rotation.vy = 0;
    camera_transform.rotation.vz = 0;

    // Load model
    Mesh* m_cube = mesh_load("\\ASSETS\\CUBETEST.MSH;1");
    Mesh* m_level = mesh_load("\\ASSETS\\LEVEL.MSH;1");
    Transform t_cube1 = { 
        {0, 0, 500},
        {-2048, 0, 0},
        {0, 0, 0}
    };
    Transform t_level = { 
        {0, 0, 0},
        {-2048, 0, 0},
        {0, 0, 0}
    };

    // Play music
    //music_play_file("\\MUSIC.XA");

    int delta_time = 300;
    int frame_counter = 0;
    while (1)
    {
        delta_time = renderer_get_delta_time_ms();
        renderer_begin_frame(&camera_transform);
        input_update();
        camera_update_flycam(&camera_transform, delta_time);
        t_cube1.rotation.vy += 64 * delta_time;
        renderer_draw_mesh_shaded(m_cube, t_cube1);
        renderer_draw_mesh_shaded(m_level, t_level);
        FntPrint(-1, "Frame: %i\n", frame_index++);
        FntPrint(-1, "Delta Time (hblank): %i\n", delta_time);
        FntPrint(-1, "Est. FPS: %i\n", 1000/delta_time);
        FntFlush(-1);
        renderer_end_frame();
    }
    return 0;
}

void init()
{
    // todo: make this more platform indepentend
#ifdef _PSX
    // Init CD
	SpuInit();
    CdInit();

    // Load the internal font texture
    FntLoad(960, 0);
    // Create the text stream
    FntOpen(0, 8, 320, 224, 0, 100);

#endif
    // todo: textures
}