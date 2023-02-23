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

#include "texture.h"

int main(void) {
	renderer_init();
	input_init();
	init();

	uint32_t frame_index = 0;

	// Camera transform
	transform_t camera_transform;
	camera_transform.position.vx = -11558322;
	camera_transform.position.vy = 14423348;
	camera_transform.position.vz = 6231602;
	camera_transform.rotation.vx = 0;
	camera_transform.rotation.vy = 0;
	camera_transform.rotation.vz = 0;

	// Stick deadzone
	input_set_stick_deadzone(36);

	// Load model
	model_t *m_cube = model_load("\\ASSETS\\CUBETEST.MSH;1");
	const model_t *m_level = model_load("\\ASSETS\\LEVEL.MSH;1");

	texture_cpu_t *tex_level;
	const uint32_t n_textures =
		texture_collection_load("\\ASSETS\\LEVEL.TXC;1", &tex_level);

	for (uint8_t i = 0; i < n_textures; ++i) {
	renderer_upload_texture(&tex_level[i], i);
	}

	transform_t t_cube1 = {{0, 0, 500}, {-2048, 0, 0}, {0, 0, 0}};
	transform_t t_level = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    // Construct BVH for level
    bvh_t bvh_level[16];
    for (uint32_t x = 0; x < m_level->n_meshes; x++) {
        bvh_from_mesh(&bvh_level[x], &m_level->meshes[x]);
    }

	// Play music
	music_play_file("\\ASSETS\\MUSIC.XA;1");
    const pixel32_t white = { 255, 255, 255, 255 };

	int frame_counter = 0;
    int bvh_depth = 0;
    ray_t ray = {0};
    while (!renderer_should_close()) {
        const int delta_time = renderer_get_delta_time_ms();
        renderer_begin_frame(&camera_transform);
        input_update();
        camera_update_flycam(&camera_transform, delta_time);
        t_cube1.rotation.vy += 64 * delta_time;
        // renderer_draw_mesh_shaded(m_cube, t_cube1);
        renderer_draw_model_shaded(m_level, &t_level);
        //renderer_debug_draw_aabb(&m_level->meshes[0].bounds, white, &t_level);
        frame_counter += delta_time;
        rayhit_t hit = { 0 };
        for (int i = 0; i < m_level->n_meshes; ++i) {
            bvh_intersect(&bvh_level[i], ray, &hit);
        }
        vertex_3d_t start = { ray.position.x.raw, ray.position.y.raw, ray.position.z.raw, 255, 255, 0, 0, 0, 255 };
        vertex_3d_t end = { ray.position.x.raw + ray.direction.x.raw * 10, ray.position.y.raw + ray.direction.y.raw * 10, ray.position.z.raw + ray.direction.z.raw * 10, 255, 255, 0, 0, 0, 255 };
        line_3d_t line = { start, end };
        renderer_debug_draw_line(line, white, &t_level);
        if (input_pressed(PAD_CIRCLE, 0)) {
            bvh_depth = (bvh_depth+1) % 16;

            ray.position.x.raw = -camera_transform.position.vx >> 12;
            ray.position.y.raw = -camera_transform.position.vy >> 12;
            ray.position.z.raw = -camera_transform.position.vz >> 12;
            ray.direction = renderer_get_forward_vector();
            ray.direction.x.raw += ray.direction.x.raw == 0;
            ray.direction.y.raw += ray.direction.y.raw == 0;
            ray.direction.z.raw += ray.direction.z.raw == 0;
            ray.inv_direction = vec3_div(vec3_from_int16s(64, 64, 64), ray.direction);
            printf("%f, %f, %f\n",
                ((float)ray.direction.x.raw) / 256.0f,
                ((float)ray.direction.y.raw) / 256.0f,
                ((float)ray.direction.z.raw) / 256.0f
            );

            for (uint32_t x = 0; x < m_level->n_meshes; x++) {
                //uint32_t color =
                //    ((((x + 1) & 0x01) * 0xFF) << 0) |
                //    ((((x + 1) & 0x02) * 0xFF) << 7) |
                //    ((((x + 1) & 0x04) * 0xFF) << 14);
                //
                //bvh_debug_draw(&bvh_level[x], bvh_depth, bvh_depth, *(pixel32_t*)&color);
            }
        }
    #ifdef _PSX
	    FntPrint(-1, "Frame: %i\n", frame_index++);
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
}

void init(void) {
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