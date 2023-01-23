#include "main.h"
#include "input.h"
#include "renderer.h"
#include "music.h"
#include <psxspu.h>
#include <psxsio.h>
#include <psxcd.h>
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
    Mesh* m_cube = mesh_load("\\CUBETEST.MSH;1");
    Mesh* m_level = mesh_load("\\LEVEL.MSH;1");
    Transform t_cube1 = { 
        {0, 0, 500},
        {-2048, 0, 0},
        {0, 0, 0}
    };
    Transform t_cube2 = { 
        {0, 0, -500},
        {-2048, 0, 0},
        {0, 0, 0}
    };
    Transform t_cube3 = { 
        {500, 0, 0},
        {-2048, 0, 0},
        {0, 0, 0}
    };
    Transform t_cube4 = { 
        {-500, 0, 0},
        {-2048, 0, 0},
        {0, 0, 0}
    };
    Transform t_level = { 
        {0, 0, 0},
        {-2048, 0, 0},
        {0, 0, 0}
    };

    // Play music
    music_play_file("\\MUSIC.XA");

    int delta_time = 300;
    while (1)
    {
        renderer_begin_frame(&camera_transform);
        input_update();
        if (input_is_connected(0))
        {
			// For dual-analog and dual-shock (analog input)
			if(input_has_analog(0)) {
				
				// Moving forwards and backwards
                camera_transform.position.vx -= ((( hisin( camera_transform.rotation.vy )*hicos( camera_transform.rotation.vx ) )>>12)*(input_left_stick_y(0)))>>2;
                camera_transform.position.vy += (hisin( camera_transform.rotation.vx )*(input_left_stick_y(0)))>>2;
                camera_transform.position.vz += ((( hicos( camera_transform.rotation.vy )*hicos( camera_transform.rotation.vx ) )>>12)*(input_left_stick_y(0)))>>2;
				
				// Strafing left and right
                camera_transform.position.vx -= (hicos( camera_transform.rotation.vy )*(input_left_stick_x(0)))>>2;
                camera_transform.position.vz -= (hisin( camera_transform.rotation.vy )*(input_left_stick_x(0)))>>2;
				
				// Look up and down
                camera_transform.rotation.vx += (input_right_stick_y(0)) << 3;
				
				// Look left and right
                camera_transform.rotation.vy -= (input_right_stick_x(0)) << 3;
            }
            if(input_held(PAD_R1, 0)) {
                // Slide up
                camera_transform.position.vx -= (( hisin( camera_transform.rotation.vy )*hisin( camera_transform.rotation.vx ) ))>>2;
                camera_transform.position.vy -= hicos( camera_transform.rotation.vx )>>2;
                camera_transform.position.vz += (( hicos( camera_transform.rotation.vy )*hisin( camera_transform.rotation.vx ) ))>>2;
            }
            
            if(input_held(PAD_R2, 0)) {
                
                // Slide down
                camera_transform.position.vx += (( hisin( camera_transform.rotation.vy )*hisin( camera_transform.rotation.vx ) ))>>2;
                camera_transform.position.vy += hicos( camera_transform.rotation.vx )>>2;
                camera_transform.position.vz -= (( hicos( camera_transform.rotation.vy )*hisin( camera_transform.rotation.vx ) ))>>2;
            }
        }
        //renderer_draw_triangles_shaded_2d(triangle, 3, x, -20);
        //x = (x + 1) % 120;
        //camera.rotation.vy += 12;
        t_cube1.rotation.vy += 1536;
        t_cube2.rotation.vy += 1536;
        t_cube3.rotation.vy += 1536;
        t_cube4.rotation.vy += 1536;
        //camera_transform.rotation.vy += 12;
        //renderer_draw_mesh_shaded(m_cube, t_cube1);
        //renderer_draw_mesh_shaded(m_cube, t_cube2);
        //renderer_draw_mesh_shaded(m_cube, t_cube3);
        //renderer_draw_mesh_shaded(m_cube, t_cube4);
        renderer_draw_mesh_shaded(m_level, t_level);
        //FntPrint(-1, "Frame: %i\n", frame_index++);
        //FntPrint(-1, "Delta Time (hblank): %i\n", delta_time);
        //FntPrint(-1, "Est. FPS: %i\n", 15625/delta_time);
        //FntPrint(-1, "R1 button: %i\n", input_held(PAD_R1, 0));
        //FntPrint(-1, "R2 button: %i\n", input_held(PAD_R2, 0));
        FntPrint(-1, "Triangles (rendered/total): %i / %i\n", renderer_get_n_rendered_triangles(), renderer_get_n_total_triangles());
        FntFlush(-1);
        renderer_end_frame();
        //delta_time = renderer_get_delta_time_raw();
    }
    return 0;
}

void init()
{
    // Init CD
	SpuInit();
    CdInit();

    // Load the internal font texture
    FntLoad(960, 0);
    // Create the text stream
    FntOpen(0, 8, 320, 224, 0, 100);

    // todo: textures
}