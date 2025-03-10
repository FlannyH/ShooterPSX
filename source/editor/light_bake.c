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

#pragma pack(push, 1)
typedef struct {
    uint16_t bf_type;      // Magic identifier: "BM" (0x4D42)
    uint32_t bf_size;      // Size of the file in bytes
    uint16_t bf_reserved1; // Reserved, must be zero
    uint16_t bf_reserved2; // Reserved, must be zero
    uint32_t bf_off_bits;  // Offset to start of pixel data
} bmp_file_header_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint32_t bi_size;          // Size of this header (40 bytes)
    int32_t  bi_width;         // Image width in pixels
    int32_t  bi_height;        // Image height in pixels
    uint16_t bi_planes;        // Number of color planes (must be 1)
    uint16_t bi_bit_count;     // Bits per pixel (e.g., 24)
    uint32_t bi_compression;   // Compression type (0 = uncompressed)
    uint32_t bi_size_image;    // Image size (may be 0 for uncompressed)
    int32_t  bi_x_pels_per_meter; // Horizontal resolution (pixels per meter)
    int32_t  bi_y_pels_per_meter; // Vertical resolution (pixels per meter)
    uint32_t bi_clr_used;         // Number of colors in the color palette
    uint32_t bi_clr_important;    // Important colors (0 = all)
} bmp_info_header_t;
#pragma pack(pop)

typedef struct  {
    bmp_file_header_t file_header;
    bmp_info_header_t info_header;
} bmp_header_t;

typedef struct {
    int16_t width;
    int16_t height;
    int16_t left;
    int16_t top;
} rect16_t;

