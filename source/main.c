#include "main.h"
#include "input.h"
#include "music.h"
#include "renderer.h"

#ifdef _PSX
#include <psxcd.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxspu.h>
#include <psxpad.h>

#else
#include "win/psx.h"
#include "win/debug_layer.h"
#endif

#include "camera.h"
#include "fixed_point.h"
#include "player.h"

#include "texture.h"

int main(void) {
	renderer_init();
	input_init();
	init();

    // Init player
    player_t player = { 0 };

    player.position.x = 11705653 * 2;
   	player.position.y = 11413985 * 2;
    player.position.z = 2112866 * 2;
    //player.position.x = 0;
    //player.position.y = -229376;
    //player.position.z = 0;

	// Stick deadzone
	input_set_stick_deadzone(36);

	// Load model
    const model_t* m_level = model_load("\\ASSETS\\LEVEL.MSH;1");
	const model_t *m_level_collision = model_load("\\ASSETS\\LEVELCOL.MSH;1");

	texture_cpu_t *tex_level;
	const uint32_t n_textures =
		texture_collection_load("\\ASSETS\\LEVEL.TXC;1", &tex_level);

	for (uint8_t i = 0; i < n_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i);
	}

	transform_t t_level = {{0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096}};
    
    bvh_t bvh_level_model;
    bvh_from_model(&bvh_level_model, m_level_collision);

	// Play music
	music_play_file("\\ASSETS\\MUSIC.XA;1");

	int frame_counter = 0;
    while (!renderer_should_close()) {
        int delta_time = renderer_get_delta_time_ms();
        if (delta_time > 34) {
            delta_time = 34;
        }
        renderer_begin_frame(&player.transform);
        input_update();
        renderer_draw_model_shaded(m_level, &t_level);
        player_update(&player, &bvh_level_model, delta_time);
        frame_counter += delta_time;
        
    #ifdef _PSX
	    FntPrint(-1, "Frame: %i\n", frame_counter++);
	    FntPrint(-1, "Delta Time (hblank): %i\n", delta_time);
	    FntPrint(-1, "Est. FPS: %i\n", 1000 / delta_time);
	    FntPrint(-1, "Triangles (rendered/total): %i / %i\n",
			     renderer_get_n_rendered_triangles(),
			     renderer_get_n_total_triangles());
	    FntFlush(-1);
    #endif
	    renderer_end_frame();
	}
#ifndef _PSX
	debug_layer_close();
#endif
    return 0;
}

void init(void) {
	// todo: make this more platform indepentend
#ifdef _PSX
	// Init CD
	SpuInit();
	CdInit();

	// Load the internal font texture
	FntLoad(608, 448);
	// Create the text stream
	FntOpen(0, 8, 320, 224, 0, 100);

#endif
}