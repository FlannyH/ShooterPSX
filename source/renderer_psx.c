#ifdef _PSX
#include "renderer.h"
#include <stdio.h>
#include <psxgte.h>	// GTE header, not really used but libgpu.h depends on it
#include <psxgpu.h>	// GPU library header
#include <psxetc.h>	// Includes some functions that controls the display
#include <inline_c.h>

// Define environment pairs and buffer counter
DISPENV disp[2];
DRAWENV draw[2];
int drawbuffer;
int delta_time_raw_curr = 0;
int delta_time_raw_prev = 0;
int n_total_triangles = 0;
int n_rendered_triangles = 0;

u_long ord_tbl[2][ORD_TBL_LENGTH];
char primitive_buffer[2][32768 << 3];
char* next_primitive;
MATRIX view_matrix;
RECT	screen_clip;

int frame_counter = 0;

void renderer_init() {
    // Reset GPU and enable interrupts
    ResetGraph(0);

    // Configures the pair of DISPENVs
    SetDefDispEnv(&disp[0], 0, 0, RES_X, RES_Y);
    SetDefDispEnv(&disp[1], 0, RES_Y, RES_X, RES_Y);

    // Screen offset to center the picture vertically
#ifdef PAL
    disp[0].screen.y = 24;
    disp[1].screen.y = 24;
#endif

    // Forces PAL video standard
#ifdef PAL
    SetVideoMode(MODE_PAL);
#else
    SetVideoMode(MODE_NTSC);
#endif

    // Configures the pair of DRAWENVs for the DISPENVs
    SetDefDrawEnv(&draw[0], 0, RES_Y, RES_X, RES_Y);
    SetDefDrawEnv(&draw[1], 0, 0, RES_X, RES_Y);
    
    // Specifies the clear color of the DRAWENV
    setRGB0(&draw[0], 63, 0, 127);
    setRGB0(&draw[1], 63, 0, 127);

    // Enable background clear
    draw[0].isbg = 1;
    draw[1].isbg = 1;

    //Set to drawbuffer 0
    drawbuffer = 0;
    next_primitive = primitive_buffer[0];

    // Clear ordering tables
    ClearOTagR(ord_tbl[0], ORD_TBL_LENGTH);
    ClearOTagR(ord_tbl[1], ORD_TBL_LENGTH);

    // Initialize GTE
    InitGeom();

    // Set up where we want the center of the screen to be
    gte_SetGeomOffset(CENTER_X, CENTER_Y);

    // Set screen depth (which according to example code is kinda like FOV apparently)
    gte_SetGeomScreen(CENTER_X);

    // Set screen side clip
	setRECT( &screen_clip, 0, 0, RES_X, RES_Y );
    next_primitive = primitive_buffer[0];
}

#define CLIP_LEFT	1
#define CLIP_RIGHT	2
#define CLIP_TOP	4
#define CLIP_BOTTOM	8

int test_clip(RECT *clip, short x, short y) {
	
	// Tests which corners of the screen a point lies outside of
	
	int result = 0;

	if ( x < clip->x ) {
		result |= CLIP_LEFT;
	}
	
	if ( x >= (clip->x+(clip->w-1)) ) {
		result |= CLIP_RIGHT;
	}
	
	if ( y < clip->y ) {
		result |= CLIP_TOP;
	}
	
	if ( y >= (clip->y+(clip->h-1)) ) {
		result |= CLIP_BOTTOM;
	}

	return result;
	
}

int tri_clip(RECT *clip, DVECTOR *v0, DVECTOR *v1, DVECTOR *v2) {
	
	// Returns non-zero if a triangle is outside the screen boundaries
	
	short c[3];

	c[0] = test_clip(clip, v0->vx, v0->vy);
	c[1] = test_clip(clip, v1->vx, v1->vy);
	c[2] = test_clip(clip, v2->vx, v2->vy);

	if ( ( c[0] & c[1] ) == 0 )
		return 0;
	if ( ( c[1] & c[2] ) == 0 )
		return 0;
	if ( ( c[2] & c[0] ) == 0 )
		return 0;

	return 1;
}

void renderer_begin_frame(Transform* camera_transform) {
    // Apply camera transform
    HiRotMatrix(&camera_transform->rotation, &view_matrix);
    VECTOR position = camera_transform->position;
    position.vx >>= 12;
    position.vy >>= 12;
    position.vz >>= 12;
    ApplyMatrixLV(&view_matrix, &position, &position);
    TransMatrix(&view_matrix, &position);
    gte_SetRotMatrix(&view_matrix);
    gte_SetTransMatrix(&view_matrix);
    
    n_rendered_triangles = 0;
    n_total_triangles = 0;
}

