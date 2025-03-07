#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cglm/types.h>
#include <cglm/vec3.h>
#include "../structs.h"
#include "../mesh.h"
#include "../vec2.h"
#include "../vec3.h"

typedef struct {
    int mesh_id;
    int first_vertex_id;
    bool is_quad; // false if triangle, true if quad
    int16_t rect_width;
    int16_t rect_height;
    int16_t rect_left;
    int16_t rect_top;
} lightmap_polygon_metadata_t;

int main(int argc, const char** argv) {
    mem_init();

    if (argc < 2 || argc > 5) {
        printf("Usage: lightmap_generator.exe <path/to/mesh.msh> [lightmap resolution, default 1024] [store to vertex colors, default true] [lightmap space per texel, default 16]\n");
        // return 1;
    }

    // const char* path = argv[1];
    const char* path = "D:/Projects/Git/ShooterPSX/assets/shared/models/level.msh";
    int lightmap_resolution = 1024;
    float lightmap_space_per_texel = 32;
    bool store_to_vertex_colors = true;

    if (argc >= 3) {
        lightmap_resolution = strtol(argv[2], NULL, 10);
    }

    if (argc >= 4) {
        if (strcmp(argv[3], "true") == 0) { store_to_vertex_colors = true; }
        else if (strcmp(argv[3], "false") == 0) { store_to_vertex_colors = false; }
        else { printf("Invalid argument \"%s\", expected \"true\" or \"false\"\n", argv[3]); return 2; }
    }

    if (argc == 5) {
        lightmap_space_per_texel = strtof(argv[4], NULL);
    }

    pixel32_t* lightmap = mem_alloc(lightmap_resolution * lightmap_resolution * sizeof(pixel32_t), MEM_CAT_TEXTURE);
    memset(lightmap, 0xFF, lightmap_resolution * lightmap_resolution * sizeof(pixel32_t));
    const model_t* const model = model_load(path, 0, 0, 0, 0);

    // How many polygons do we have? We need to know this for some helper arrays
    int n_triangles_total = 0;
    int n_quads_total = 0;
    for (int mesh_i = 0; mesh_i < model->n_meshes; ++mesh_i) {
        n_triangles_total += model->meshes[mesh_i].n_triangles;
        n_quads_total += model->meshes[mesh_i].n_quads;
    }
    const int n_polygons_total = n_triangles_total + n_quads_total;

    // Figure out rectangle sizes in lightmap for each polygon
    lightmap_polygon_metadata_t* lm_meta = mem_alloc(n_polygons_total * sizeof(lightmap_polygon_metadata_t), MEM_CAT_UNDEFINED);
    size_t lm_meta_cursor = 0;

    for (int mesh_i = 0; mesh_i < model->n_meshes; ++mesh_i) {
        const mesh_t* const mesh = &model->meshes[mesh_i];
        const vertex_3d_t* const triangles = &mesh->vertices[0];
        const vertex_3d_t* const quads = &mesh->vertices[mesh->n_triangles * 3];

        // Allocate space in the lightmap for all triangles (they waste space but eh tough luck)
        // This is done by creating a UV coordinate space and projecting it onto a rectangle
        for (int tri_i = 0; tri_i < mesh->n_triangles; ++tri_i) {
            const vertex_3d_t* const vtx0 = &triangles[(tri_i * 3) + 0];
            const vertex_3d_t* const vtx1 = &triangles[(tri_i * 3) + 1];
            const vertex_3d_t* const vtx2 = &triangles[(tri_i * 3) + 2];
            const vec3 pos0 = { vtx0->x, vtx0->y, vtx0->z };
            const vec3 pos1 = { vtx1->x, vtx1->y, vtx1->z };
            const vec3 pos2 = { vtx2->x, vtx2->y, vtx2->z };
            vec3 u_axis; glm_vec3_sub(pos2, pos0, u_axis);
            vec3 v_axis; glm_vec3_sub(pos1, pos0, v_axis);
            const float u_length = glm_vec3_norm(u_axis);
            const float v_length = glm_vec3_norm(v_axis);
            int u_pixels = (u_length / (float)lightmap_space_per_texel);
            int v_pixels = (v_length / (float)lightmap_space_per_texel);
            if (u_pixels == 0) u_pixels = 1;
            if (v_pixels == 0) v_pixels = 1;
            lm_meta[lm_meta_cursor] = (lightmap_polygon_metadata_t) {
                .mesh_id = mesh_i,
                .first_vertex_id = tri_i * 3,
                .is_quad = false,
                .rect_width = u_pixels,
                .rect_height = v_pixels,
            };
            ++lm_meta_cursor;
        }

        // Allocate space in the lightmap for all quads
        // This is done by projecting its 4 points onto a rectangle. Might cause distortion 
        // for unusually shaped quads, but we should accept all shapes of quads.
        // The size of the rectangle is based on the 2 edges connected to vertex 0
        for (int quad_i = 0; quad_i < mesh->n_quads; ++quad_i) {
            const vertex_3d_t* const vtx0 = &quads[(quad_i * 4) + 0];
            const vertex_3d_t* const vtx1 = &quads[(quad_i * 4) + 1];
            const vertex_3d_t* const vtx2 = &quads[(quad_i * 4) + 2];
            const vertex_3d_t* const vtx3 = &quads[(quad_i * 4) + 3];
            const vec3 pos0 = { vtx0->x, vtx0->y, vtx0->z };
            const vec3 pos1 = { vtx1->x, vtx1->y, vtx1->z };
            const vec3 pos2 = { vtx2->x, vtx2->y, vtx2->z };
            const vec3 pos3 = { vtx3->x, vtx3->y, vtx3->z };
            vec3 u_axis; glm_vec3_sub(pos2, pos0, u_axis);
            vec3 v_axis; glm_vec3_sub(pos1, pos0, v_axis);
            const float u_length = glm_vec3_norm(u_axis);
            const float v_length = glm_vec3_norm(v_axis);
            int u_pixels = ceilf(u_length / (float)lightmap_space_per_texel);
            int v_pixels = ceilf(v_length / (float)lightmap_space_per_texel);
            if (u_pixels == 0) u_pixels = 1;
            if (v_pixels == 0) v_pixels = 1;
            lm_meta[lm_meta_cursor] = (lightmap_polygon_metadata_t) {
                .mesh_id = mesh_i,
                .first_vertex_id = (mesh->n_triangles * 3) + (quad_i * 4),
                .is_quad = false,
                .rect_width = u_pixels,
                .rect_height = v_pixels,
            };
            ++lm_meta_cursor;
        }
    }

    // Create index buffer
    int* lm_meta_indices = mem_alloc(n_polygons_total * sizeof(int), MEM_CAT_UNDEFINED);

    for (int i = 0; i < n_polygons_total; ++i) {
        lm_meta_indices[i] = i;
    }

    // Sort them based on height, this way we can get a tighter fit in the lightmap
    for (int i = 0; i < n_polygons_total - 1; ++i) {
        for (int j = 0; j < n_polygons_total - i - 1; ++j) {
            int height_j = lm_meta[lm_meta_indices[j]].rect_height;
            int height_j1 = lm_meta[lm_meta_indices[j + 1]].rect_height;
            if (height_j < height_j1) {
                int temp = lm_meta_indices[j];
                lm_meta_indices[j] = lm_meta_indices[j + 1];
                lm_meta_indices[j + 1] = temp;
            }
        }
    }

    // Place the polygon rectangles in the lightmap texture
    int x_cursor = 0;
    int y_cursor = 0;
    int curr_row_height = 0;

    for (int i = 0; i < n_polygons_total; ++i) {
        const int index = lm_meta_indices[i];

        if (lm_meta[index].rect_width > lightmap_resolution) {
            printf("One of the polygons' width exceeds the lightmap resolution!");
            return 3;
        }

        if (lm_meta[index].rect_height > curr_row_height) {
            curr_row_height = lm_meta[index].rect_height;
        }
        
        lm_meta[index].rect_left = x_cursor;
        lm_meta[index].rect_top = y_cursor;
        x_cursor += lm_meta[index].rect_width;

        if (x_cursor + lm_meta[index].rect_width >= lightmap_resolution) {
            x_cursor = 0;
            y_cursor += curr_row_height;
            curr_row_height = 0;

            if (y_cursor >= lightmap_resolution) {
                printf("Ran out of lightmap space after %i polygons!\n", i);
                return 4;
            }
        }
    }

}

/*
alloc:
- allocate texture for lightmap
- for each polygon:
  - allocate texture rectangle
  - store world positions for each pixel in the rectangle
  - store normals for each pixel in the rectangle

bake:
- for each pixel in lightmap:
  - if normal is 0, 0, 0, ignore this pixel
  - fetch world position
  - fetch normal
  - get ray position from world position
  - get ray direction from cosine weighted importance sampling based on normal
  - trace ray with n bounces
  ! first ray should be only diffuse, bounces can have specular though

store:
- create a new optional section in the fmsh file that stores lightmap uvs per vertex
- create a new optional section in the fmsh file that stores the lightmap texture, or a path to one
- for each vertex in the mesh:
  - calculate weighted average of all lightmap pixels that are closest to this vertex
  - store this in the vertex color field we already had (so ps1 and ds code can stay mostly identical)
*/