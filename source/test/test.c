#include "../collision.h"
#include "../vec3.h"

int test_vertical_capsule_aabb_intersect(void) {
    aabb_t aabb = {
        .min = vec3_from_floats(-1.0f, -1.0f, -1.0f),
        .max = vec3_from_floats(+1.0f, +1.0f, +1.0f),
    };
    
    int n_errors_fast = 0;
    int n_errors_fancy = 0;

    struct {
        char* description;
        vec3_t bottom;
        int expected_result_fast;
        int expected_result_fancy;
    } capsule_bottoms[] = {
        {"capsule entirely inside aabb",       vec3_from_floats(0.0f, -0.5f, 0.0f),   1, 1},
        {"capsule bottom inside aabb",         vec3_from_floats(0.0f, +0.5f, 0.0f),   1, 1},
        {"capsule top inside aabb",            vec3_from_floats(0.0f, -1.5f, 0.0f),   1, 1},
        {"capsule touches on -X",              vec3_from_floats(-1.0f, -0.5f, 0.0f),  1, 1},
        {"capsule touches on +X",              vec3_from_floats(1.0f, -0.5f, 0.0f),   1, 1},
        {"capsule touches on -Z",              vec3_from_floats(0.0f, -0.5f, -1.0f),  1, 1},
        {"capsule touches on +Z",              vec3_from_floats(0.0f, -0.5f, +1.0f),  1, 1},
        {"capsule touches on +X+Z",            vec3_from_floats(+1.0f, -0.5f, +1.0f), 1, 1},
        {"capsule touches on +X-Z",            vec3_from_floats(+1.0f, -0.5f, -1.0f), 1, 1},
        {"capsule touches on -X+Z",            vec3_from_floats(-1.0f, -0.5f, +1.0f), 1, 1},
        {"capsule touches on -X-Z",            vec3_from_floats(-1.0f, -0.5f, -1.0f), 1, 1},
        {"capsule barely touching on -Y",      vec3_from_floats(0.0f, -2.49975f, 0.0f), 1, 1},
        {"capsule barely touching on +Y",      vec3_from_floats(0.0f, +1.49975f, 0.0f), 1, 1},
        {"capsule almost touching on -Y",      vec3_from_floats(0.0f, -2.50025f, 0.0f), 0, 0},
        {"capsule almost touching on +Y",      vec3_from_floats(0.0f, +1.50025f, 0.0f), 0, 0},
        {"capsule aabb corner +++",            vec3_from_floats(+1.2890f, +1.2890f, +1.2890f), 1, 0},
        {"capsule aabb corner ++-",            vec3_from_floats(+1.2890f, +1.2890f, -1.2890f), 1, 0},
        {"capsule aabb corner +-+",            vec3_from_floats(+1.2890f, -2.2890f, +1.2890f), 1, 0},
        {"capsule aabb corner +--",            vec3_from_floats(+1.2890f, -2.2890f, -1.2890f), 1, 0},
        {"capsule aabb corner -++",            vec3_from_floats(-1.2890f, +1.2890f, +1.2890f), 1, 0},
        {"capsule aabb corner -+-",            vec3_from_floats(-1.2890f, +1.2890f, -1.2890f), 1, 0},
        {"capsule aabb corner --+",            vec3_from_floats(-1.2890f, -2.2890f, +1.2890f), 1, 0},
        {"capsule aabb corner ---",            vec3_from_floats(-1.2890f, -2.2890f, -1.2890f), 1, 0},
        {"capsule aabb corner +++",            vec3_from_floats(+1.2885f, +1.2885f, +1.2885f), 1, 1},
        {"capsule aabb corner ++-",            vec3_from_floats(+1.2885f, +1.2885f, -1.2885f), 1, 1},
        {"capsule aabb corner +-+",            vec3_from_floats(+1.2885f, -2.2885f, +1.2885f), 1, 1},
        {"capsule aabb corner +--",            vec3_from_floats(+1.2885f, -2.2885f, -1.2885f), 1, 1},
        {"capsule aabb corner -++",            vec3_from_floats(-1.2885f, +1.2885f, +1.2885f), 1, 1},
        {"capsule aabb corner -+-",            vec3_from_floats(-1.2885f, +1.2885f, -1.2885f), 1, 1},
        {"capsule aabb corner --+",            vec3_from_floats(-1.2885f, -2.2885f, +1.2885f), 1, 1},
        {"capsule aabb corner ---",            vec3_from_floats(-1.2885f, -2.2885f, -1.2885f), 1, 1},
        {"capsule fully below aabb",           vec3_from_floats(0.0f, -3.0f, 0.0f),   0, 0},
        {"capsule fully above aabb",           vec3_from_floats(0.0f, +2.0f, 0.0f),   0, 0},
        {"capsule fully to the left of aabb",  vec3_from_floats(-2.0f, -0.5f, 0.0f),  0, 0},
        {"capsule fully to the right of aabb", vec3_from_floats(+2.0f, -0.5f, 0.0f),  0, 0},
        {"capsule fully to the front of aabb", vec3_from_floats(0.0f, -0.5f, +2.0f),  0, 0},
        {"capsule fully to the back of aabb",  vec3_from_floats(0.0f, -0.5f, -2.0f),  0, 0},
        {"capsule fully front left of aabb",   vec3_from_floats(-2.0f, -0.5f, +2.0f), 0, 0},
        {"capsule fully front right of aabb",  vec3_from_floats(+2.0f, -0.5f, +2.0f), 0, 0},
        {"capsule fully back left of aabb",    vec3_from_floats(-2.0f, -0.5f, -2.0f), 0, 0},
        {"capsule fully back right of aabb",   vec3_from_floats(+2.0f, -0.5f, -2.0f), 0, 0},
    };

    for (size_t i = 0; i < sizeof(capsule_bottoms) / sizeof(capsule_bottoms[0]); ++i) {
        rayhit_t hit = {0};

        vertical_capsule_t capsule = {
            .bottom = capsule_bottoms[i].bottom,
            .height = ONE,
            .radius = ONE/2,
        };

        const int result_fast = !!vertical_capsule_aabb_intersect(&aabb, capsule);
        const int result_fancy = !!vertical_capsule_aabb_intersect_fancy(&aabb, capsule, &hit);
        
        if (result_fast != capsule_bottoms[i].expected_result_fast) {
            printf("UNIT TEST FAILED: \"%s (fast)\": expected result %i, got %i\n", capsule_bottoms[i].description, capsule_bottoms[i].expected_result_fast, result_fast);
            ++n_errors_fast;
        }
        if (result_fancy != capsule_bottoms[i].expected_result_fancy) {
            printf("UNIT TEST FAILED: \"%s (fancy)\": expected result %i, got %i\n", capsule_bottoms[i].description, capsule_bottoms[i].expected_result_fancy, result_fancy);
            ++n_errors_fancy;
        }
    }

    return n_errors_fancy + n_errors_fast;
}

