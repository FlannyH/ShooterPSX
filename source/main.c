#include "main.h"
#include "input.h"
#include "renderer.h"
#include "music.h"
#ifdef _PSX
#include <psxspu.h>
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#else
#include "win/psx.h"
#endif
#include "camera.h"

#include "texture.h"

int main(void) {
    renderer_init();
    input_init();
    init();

    uint32_t frame_index = 0;

    // Camera transform
    Transform camera_transform;
    camera_transform.position.vx = -11558322;
    camera_transform.position.vy = 14423348;
    camera_transform.position.vz = 6231602;
    camera_transform.rotation.vx = 23793;
    camera_transform.rotation.vy = 196707;
    camera_transform.rotation.vz = 0;

    // Stick deadzone
    input_set_stick_deadzone(36);

    // Load model
    Model* m_cube = model_load("\\ASSETS\\CUBETEST.MSH;1");
    const Model* m_level = model_load("\\ASSETS\\LEVEL.MSH;1");

    TextureCPU* tex_level;
    const uint32_t n_textures = texture_collection_load("\\ASSETS\\LEVEL.TXC;1", &tex_level);

    for (uint8_t i = 0;i < n_textures; ++i) {
        renderer_upload_texture(&tex_level[i], i);
    }

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
    music_play_file("\\ASSETS\\MUSIC.XA;1");

    int frame_counter = 0;
    while (1)
    {
        const int delta_time = renderer_get_delta_time_ms();
        renderer_begin_frame(&camera_transform);
        input_update();
        camera_update_flycam(&camera_transform, delta_time);
        t_cube1.rotation.vy += 64 * delta_time;
        //renderer_draw_mesh_shaded(m_cube, t_cube1);
        renderer_draw_model_shaded(m_level, &t_level);
#ifdef _PSX
        FntPrint(-1, "Frame: %i\n", frame_index++);
        FntPrint(-1, "Delta Time (hblank): %i\n", delta_time);
        FntPrint(-1, "Est. FPS: %i\n", 1000/delta_time);
        FntPrint(-1, "Triangles (rendered/total): %i / %i\n", renderer_get_n_rendered_triangles(), renderer_get_n_total_triangles());
        FntFlush(-1);
#endif
        renderer_end_frame();
    }
}

void init(void)
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