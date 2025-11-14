#include "../collision.h"
#include "../vec3.h"

int test_vertical_capsule_aabb_intersect(void) {
    rayhit_t hit = {0};
    aabb_t aabb = {
        .min = vec3_from_floats(-1.0f, -1.0f, -1.0f),
        .max = vec3_from_floats(+1.0f, +1.0f, +1.0f),
    };
    
    int n_errors_fast = 0;
    int n_errors_fancy = 0;

    struct {
        char* description;
        vec3_t bottom;
        int expected_result;
    } capsule_bottoms[] = {
        {"bottom and top are both inside aabb",       vec3_from_floats(0.0f, -0.5f, 0.0f),   1},
        {"only bottom is inside aabb",                vec3_from_floats(0.0f, +0.5f, 0.0f),   1},
        {"only top is inside aabb",                   vec3_from_floats(0.0f, -1.5f, 0.0f),   1},
        {"capsule touches on -X",                     vec3_from_floats(-1.0f, -0.5f, 0.0f),  1},
        {"capsule touches on +X",                     vec3_from_floats(1.0f, -0.5f, 0.0f),   1},
        {"capsule touches on -Z",                     vec3_from_floats(0.0f, -0.5f, -1.0f),  1},
        {"capsule touches on +Z",                     vec3_from_floats(0.0f, -0.5f, +1.0f),  1},
        {"capsule touches on +X+Z",                   vec3_from_floats(+1.0f, -0.5f, +1.0f), 1},
        {"capsule touches on +X-Z",                   vec3_from_floats(+1.0f, -0.5f, -1.0f), 1},
        {"capsule touches on -X+Z",                   vec3_from_floats(-1.0f, -0.5f, +1.0f), 1},
        {"capsule touches on -X-Z",                   vec3_from_floats(-1.0f, -0.5f, -1.0f), 1},
        {"capsule is fully below aabb",               vec3_from_floats(0.0f, -2.5f, 0.0f),   0},
        {"capsule is fully above aabb",               vec3_from_floats(0.0f, +1.5f, 0.0f),   0},
        {"capsule is fully to the left of aabb",      vec3_from_floats(-2.0f, -0.5f, 0.0f),  0},
        {"capsule is fully to the right of aabb",     vec3_from_floats(+2.0f, -0.5f, 0.0f),  0},
        {"capsule is fully to the front of aabb",     vec3_from_floats(0.0f, -0.5f, +2.0f),  0},
        {"capsule is fully to the back of aabb",      vec3_from_floats(0.0f, -0.5f, -2.0f),  0},
        {"capsule is fully front left of aabb",       vec3_from_floats(-2.0f, -0.5f, +2.0f), 0},
        {"capsule is fully front right of aabb",      vec3_from_floats(+2.0f, -0.5f, +2.0f), 0},
        {"capsule is fully back left of aabb",        vec3_from_floats(-2.0f, -0.5f, -2.0f), 0},
        {"capsule is fully back right of aabb",       vec3_from_floats(+2.0f, -0.5f, -2.0f), 0},
    };

    for (size_t i = 0; i < sizeof(capsule_bottoms) / sizeof(capsule_bottoms[0]); ++i) {
        vertical_capsule_t capsule = {
            .bottom = capsule_bottoms[i].bottom,
            .height = ONE,
            .radius = ONE/2,
        };
        const int result_fast = !!vertical_capsule_aabb_intersect(&aabb, capsule);
        const int result_fancy = !!vertical_capsule_aabb_intersect_fancy(&aabb, capsule, &hit);
        if (result_fast != capsule_bottoms[i].expected_result) {
            printf("UNIT TEST FAILED: \"%s (fast)\": expected result %i, got %i\n", capsule_bottoms[i].description, capsule_bottoms[i].expected_result, result_fast);
            ++n_errors_fast;
        }
        if (result_fancy != capsule_bottoms[i].expected_result) {
            printf("UNIT TEST FAILED: \"%s (fancy)\": expected result %i, got %i\n", capsule_bottoms[i].description, capsule_bottoms[i].expected_result, result_fancy);
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