void renderer_draw_mesh_shaded(Mesh* mesh, Transform model_transform) {
    if (!mesh) {
        printf("renderer_draw_mesh_shaded: mesh was null!\n");
        return;
    }

    // Set rotation and translation matrix
    MATRIX model_matrix;
    HiRotMatrix(&model_transform.rotation, &model_matrix);
    TransMatrix(&model_matrix, &model_transform.position);
    CompMatrixLV(&view_matrix, &model_matrix, &model_matrix);

    // Send it to the GTE
	PushMatrix();
    gte_SetRotMatrix(&model_matrix);
    gte_SetTransMatrix(&model_matrix);

    n_total_triangles += mesh->n_vertices / 3;

    // Loop over each triangle
    for (size_t i = 0; i < mesh->n_vertices; i += 3) {
        // Load a triangle's vertices into the GTE
        gte_ldv3(
            &mesh->vertices[i + 0].x,
            &mesh->vertices[i + 1].x,
            &mesh->vertices[i + 2].x
        );

        // Apply transformations
        gte_rtpt();

        // Calculate normal clip for backface culling
        int p;
        gte_nclip();
        gte_stopz(&p);

        // If this is the back of the triangle, cull it
        if (p <= 0) 
            continue;

        // Calculate average depth of the triangle
        gte_avsz3();
        gte_stotz(&p);

        // Depth clipping
        if ((p>>2) > ORD_TBL_LENGTH || ((p >> 2) <= 0))
            continue;

        // Create primitive
        POLY_G3* new_triangle = (POLY_G3*)next_primitive;
        next_primitive += sizeof(POLY_G3);

        // Initialize the entry in the render queue
        setPolyG3(new_triangle);

        // Set the vertex positions of the triangle
        gte_stsxy0( &new_triangle->x0 );
        gte_stsxy1( &new_triangle->x1 );
        gte_stsxy2( &new_triangle->x2 );
        if( tri_clip( &screen_clip, 
            (DVECTOR*)&new_triangle->x0, (DVECTOR*)&new_triangle->x1, (DVECTOR*)&new_triangle->x2) )
                continue;

        // Set the vertex colors of the triangle
        setRGB0(new_triangle,
            mesh->vertices[i + 0].r,
            mesh->vertices[i + 0].g,
            mesh->vertices[i + 0].b
        );
        setRGB1(new_triangle,
            mesh->vertices[i + 1].r,
            mesh->vertices[i + 1].g,
            mesh->vertices[i + 1].b
        );
        setRGB2(new_triangle,
            mesh->vertices[i + 2].r,
            mesh->vertices[i + 2].g,
            mesh->vertices[i + 2].b
        );

        // Add the triangle to the draw queue
        addPrim(ord_tbl[drawbuffer] + (p>>2), new_triangle);
        ++n_rendered_triangles;
    }
	PopMatrix();
}

void renderer_draw_triangles_shaded_2d(Vertex2D* vertex_buffer, uint16_t n_verts, int16_t x, int16_t y) {
    // Loop over each triangle
    for (size_t i = 0; i < n_verts; i += 3) {
        // Allocate a triangle in the render queue
        POLY_G3* new_triangle = (POLY_G3*)next_primitive;
        next_primitive += sizeof(POLY_G3);

        // Initialize the entry in the render queue
        setPolyG3(new_triangle);

        // Set the vertex positions of the triangle
        setXY3(new_triangle, 
            vertex_buffer[i + 0].x + x, vertex_buffer[i + 0].y + y,
            vertex_buffer[i + 1].x + x, vertex_buffer[i + 1].y + y,
            vertex_buffer[i + 2].x + x, vertex_buffer[i + 2].y + y
        );

        // Set the vertex colors of the triangle
        setRGB0(new_triangle,
            vertex_buffer[i + 0].r,
            vertex_buffer[i + 0].g,
            vertex_buffer[i + 0].b
        );
        setRGB1(new_triangle,
            vertex_buffer[i + 1].r,
            vertex_buffer[i + 1].g,
            vertex_buffer[i + 1].b
        );
        setRGB2(new_triangle,
            vertex_buffer[i + 2].r,
            vertex_buffer[i + 2].g,
            vertex_buffer[i + 2].b
        );

        // Add the triangle to the draw queue
        addPrim(ord_tbl[drawbuffer], new_triangle);
    }
}

void renderer_end_frame() {
    // Wait for GPU to finish drawing and V-blank
    DrawSync(0);
    //while (((VSync(-1) - frame_counter) & 0x3FFF) < 2)
    //    VSync(0);
    //frame_counter = VSync(-1);

    // Flip buffer counter
    drawbuffer = !drawbuffer;

    // Set the next primitive to draw to be the first primitive in the buffer
    next_primitive = primitive_buffer[drawbuffer];

    // Clear render queue
    ClearOTagR(ord_tbl[drawbuffer], ORD_TBL_LENGTH);
    
    // Apply Envs
    PutDispEnv(&disp[drawbuffer]);
    PutDrawEnv(&draw[drawbuffer]);
    
    // Enable display
    SetDispMask(1);
    
    // Draw Ordering Table
    DrawOTag(ord_tbl[1-drawbuffer] + ORD_TBL_LENGTH - 1);
}

int renderer_get_delta_time_raw() {
    delta_time_raw_prev = delta_time_raw_curr;
    delta_time_raw_curr = VSync(-1);
    return (delta_time_raw_curr - delta_time_raw_prev) & 0x7FFF;
}

int renderer_get_n_rendered_triangles() {
    return n_rendered_triangles;
}

int renderer_get_n_total_triangles() {
    return n_total_triangles;
}
#endif