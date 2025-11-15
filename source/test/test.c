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

int test(void) {
    int n_errors = 0;

    n_errors += test_vertical_capsule_aabb_intersect();

    return n_errors;
}
