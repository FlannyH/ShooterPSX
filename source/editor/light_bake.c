#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../structs.h"
#include "../mesh.h"
#include "../vec3.h"

int main(int argc, const char** argv) {
    if (argc < 2 || argc > 5) {
        printf("Usage: lightmap_generator.exe <path/to/mesh.msh> [lightmap resolution, default 1024] [store to vertex colors, default true] [lightmap texel density, default 32]\n");
        return 1;
    }

    const char* path = argv[1];
    int lightmap_resolution = 1024;
    int lightmap_texel_density = 16;
    bool store_to_vertex_colors = false;

    if (argc >= 3) {
        lightmap_resolution = strtol(argv[2], NULL, 10);
    }

    if (argc >= 4) {
        if (strcmp(argv[3], "true") == 0) { store_to_vertex_colors = true; }
        else if (strcmp(argv[3], "false") == 0) { store_to_vertex_colors = false; }
        else { printf("Invalid argument \"%s\", expected \"true\" or \"false\"\n", argv[3]); return 2; }
    }

    if (argc == 5) {
        lightmap_texel_density = strtol(argv[4], NULL, 10);
    }

    pixel32_t* lightmap = mem_alloc(lightmap_resolution * lightmap_resolution * sizeof(pixel32_t), MEM_CAT_TEXTURE);

    // todo: reimplement this
    model_t* model = model_load(path, 0, 0, 0, 0);

    // Allocate space in lightmap for each polygon
    for (int mesh_i = 0; mesh_i < model->n_meshes; ++mesh_i) {
        const mesh_t* const mesh = &model->meshes[mesh_i];

        // Allocate space in the lightmap for all triangles (they waste space but eh tough luck)
        for (int tri_i = 0; tri_i < mesh->n_triangles; ++tri_i) {
            const vertex_3d_t* const vtx0 = &mesh->vertices[tri_i + 0];
            const vertex_3d_t* const vtx1 = &mesh->vertices[tri_i + 1];
            const vertex_3d_t* const vtx2 = &mesh->vertices[tri_i + 2];
            const vec3_t pos0 = { vtx0->x * ONE, vtx0->y * ONE, vtx0->z * ONE };
            const vec3_t pos1 = { vtx1->x * ONE, vtx1->y * ONE, vtx1->z * ONE };
            const vec3_t pos2 = { vtx2->x * ONE, vtx2->y * ONE, vtx2->z * ONE };
            const vec3_t u_axis = vec3_sub(pos2, pos0);
            const vec3_t v_axis = vec3_sub(pos1, pos0);
            const scalar_t u_length = scalar_sqrt(vec3_magnitude_squared(u_axis));
            const scalar_t v_length = scalar_sqrt(vec3_magnitude_squared(v_axis));
            const int u_pixels = u_length / (ONE * lightmap_texel_density);
            const int v_pixels = v_length / (ONE * lightmap_texel_density);

            if (mesh_i == 0 && tri_i < 9) printf("%4i: %4i, %4i\n", tri_i, u_pixels, v_pixels);
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