int test_vertical_capsule_triangle_intersect(void) {
    collision_triangle_3d_t triangles[] = {
        (collision_triangle_3d_t){ // facing up
            vec3_from_floats(0.0f, 0.0f, 0.0f), 
            vec3_from_floats(-1.5f, 0.0f, 2.0f), 
            vec3_from_floats(-2.0f, 0.0f, 0.0f), 
            vec3_from_floats(0.0f, 1.0f, 0.0f)
        },
    };
    float v0x = 0.0f; float v0y = 0.0f;
    float v1x = -1.5f; float v1y = 2.0f;
    float v2x = -2.0f; float v2y = 0.0f;

    struct {
        collision_triangle_3d_t* triangle;
        char* description;
        vec3_t bottom;
        int expected_result;
    } tests[] = {
        // triangle facing up - corner
        {&triangles[0], "t0 v0 miss bottom",                      vec3_from_floats(0.0f, 0.3f, 0.0f), 0},
        {&triangles[0], "t0 v0 barely miss bottom",               vec3_from_floats(0.0f, 0.2505f, 0.0f), 0},
        {&triangles[0], "t0 v0 hit radius bottom",                vec3_from_floats(0.0f, 0.125f, 0.0f), 1},
        {&triangles[0], "t0 v0 barely hit segment bottom",        vec3_from_floats(0.0f, 0.0f, 0.0f), 1},
        {&triangles[0], "t0 v0 hit center",                       vec3_from_floats(0.0f, -0.25f, 0.0f), 1},
        {&triangles[0], "t0 v0 barely hit segment top",           vec3_from_floats(0.0f, -0.5f, 0.0f), 1},
        {&triangles[0], "t0 v0 hit radius top",                   vec3_from_floats(0.0f, -0.625f, 0.0f), 1},
        {&triangles[0], "t0 v0 barely miss top",                  vec3_from_floats(0.0f, -0.7505f, 0.0f), 0},
        {&triangles[0], "t0 v0 miss top",                         vec3_from_floats(0.0f, -1.0000f, 0.0f), 0},
        {&triangles[0], "t0 v1 barely miss bottom",               vec3_from_floats(-1.5f, 0.2505f, 2.0f), 0},
        {&triangles[0], "t0 v1 hit center",                       vec3_from_floats(-1.5f, -0.25f, 2.0f), 1},
        {&triangles[0], "t0 v1 barely miss top",                  vec3_from_floats(-1.5f, -0.7505f, 2.0f), 0},
        {&triangles[0], "t0 v2 barely miss bottom",               vec3_from_floats(-2.0f, 0.2505f, 0.0f), 0},
        {&triangles[0], "t0 v2 hit center",                       vec3_from_floats(-2.0f, -0.25f, 0.0f), 1},
        {&triangles[0], "t0 v2 barely miss top",                  vec3_from_floats(-2.0f, -0.7505f, 0.0f), 0},
        {&triangles[0], "t0 v2 barely miss top",                  vec3_from_floats(-2.0f, -0.7505f, 0.0f), 0},

        // triangle facing up - edge vertical
        {&triangles[0], "t0 edge 0-1 miss bottom",                vec3_from_floats(-0.75f, 0.2505f, 1.0f), 0},
        {&triangles[0], "t0 edge 0-1 hit bottom",                 vec3_from_floats(-0.75f, 0.2495f, 1.0f), 1},
        {&triangles[0], "t0 edge 1-2 miss bottom",                vec3_from_floats(-1.75f, 0.2505f, 1.0f), 0},
        {&triangles[0], "t0 edge 1-2 hit bottom",                 vec3_from_floats(-1.75f, 0.2495f, 1.0f), 1},
        {&triangles[0], "t0 edge 2-0 miss bottom",                vec3_from_floats(1.0f, 0.2505f, 0.0f), 0},
        {&triangles[0], "t0 edge 2-0 hit bottom",                 vec3_from_floats(1.0f, 0.2495f, 0.0f), 1},
        {&triangles[0], "t0 edge 0-1 miss bottom",                vec3_from_floats(-0.75f, 0.2505f, 1.0f), 0},
        {&triangles[0], "t0 edge 0-1 hit bottom",                 vec3_from_floats(-0.75f, 0.2495f, 1.0f), 1},
        {&triangles[0], "t0 edge 1-2 miss bottom",                vec3_from_floats(-1.75f, 0.2505f, 1.0f), 0},
        {&triangles[0], "t0 edge 1-2 hit bottom",                 vec3_from_floats(-1.75f, 0.2495f, 1.0f), 1},
        {&triangles[0], "t0 edge 2-0 miss bottom",                vec3_from_floats(1.0f, 0.2505f, 0.0f), 0},
        {&triangles[0], "t0 edge 2-0 hit bottom",                 vec3_from_floats(1.0f, 0.2495f, 0.0f), 1           },

        // triangle facing up - edge sideways
        {&triangles[0], "t0 edge 0-1 miss sideways",              vec3_from_floats(-0.5725f, -0.25f, 1.165f), 0},
        {&triangles[0], "t0 edge 0-1 hit sideways",               vec3_from_floats(-0.5525f, -0.25f, 1.165f), 1},
        {&triangles[0], "t0 edge 1-2 miss sideways",              vec3_from_floats(-1.98861f, -0.25f, 1.14448f), 0},
        {&triangles[0], "t0 edge 1-2 hit sideways",               vec3_from_floats(-1.96953f, -0.25f, 1.14486f), 1},
        {&triangles[0], "t0 edge 2-1 miss sideways",              vec3_from_floats(-1.08621f, -0.25f, -0.248892f), 0},
        {&triangles[0], "t0 edge 2-1 hit sideways",               vec3_from_floats(-1.08621f, -0.25f, -0.251354f), 1},

        // triangle facing up - corners sideways
        {&triangles[0], "t0 corner v0 hit",                       vec3_from_floats(0.165f, -0.25f, -0.165f), 1},
        {&triangles[0], "t0 corner v0 miss",                      vec3_from_floats(0.175f, -0.25f, -0.185f), 0},
        {&triangles[0], "t0 corner v1 hit",                       vec3_from_floats(-1.5f, -0.25f, 2.249f), 1},
        {&triangles[0], "t0 corner v1 miss",                      vec3_from_floats(-1.5f, -0.25f, 2.251f), 0},
        {&triangles[0], "t0 corner v2 hit",                       vec3_from_floats(-2.16628f, -0.25f, -0.183405f), 1},
        {&triangles[0], "t0 corner v2 miss",                      vec3_from_floats(-2.16988f, -0.25f, -0.187204f), 0},

        // triangle facing up - center
        {&triangles[0], "t0 center miss bottom",               vec3_from_floats(-1.16666f, 0.3f, 0.66666f)},
        {&triangles[0], "t0 center barely miss bottom",        vec3_from_floats(-1.16666f, 0.2505f, 0.66666f)},
        {&triangles[0], "t0 center hit radius bottom",         vec3_from_floats(-1.16666f, 0.125f, 0.66666f)},
        {&triangles[0], "t0 center barely hit segment bottom", vec3_from_floats(-1.16666f, 0.0f, 0.66666f)},
        {&triangles[0], "t0 center hit center",                vec3_from_floats(-1.16666f, -0.25f, 0.66666f)},
        {&triangles[0], "t0 center barely hit segment top",    vec3_from_floats(-1.16666f, -0.5f, 0.66666f)},
        {&triangles[0], "t0 center hit radius top",            vec3_from_floats(-1.16666f, -0.625f, 0.66666f)},
        {&triangles[0], "t0 center barely miss top",           vec3_from_floats(-1.16666f, -0.7505f, 0.66666f)},
        {&triangles[0], "t0 center miss top",                  vec3_from_floats(-1.16666f, -1.0000f, 0.66666f)},
    };
}

int test(void) {
    int n_errors = 0;

    n_errors += test_vertical_capsule_aabb_intersect();
    n_errors += test_vertical_capsule_triangle_intersect();

    return n_errors;
}