typedef struct {
    int mesh_id;
    int first_vertex_id;
    bool is_quad; // false if triangle, true if quad
    bool is_allocated;
    bool in_extra_free_space;
    rect16_t rect;
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
    float lightmap_space_per_texel = 25.93775;
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
                .rect.width = u_pixels,
                .rect.height = v_pixels,
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
            u_pixels += 1; // extra padding since we use half a pixel on each side to
            v_pixels += 1; // mitigate filtering artifacts and seams between polygons
            lm_meta[lm_meta_cursor] = (lightmap_polygon_metadata_t) {
                .mesh_id = mesh_i,
                .first_vertex_id = (mesh->n_triangles * 3) + (quad_i * 4),
                .is_quad = false,
                .rect.width = u_pixels,
                .rect.height = v_pixels,
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
            int height_j = lm_meta[lm_meta_indices[j]].rect.height;
            int height_j1 = lm_meta[lm_meta_indices[j + 1]].rect.height;
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
    int curr_row_start_index = 0;

    for (int i = 0; i < n_polygons_total; ++i) {
        const int index = lm_meta_indices[i];

        if (lm_meta[index].is_allocated) continue;

        if (lm_meta[index].rect.width > lightmap_resolution) {
            printf("One of the polygons' width exceeds the lightmap resolution!");
            return 3;
        }

        if (lm_meta[index].rect.height > curr_row_height) {
            curr_row_height = lm_meta[index].rect.height;
        }
        
        lm_meta[index].rect.left = x_cursor;
        lm_meta[index].rect.top = y_cursor;
        lm_meta[index].is_allocated = true;
        lm_meta[index].in_extra_free_space = false;

        x_cursor += lm_meta[index].rect.width;

        if (x_cursor + lm_meta[index].rect.width >= lightmap_resolution) {
            // There's a good chance there's free space between allocated rows
            // Let's make the most of that space!
            for (int free_i = -1; free_i < n_polygons_total; ++free_i) {
                rect16_t free_rect = {};

                // The first rectangle we try to fill is the one at the right side of the
                // row, which isn't linked to any previously allocated rectangle on this row
                if (free_i != -1) {
                    if (lm_meta[lm_meta_indices[free_i]].is_allocated == false) continue;
                    if (lm_meta[lm_meta_indices[free_i]].in_extra_free_space == true) continue;
                    const rect16_t curr_rect = lm_meta[lm_meta_indices[free_i]].rect;
                    free_rect = (rect16_t) {
                        .width = curr_rect.width,
                        .height = curr_row_height - curr_rect.height,
                        .left = curr_rect.left,
                        .top = y_cursor + curr_rect.height,
                    };

                    // Stitch together multiple free rectangles with the same height
                    while (true) {
                        if (free_i >= n_polygons_total - 1) break;
                        if (lm_meta[lm_meta_indices[free_i + 1]].is_allocated == false) goto skip;
                        if (lm_meta[lm_meta_indices[free_i + 1]].in_extra_free_space == true) goto skip;
                        const rect16_t next_rect = lm_meta[lm_meta_indices[free_i+1]].rect;
                        if (next_rect.height != curr_rect.height) break;
                        free_rect.width += next_rect.width;
                    skip:
                        ++free_i;
                    }
                } else {
                    free_rect = (rect16_t) {
                        .width = lightmap_resolution - x_cursor,
                        .height = curr_row_height,
                        .left = x_cursor,
                        .top = y_cursor,
                    };
                }

                if (free_rect.height == 0) continue;

                // Keep filling up the free space with whatever subsequent candidates fit
                // best in that spot, until no more candidates fit
                while (free_rect.width > 0) {
                    int best_candidate_i = -1;
                    int best_candidate_width_error = INT32_MAX;
                    int best_candidate_height_error = INT32_MAX;

                    for (int small_i = 0; small_i < n_polygons_total; ++small_i) {
                        if (lm_meta[lm_meta_indices[small_i]].is_allocated) continue;
                        
                        const rect16_t small_rect = lm_meta[lm_meta_indices[small_i]].rect;
                        if (small_rect.height > free_rect.height) continue;
                        if (small_rect.width > free_rect.width) continue;
                        int height_error = (free_rect.height - small_rect.height);
                        int width_error = (free_rect.width - small_rect.width);

                        if (height_error < best_candidate_height_error) {
                            best_candidate_i = small_i;
                            best_candidate_height_error = height_error;
                        }
                        else if (height_error == best_candidate_height_error) {
                            if (width_error < best_candidate_width_error) {
                                best_candidate_i = small_i;
                                best_candidate_width_error = width_error;
                            }
                        }
                    }

                    if (best_candidate_i == -1) break;
                    lm_meta[lm_meta_indices[best_candidate_i]].rect.top = free_rect.top;
                    lm_meta[lm_meta_indices[best_candidate_i]].rect.left = free_rect.left;
                    lm_meta[lm_meta_indices[best_candidate_i]].is_allocated = true;
                    lm_meta[lm_meta_indices[best_candidate_i]].in_extra_free_space = true;
                    free_rect.left += lm_meta[lm_meta_indices[best_candidate_i]].rect.width;
                    free_rect.width -= lm_meta[lm_meta_indices[best_candidate_i]].rect.width;
                }
            }

            // Next row
            x_cursor = 0;
            y_cursor += curr_row_height;
            curr_row_height = 0;
            curr_row_start_index = i + 1;

            if (y_cursor >= lightmap_resolution) {
                printf("Ran out of lightmap space after %i / %i polygons!\n", i, n_polygons_total);
                return 4;
            }
        }
    }

    // Check for overlap
    for (int i = 0; i < n_polygons_total; ++i) {
        for (int j = 0; j < n_polygons_total; ++j) {
            if (i == j) continue;

            const rect16_t a = lm_meta[i].rect;
            const rect16_t b = lm_meta[j].rect;
            if (a.left < b.left + b.width && a.left + a.width > b.left &&
                a.top < b.top + b.height && a.top + a.height > b.top) {
                printf("error: overlap between polygon %d and %d\n", i, j);
                return 5;
            }
        }
    }

    // debug
    for (int i = 0; i < n_polygons_total; ++i) {
        const int index = lm_meta_indices[i];
        printf("(%4i, %4i) -> (%4i, %4i)\n", 
            lm_meta[index].rect.left, 
            lm_meta[index].rect.top, 
            lm_meta[index].rect.left + lm_meta[index].rect.width - 1, 
            lm_meta[index].rect.top + lm_meta[index].rect.height - 1
        );

        for (int y = lm_meta[index].rect.top; y < lm_meta[index].rect.top + lm_meta[index].rect.height; ++y) {
            for (int x = lm_meta[index].rect.left; x < lm_meta[index].rect.left + lm_meta[index].rect.width; ++x) {   
                lightmap[x + (y * lightmap_resolution)].b = (index) % 256;
                lightmap[x + (y * lightmap_resolution)].g = (index / 256) % 256;
                lightmap[x + (y * lightmap_resolution)].r += 1; // overflows to 0. if the red channel has any values above 0 we're fucked
                lightmap[x + (y * lightmap_resolution)].a = 255;
            }   
        }
    }

    bmp_header_t bmp_header = {0};
    bmp_header.file_header.bf_type = 0x4D42;
    bmp_header.file_header.bf_size = sizeof(bmp_header_t) + (lightmap_resolution * lightmap_resolution * sizeof(pixel32_t));
    bmp_header.file_header.bf_off_bits = sizeof(bmp_header_t);
    bmp_header.info_header.bi_size = 40;
    bmp_header.info_header.bi_width = lightmap_resolution;
    bmp_header.info_header.bi_height = lightmap_resolution;
    bmp_header.info_header.bi_planes = 1;
    bmp_header.info_header.bi_bit_count = 24;
    bmp_header.info_header.bi_compression = 0;
    bmp_header.info_header.bi_size_image = 0;

    FILE* image_debug = fopen("test.bmp", "wb");
    fwrite(&bmp_header, sizeof(bmp_header), 1, image_debug);

    for (int y = lightmap_resolution-1; y >= 0; --y) {
        for (int x = 0; x < lightmap_resolution; ++x) {
            fwrite(&lightmap[x + (y * lightmap_resolution)].b, 1, 1, image_debug);
            fwrite(&lightmap[x + (y * lightmap_resolution)].g, 1, 1, image_debug);
            fwrite(&lightmap[x + (y * lightmap_resolution)].r, 1, 1, image_debug);
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