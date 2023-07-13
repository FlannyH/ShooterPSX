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

int widescreen = 0;

int main(void) {
	renderer_init();
	input_init();
	init();

    // Init player
    player_t player = { 0 };

    player.position.x = 11705653 / 2;
   	player.position.y = 11413985 / 2;
    player.position.z = 2112866  / 2;
	player.rotation.y = 4096 * 16;
    //player.position.x = 0;
    //player.position.y = -229376;
    //player.position.z = 0;

	// Stick deadzone
	input_set_stick_deadzone(36);

	// Load model
    const model_t* m_level = model_load("\\ASSETS\\MODELS\\LEVEL.MSH;1");
    const model_t* m_level_col_dbg = model_load_collision_debug("\\ASSETS\\MODELS\\LEVEL.COL;1");
    const collision_mesh_t* m_level_col = model_load_collision("\\ASSETS\\MODELS\\LEVEL.COL;1");

	texture_cpu_t *tex_level;

	// todo: add unload functionality for when the textures are on the gpu. we don't need these in ram.
	const uint32_t n_textures = texture_collection_load("\\ASSETS\\MODELS\\LEVEL.TXC;1", &tex_level);

	for (uint8_t i = 0; i < n_textures; ++i) {
	    renderer_upload_texture(&tex_level[i], i);
	}

	music_load_soundbank("\\ASSETS\\MUSIC\\INSTR.SBK;1");
	music_load_sequence("\\ASSETS\\MUSIC\\SEQUENCE\\SUBNIVIS.DSS;1");
	music_play_sequence(0);

	transform_t t_level = {{0, 0, 0}, {0, 0, 0}, {4096, 4096, 4096}};
    
    bvh_t bvh_level_model;
    bvh_from_model(&bvh_level_model, m_level_col);

	int frame_counter = 0;
	int show_debug = 0;
    player_update(&player, &bvh_level_model, 16);
    while (!renderer_should_close()) {
#ifndef _WIN32
        int delta_time_raw = renderer_get_delta_time_raw();
        if (delta_time_raw > 520) {
            delta_time_raw = 520;
        }
        int delta_time = renderer_convert_dt_raw_to_ms(delta_time_raw);
#else
        int delta_time = renderer_get_delta_time_ms();
#endif
        frame_counter += delta_time;
		if (input_pressed(PAD_SELECT, 0)) show_debug = !show_debug;
		if (show_debug) {
			renderer_begin_frame(&player.transform);
			PROFILE("input", input_update(), 1);
			PROFILE("render", renderer_draw_model_shaded(m_level, &t_level), 1);
			PROFILE("player", player_update(&player, &bvh_level_model, delta_time), 1);
			PROFILE("music", music_tick(16), 1);
			FntPrint(-1, "dt: %i\n", delta_time);
			FntPrint(-1, "meshes drawn: %i / %i\n", n_meshes_drawn, n_meshes_total);
			FntFlush(-1);
			renderer_end_frame();
		}
		else {
			renderer_begin_frame(&player.transform);
			input_update();
			renderer_draw_model_shaded(m_level, &t_level);
			player_update(&player, &bvh_level_model, delta_time);
			music_tick(16);
			renderer_end_frame();
		}
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
	FntLoad(512, 256);
	// Create the text stream
	FntOpen(16, 16, 480, 224, 0, 512);

#endif
}