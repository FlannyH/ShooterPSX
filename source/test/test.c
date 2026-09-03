#include "../collision.h"
#include "../math/vec3.h"

// int test_scalar_math(void) {
//     int n_errors = 0;

//     typedef enum {
//         TEST_MUL = 2,
//         TEST_DIV = 3,
//         TEST_SIN = 4,
//         TEST_COS = 5,
//     } test_type_t;

//     struct {
//         char* description;
//         scalar_t a, b;
//         scalar_t expected_result;
//         test_type_t type;
//     } tests[] = {
//         // multiply
//         {"2.5 * 1.25", scalar_from_float(2.5f), scalar_from_float(1.25f), scalar_from_float(3.125f), TEST_MUL},
//         {"0.0 * 1.25", scalar_from_float(0.0f), scalar_from_float(1.25f), scalar_from_float(0.0f), TEST_MUL},
//         {"511.0 * 1024.0", scalar_from_float(511.0f), scalar_from_float(1024.0f), scalar_from_float(523264.0f), TEST_MUL},
//         {"-511.0 * 1024.0", scalar_from_float(-511.0f), scalar_from_float(1024.0f), scalar_from_float(-523264.0f), TEST_MUL},
//         // divide
//         {"1.0 / 1.0", scalar_from_float(1.0f), scalar_from_float(1.0f), scalar_from_float(1.0f), TEST_DIV},
//         {"2.0 / 1.0", scalar_from_float(2.0f), scalar_from_float(1.0f), scalar_from_float(2.0f), TEST_DIV},
//         {"1.0 / 0.5", scalar_from_float(1.0f), scalar_from_float(0.5f), scalar_from_float(2.0f), TEST_DIV},
//         {"1.0 / 4096.0", scalar_from_float(1.0f), scalar_from_float(4096.0f), scalar_from_float(1.0f / 4096.0f), TEST_DIV},
//         // sin
//         {"sin(0 deg)", scalar_from_float(0.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(90 deg)", scalar_from_float(90.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(180 deg)", scalar_from_float(180.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(270 deg)", scalar_from_float(270.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(360 deg)", scalar_from_float(360.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(15 deg)", scalar_from_float(15.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(30 deg)", scalar_from_float(30.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(45 deg)", scalar_from_float(45.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(60 deg)", scalar_from_float(60.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         {"sin(75 deg)", scalar_from_float(75.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_SIN},
//         // cos
//         {"cos(0 deg)", scalar_from_float(0.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(90 deg)", scalar_from_float(90.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(180 deg)", scalar_from_float(180.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(270 deg)", scalar_from_float(270.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(360 deg)", scalar_from_float(360.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(15 deg)", scalar_from_float(15.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(30 deg)", scalar_from_float(30.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(45 deg)", scalar_from_float(45.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(60 deg)", scalar_from_float(60.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//         {"cos(75 deg)", scalar_from_float(75.0f / 360.0f), 0, scalar_from_float(0.0f), TEST_COS},
//     };

//     for (size_t i = 0; i < (sizeof(tests) / sizeof(*tests)); ++i) {
//         scalar_t result = 0;
//         switch (tests[i].type) {
//             case TEST_MUL: result = scalar_mul(tests[i].a, tests[i].b); break;
//             case TEST_DIV: result = scalar_div(tests[i].a, tests[i].b); break;
//             case TEST_SIN: result = trig_sin(tests[i].a); break;
//             case TEST_COS: result = trig_cos(tests[i].a); break;
//             default: break;
//         }
//         if (result != tests[i].expected_result) {
//             printf("UNIT TEST FAILED: \"%s\": expected result %i, got %i\n", tests[i].description, tests[i].expected_result, result);
//             ++n_errors;
//         }
//     }

//     return n_errors;
// }

// int test_vertical_capsule_aabb_intersect(void) {
//     aabb_t aabb = {
//         .min = vec3_from_floats(-1.0f, -1.0f, -1.0f),
//         .max = vec3_from_floats(+1.0f, +1.0f, +1.0f),
//     };

//     int n_errors = 0;

//     struct {
//         char* description;
//         vec3_t bottom;
//         int expected_result_fast;
//         int expected_result_fancy;
//     } capsule_bottoms[] = {
//         {"capsule entirely inside aabb",       vec3_from_floats(0.0f, -0.5f, 0.0f),   1},
//         {"capsule bottom inside aabb",         vec3_from_floats(0.0f, +0.5f, 0.0f),   1},
//         {"capsule top inside aabb",            vec3_from_floats(0.0f, -1.5f, 0.0f),   1},
//         {"capsule touches on -X",              vec3_from_floats(-1.0f, -0.5f, 0.0f),  1},
//         {"capsule touches on +X",              vec3_from_floats(1.0f, -0.5f, 0.0f),   1},
//         {"capsule touches on -Z",              vec3_from_floats(0.0f, -0.5f, -1.0f),  1},
//         {"capsule touches on +Z",              vec3_from_floats(0.0f, -0.5f, +1.0f),  1},
//         {"capsule touches on +X+Z",            vec3_from_floats(+1.0f, -0.5f, +1.0f), 1},
//         {"capsule touches on +X-Z",            vec3_from_floats(+1.0f, -0.5f, -1.0f), 1},
//         {"capsule touches on -X+Z",            vec3_from_floats(-1.0f, -0.5f, +1.0f), 1},
//         {"capsule touches on -X-Z",            vec3_from_floats(-1.0f, -0.5f, -1.0f), 1},
//         {"capsule barely touching on -Y",      vec3_from_floats(0.0f, -2.49975f, 0.0f), 1},
//         {"capsule barely touching on +Y",      vec3_from_floats(0.0f, +1.49975f, 0.0f), 1},
//         {"capsule almost touching on -Y",      vec3_from_floats(0.0f, -2.50025f, 0.0f), 0},
//         {"capsule almost touching on +Y",      vec3_from_floats(0.0f, +1.50025f, 0.0f), 0},
//         {"capsule aabb corner +++",            vec3_from_floats(+1.2890f, +1.2890f, +1.2890f), 0},
//         {"capsule aabb corner ++-",            vec3_from_floats(+1.2890f, +1.2890f, -1.2890f), 0},
//         {"capsule aabb corner +-+",            vec3_from_floats(+1.2890f, -2.2890f, +1.2890f), 0},
//         {"capsule aabb corner +--",            vec3_from_floats(+1.2890f, -2.2890f, -1.2890f), 0},
//         {"capsule aabb corner -++",            vec3_from_floats(-1.2890f, +1.2890f, +1.2890f), 0},
//         {"capsule aabb corner -+-",            vec3_from_floats(-1.2890f, +1.2890f, -1.2890f), 0},
//         {"capsule aabb corner --+",            vec3_from_floats(-1.2890f, -2.2890f, +1.2890f), 0},
//         {"capsule aabb corner ---",            vec3_from_floats(-1.2890f, -2.2890f, -1.2890f), 0},
//         {"capsule aabb corner +++",            vec3_from_floats(+1.2885f, +1.2885f, +1.2885f), 1},
//         {"capsule aabb corner ++-",            vec3_from_floats(+1.2885f, +1.2885f, -1.2885f), 1},
//         {"capsule aabb corner +-+",            vec3_from_floats(+1.2885f, -2.2885f, +1.2885f), 1},
//         {"capsule aabb corner +--",            vec3_from_floats(+1.2885f, -2.2885f, -1.2885f), 1},
//         {"capsule aabb corner -++",            vec3_from_floats(-1.2885f, +1.2885f, +1.2885f), 1},
//         {"capsule aabb corner -+-",            vec3_from_floats(-1.2885f, +1.2885f, -1.2885f), 1},
//         {"capsule aabb corner --+",            vec3_from_floats(-1.2885f, -2.2885f, +1.2885f), 1},
//         {"capsule aabb corner ---",            vec3_from_floats(-1.2885f, -2.2885f, -1.2885f), 1},
//         {"capsule fully below aabb",           vec3_from_floats(0.0f, -3.0f, 0.0f),   0},
//         {"capsule fully above aabb",           vec3_from_floats(0.0f, +2.0f, 0.0f),   0},
//         {"capsule fully to the left of aabb",  vec3_from_floats(-2.0f, -0.5f, 0.0f),  0},
//         {"capsule fully to the right of aabb", vec3_from_floats(+2.0f, -0.5f, 0.0f),  0},
//         {"capsule fully to the front of aabb", vec3_from_floats(0.0f, -0.5f, +2.0f),  0},
//         {"capsule fully to the back of aabb",  vec3_from_floats(0.0f, -0.5f, -2.0f),  0},
//         {"capsule fully front left of aabb",   vec3_from_floats(-2.0f, -0.5f, +2.0f), 0},
//         {"capsule fully front right of aabb",  vec3_from_floats(+2.0f, -0.5f, +2.0f), 0},
//         {"capsule fully back left of aabb",    vec3_from_floats(-2.0f, -0.5f, -2.0f), 0},
//         {"capsule fully back right of aabb",   vec3_from_floats(+2.0f, -0.5f, -2.0f), 0},
//     };

//     for (size_t i = 0; i < sizeof(capsule_bottoms) / sizeof(capsule_bottoms[0]); ++i) {
//         rayhit_t hit = {0};

//         vertical_capsule_t capsule = {
//             .bottom = capsule_bottoms[i].bottom,
//             .height = ONE,
//             .radius = ONE/2,
//         };

//         const int result = vertical_capsule_aabb_intersect(&aabb, capsule);

//         if (result != capsule_bottoms[i].expected_result) {
//             printf("UNIT TEST FAILED: \"%s (fast)\": expected result %i, got %i\n", capsule_bottoms[i].description, capsule_bottoms[i].expected_result, result);
//             ++n_errors;
//         }
//     }

//     return n_errors;
// }

// int test_vertical_capsule_triangle_intersect(void) {
//     collision_triangle_3d_t triangles[] = {
//         (collision_triangle_3d_t){ // facing up
//             vec3_from_floats(0.0f, 0.0f, 0.0f),
//             vec3_from_floats(-1.5f, 0.0f, 2.0f),
//             vec3_from_floats(-2.0f, 0.0f, 0.0f),
//             {}
//         },
//         (collision_triangle_3d_t){ // facing side
//             vec3_from_floats(0.0f, 0.0f, 0.0f),
//             vec3_from_floats(-1.5f, 2.0f, 0.0f),
//             vec3_from_floats(-2.0f, 0.0f, 0.0f),
//             {}
//         },
//     };

//     for (size_t i = 0; i < (sizeof(triangles) / sizeof(*triangles)); ++i) {
//         const vec3_t e01 = vec3_sub(triangles[i].v1, triangles[i].v0);
//         const vec3_t e02 = vec3_sub(triangles[i].v2, triangles[i].v0);
//         triangles[i].normal = vec3_normalize(vec3_cross(e01, e02));
//     }

//     int n_errors = 0;

//     struct {
//         collision_triangle_3d_t* triangle;
//         char* description;
//         vec3_t bottom;
//         int expected_result;
//     } tests[] = {
//         // triangle facing up - corner
//         {&triangles[0], "t0 v0 miss bottom",                      vec3_from_floats(0.0f, 0.3f, 0.0f), 0},
//         {&triangles[0], "t0 v0 barely miss bottom",               vec3_from_floats(0.0f, 0.2505f, 0.0f), 0},
//         {&triangles[0], "t0 v0 hit radius bottom",                vec3_from_floats(0.0f, 0.125f, 0.0f), 1},
//         {&triangles[0], "t0 v0 barely hit segment bottom",        vec3_from_floats(0.0f, 0.0f, 0.0f), 1},
//         {&triangles[0], "t0 v0 hit center",                       vec3_from_floats(0.0f, -0.25f, 0.0f), 1},
//         {&triangles[0], "t0 v0 barely hit segment top",           vec3_from_floats(0.0f, -0.5f, 0.0f), 1},
//         {&triangles[0], "t0 v0 hit radius top",                   vec3_from_floats(0.0f, -0.625f, 0.0f), 1},
//         {&triangles[0], "t0 v0 barely miss top",                  vec3_from_floats(0.0f, -0.7505f, 0.0f), 0},
//         {&triangles[0], "t0 v0 miss top",                         vec3_from_floats(0.0f, -1.0000f, 0.0f), 0},
//         {&triangles[0], "t0 v1 barely miss bottom",               vec3_from_floats(-1.5f, 0.2505f, 2.0f), 0},
//         {&triangles[0], "t0 v1 hit center",                       vec3_from_floats(-1.5f, -0.25f, 2.0f), 1},
//         {&triangles[0], "t0 v1 barely miss top",                  vec3_from_floats(-1.5f, -0.7505f, 2.0f), 0},
//         {&triangles[0], "t0 v2 barely miss bottom",               vec3_from_floats(-2.0f, 0.2505f, 0.0f), 0},
//         {&triangles[0], "t0 v2 hit center",                       vec3_from_floats(-2.0f, -0.25f, 0.0f), 1},
//         {&triangles[0], "t0 v2 barely miss top",                  vec3_from_floats(-2.0f, -0.7505f, 0.0f), 0},

//         // triangle facing up - edge vertical
//         {&triangles[0], "t0 edge 0-1 miss bottom",                vec3_from_floats(-0.75f, 0.2505f, 1.0f), 0},
//         {&triangles[0], "t0 edge 0-1 hit bottom",                 vec3_from_floats(-0.75f, 0.2495f, 1.0f), 1},
//         {&triangles[0], "t0 edge 1-2 miss bottom",                vec3_from_floats(-1.75f, 0.2505f, 1.0f), 0},
//         {&triangles[0], "t0 edge 1-2 hit bottom",                 vec3_from_floats(-1.75f, 0.2495f, 1.0f), 1},
//         {&triangles[0], "t0 edge 2-0 miss bottom",                vec3_from_floats(-1.0f, 0.2505f, 0.0f), 0},
//         {&triangles[0], "t0 edge 2-0 hit bottom",                 vec3_from_floats(-1.0f, 0.2495f, 0.0f), 1},

//         // triangle facing up - edge sideways
//         {&triangles[0], "t0 edge 0-1 miss sideways",              vec3_from_floats(-0.5525f, -0.25f, 1.165f), 0},
//         {&triangles[0], "t0 edge 0-1 hit sideways",               vec3_from_floats(-0.5725f, -0.25f, 1.165f), 1},
//         {&triangles[0], "t0 edge 1-2 miss sideways",              vec3_from_floats(-1.98861f, -0.25f, 1.14448f), 0},
//         {&triangles[0], "t0 edge 1-2 hit sideways",               vec3_from_floats(-1.96953f, -0.25f, 1.14486f), 1},
//         {&triangles[0], "t0 edge 2-1 miss sideways",              vec3_from_floats(-1.08621f, -0.25f, -0.251354f), 0},
//         {&triangles[0], "t0 edge 2-1 hit sideways",               vec3_from_floats(-1.08621f, -0.25f, -0.248892f), 1},

//         // triangle facing up - corners sideways
//         {&triangles[0], "t0 corner v0 hit",                       vec3_from_floats(0.165f, -0.25f, -0.165f), 1},
//         {&triangles[0], "t0 corner v0 miss",                      vec3_from_floats(0.175f, -0.25f, -0.185f), 0},
//         {&triangles[0], "t0 corner v1 hit",                       vec3_from_floats(-1.5f, -0.25f, 2.249f), 1},
//         {&triangles[0], "t0 corner v1 miss",                      vec3_from_floats(-1.5f, -0.25f, 2.251f), 0},
//         {&triangles[0], "t0 corner v2 hit",                       vec3_from_floats(-2.16628f, -0.25f, -0.183405f), 1},
//         {&triangles[0], "t0 corner v2 miss",                      vec3_from_floats(-2.16988f, -0.25f, -0.187204f), 0},

//         // triangle facing up - center
//         {&triangles[0], "t0 center miss bottom",                  vec3_from_floats(-1.16666f, 0.3f, 0.66666f), 0},
//         {&triangles[0], "t0 center barely miss bottom",           vec3_from_floats(-1.16666f, 0.2505f, 0.66666f), 0},
//         {&triangles[0], "t0 center hit radius bottom",            vec3_from_floats(-1.16666f, 0.125f, 0.66666f), 1},
//         {&triangles[0], "t0 center barely hit segment bottom",    vec3_from_floats(-1.16666f, 0.0f, 0.66666f), 1},
//         {&triangles[0], "t0 center hit center",                   vec3_from_floats(-1.16666f, -0.25f, 0.66666f), 1},
//         {&triangles[0], "t0 center barely hit segment top",       vec3_from_floats(-1.16666f, -0.5f, 0.66666f), 1},
//         {&triangles[0], "t0 center hit radius top",               vec3_from_floats(-1.16666f, -0.625f, 0.66666f), 1},
//         {&triangles[0], "t0 center barely miss top",              vec3_from_floats(-1.16666f, -0.7505f, 0.66666f), 0},
//         {&triangles[0], "t0 center miss top",                     vec3_from_floats(-1.16666f, -1.0000f, 0.66666f), 0},

//         // triangle facing side - on vertices
//         {&triangles[1], "t1 v0 +Z vertex top miss",                      vec3_from_floats(0.0f, -0.51f, 0.3f), 0},
//         {&triangles[1], "t1 v0 +Z vertex top barely miss",               vec3_from_floats(0.0f, -0.51f, 0.251f), 0},
//         {&triangles[1], "t1 v0 +Z vertex top barely hit",                vec3_from_floats(0.0f, -0.51f, 0.249f), 1},
//         {&triangles[1], "t1 v0 +Z vertex top hit",                       vec3_from_floats(0.0f, -0.51f, 0.125f), 1},
//         {&triangles[1], "t1 v0 +Z vertex center miss",                   vec3_from_floats(0.0f, -0.25f, 0.3f), 0},
//         {&triangles[1], "t1 v0 +Z vertex center barely miss",            vec3_from_floats(0.0f, -0.25f, 0.2505f), 0},
//         {&triangles[1], "t1 v0 +Z vertex center barely hit",             vec3_from_floats(0.0f, -0.25f, 0.2495f), 1},
//         {&triangles[1], "t1 v0 +Z vertex center hit",                    vec3_from_floats(0.0f, -0.25f, 0.125f), 1},
//         {&triangles[1], "t1 v0 +Z vertex bottom miss",                   vec3_from_floats(0.0f, 0.0f, 0.3f), 0},
//         {&triangles[1], "t1 v0 +Z vertex bottom barely miss",            vec3_from_floats(0.0f, 0.0f, 0.2505f), 0},
//         {&triangles[1], "t1 v0 +Z vertex bottom barely hit",             vec3_from_floats(0.0f, 0.0f, 0.2495f), 1},
//         {&triangles[1], "t1 v0 +Z vertex bottom hit",                    vec3_from_floats(0.0f, 0.0f, 0.125f), 1},
//         {&triangles[1], "t1 v0 -Z vertex center miss",                   vec3_from_floats(0.0f, -0.25f, -0.3f), 0},
//         {&triangles[1], "t1 v0 -Z vertex center barely miss",            vec3_from_floats(0.0f, -0.25f, -0.2505f), 0},
//         {&triangles[1], "t1 v0 -Z vertex center barely hit",             vec3_from_floats(0.0f, -0.25f, -0.2495f), 1},
//         {&triangles[1], "t1 v0 -Z vertex center hit",                    vec3_from_floats(0.0f, -0.25f, -0.125f), 1},
//         {&triangles[1], "t1 v0 -Z vertex top miss",                      vec3_from_floats(0.0f, -0.51f, -0.3f), 0},
//         {&triangles[1], "t1 v0 -Z vertex top barely miss",               vec3_from_floats(0.0f, -0.51f, -0.251f), 0},
//         {&triangles[1], "t1 v0 -Z vertex top barely hit",                vec3_from_floats(0.0f, -0.51f, -0.249f), 1},
//         {&triangles[1], "t1 v0 -Z vertex top hit",                       vec3_from_floats(0.0f, -0.51f, -0.125f), 1},
//         {&triangles[1], "t1 v1 +Z vertex top miss",                      vec3_from_floats(-1.5f, 1.49f, 0.3f), 0},
//         {&triangles[1], "t1 v1 +Z vertex top barely miss",               vec3_from_floats(-1.5f, 1.49f, 0.251f), 0},
//         {&triangles[1], "t1 v1 +Z vertex top barely hit",                vec3_from_floats(-1.5f, 1.49f, 0.249f), 1},
//         {&triangles[1], "t1 v1 +Z vertex top hit",                       vec3_from_floats(-1.5f, 1.49f, 0.125f), 1},
//         {&triangles[1], "t1 v1 +Z vertex center miss",                   vec3_from_floats(-1.5f, 1.75f, 0.3f), 0},
//         {&triangles[1], "t1 v1 +Z vertex center barely miss",            vec3_from_floats(-1.5f, 1.75f, 0.2505f), 0},
//         {&triangles[1], "t1 v1 +Z vertex center barely hit",             vec3_from_floats(-1.5f, 1.75f, 0.2495f), 1},
//         {&triangles[1], "t1 v1 +Z vertex center hit",                    vec3_from_floats(-1.5f, 1.75f, 0.125f), 1},
//         {&triangles[1], "t1 v1 +Z vertex bottom miss",                   vec3_from_floats(-1.5f, 2.0f, 0.3f), 0},
//         {&triangles[1], "t1 v1 +Z vertex bottom barely miss",            vec3_from_floats(-1.5f, 2.0f, 0.2505f), 0},
//         {&triangles[1], "t1 v1 +Z vertex bottom barely hit",             vec3_from_floats(-1.5f, 2.0f, 0.2495f), 1},
//         {&triangles[1], "t1 v1 +Z vertex bottom hit",                    vec3_from_floats(-1.5f, 2.0f, 0.125f), 1},
//         {&triangles[1], "t1 v1 -Z vertex center miss",                   vec3_from_floats(-1.5f, 1.75f, -0.3f), 0},
//         {&triangles[1], "t1 v1 -Z vertex center barely miss",            vec3_from_floats(-1.5f, 1.75f, -0.2505f), 0},
//         {&triangles[1], "t1 v1 -Z vertex center barely hit",             vec3_from_floats(-1.5f, 1.75f, -0.2495f), 1},
//         {&triangles[1], "t1 v1 -Z vertex center hit",                    vec3_from_floats(-1.5f, 1.75f, -0.125f), 1},
//         {&triangles[1], "t1 v1 -Z vertex top miss",                      vec3_from_floats(-1.5f, 1.49f, -0.3f), 0},
//         {&triangles[1], "t1 v1 -Z vertex top barely miss",               vec3_from_floats(-1.5f, 1.49f, -0.2505f), 0},
//         {&triangles[1], "t1 v1 -Z vertex top barely hit",                vec3_from_floats(-1.5f, 1.49f, -0.2495f), 1},
//         {&triangles[1], "t1 v1 -Z vertex top hit",                       vec3_from_floats(-1.5f, 1.49f, -0.125f), 1},
//         {&triangles[1], "t1 v2 +Z vertex top miss",                      vec3_from_floats(-2.0f, -0.51f, 0.3f), 0},
//         {&triangles[1], "t1 v2 +Z vertex top barely miss",               vec3_from_floats(-2.0f, -0.51f, 0.251f), 0},
//         {&triangles[1], "t1 v2 +Z vertex top barely hit",                vec3_from_floats(-2.0f, -0.51f, 0.249f), 1},
//         {&triangles[1], "t1 v2 +Z vertex top hit",                       vec3_from_floats(-2.0f, -0.51f, 0.125f), 1},
//         {&triangles[1], "t1 v2 +Z vertex center miss",                   vec3_from_floats(-2.0f, -0.25f, 0.3f), 0},
//         {&triangles[1], "t1 v2 +Z vertex center barely miss",            vec3_from_floats(-2.0f, -0.25f, 0.2505f), 0},
//         {&triangles[1], "t1 v2 +Z vertex center barely hit",             vec3_from_floats(-2.0f, -0.25f, 0.2495f), 1},
//         {&triangles[1], "t1 v2 +Z vertex center hit",                    vec3_from_floats(-2.0f, -0.25f, 0.125f), 1},
//         {&triangles[1], "t1 v2 +Z vertex bottom miss",                   vec3_from_floats(-2.0f, 0.0f, 0.3f), 0},
//         {&triangles[1], "t1 v2 +Z vertex bottom barely miss",            vec3_from_floats(-2.0f, 0.0f, 0.2505f), 0},
//         {&triangles[1], "t1 v2 +Z vertex bottom barely hit",             vec3_from_floats(-2.0f, 0.0f, 0.2495f), 1},
//         {&triangles[1], "t1 v2 +Z vertex bottom hit",                    vec3_from_floats(-2.0f, 0.0f, 0.125f), 1},
//         {&triangles[1], "t1 v2 -Z vertex center miss",                   vec3_from_floats(-2.0f, -0.25f, -0.3f), 0},
//         {&triangles[1], "t1 v2 -Z vertex center barely miss",            vec3_from_floats(-2.0f, -0.25f, -0.2505f), 0},
//         {&triangles[1], "t1 v2 -Z vertex center barely hit",             vec3_from_floats(-2.0f, -0.25f, -0.2495f), 1},
//         {&triangles[1], "t1 v2 -Z vertex center hit",                    vec3_from_floats(-2.0f, -0.25f, -0.125f), 1},
//         {&triangles[1], "t1 v2 -Z vertex top miss",                      vec3_from_floats(-2.0f, -0.51f, -0.3f), 0},
//         {&triangles[1], "t1 v2 -Z vertex top barely miss",               vec3_from_floats(-2.0f, -0.51f, -0.251f), 0},
//         {&triangles[1], "t1 v2 -Z vertex top barely hit",                vec3_from_floats(-2.0f, -0.51f, -0.249f), 1},
//         {&triangles[1], "t1 v2 -Z vertex top hit",                       vec3_from_floats(-2.0f, -0.51f, -0.125f), 1},

//         // triangle facing side - barely hit vertices
//         {&triangles[1], "t1 v0 barely on vertex top miss +X",            vec3_from_floats(0.21f, -0.65f, 0.0f), 0},
//         {&triangles[1], "t1 v0 barely on vertex top hit +X",             vec3_from_floats(0.19f, -0.65f, 0.0f), 1},
//         {&triangles[1], "t1 v0 barely on vertex bottom miss +X",         vec3_from_floats(0.21f, 0.15f, 0.0f), 0},
//         {&triangles[1], "t1 v0 barely on vertex bottom hit +X",          vec3_from_floats(0.19f, 0.15f, 0.0f), 1},
//         {&triangles[1], "t1 v1 barely on vertex bottom miss -X",         vec3_from_floats(-1.69f, 2.17f, 0.0f), 0},
//         {&triangles[1], "t1 v1 barely on vertex bottom hit -X",          vec3_from_floats(-1.67f, 2.17f, 0.0f), 1},
//         {&triangles[1], "t1 v1 barely on vertex bottom hit +X",          vec3_from_floats(-1.32f, 2.17f, 0.0f), 1},
//         {&triangles[1], "t1 v1 barely on vertex bottom miss +X",         vec3_from_floats(-1.31f, 2.17f, 0.0f), 0},
//         {&triangles[1], "t1 v2 barely on vertex top miss +X",            vec3_from_floats(-2.21f, -0.65f, 0.0f), 0},
//         {&triangles[1], "t1 v2 barely on vertex top hit +X",             vec3_from_floats(-2.19f, -0.65f, 0.0f), 1},
//         {&triangles[1], "t1 v2 barely on vertex bottom hit +X",          vec3_from_floats(-2.21f, 0.15f, 0.0f), 1},
//         {&triangles[1], "t1 v2 barely on vertex bottom miss +X",         vec3_from_floats(-2.23f, 0.15f, 0.0f), 0},

//         // triangle facing side - edges
//         {&triangles[1], "t1 e20 -Z radius miss",           vec3_from_floats(-1.0f, -0.65f, -0.3f), 0},
//         {&triangles[1], "t1 e20 -Z radius barely miss",    vec3_from_floats(-1.0f, -0.65f, -0.21f), 0},
//         {&triangles[1], "t1 e20 -Z radius barely hit",     vec3_from_floats(-1.0f, -0.65f, -0.19f), 1},
//         {&triangles[1], "t1 e20 -Z radius hit",            vec3_from_floats(-1.0f, -0.65f, -0.1f), 1},
//         {&triangles[1], "t1 e20 Z radius center",          vec3_from_floats(-1.0f, -0.65f, 0.0f), 1},
//         {&triangles[1], "t1 e20 +Z radius hit",            vec3_from_floats(-1.0f, -0.65f, 0.1f), 1},
//         {&triangles[1], "t1 e20 +Z radius barely hit",     vec3_from_floats(-1.0f, -0.65f, 0.19f), 1},
//         {&triangles[1], "t1 e20 +Z radius barely miss",    vec3_from_floats(-1.0f, -0.65f, 0.21f), 0},
//         {&triangles[1], "t1 e20 +Z radius miss",           vec3_from_floats(-1.0f, -0.65f, 0.3f), 0},
//         {&triangles[1], "t1 e01 -Z radius miss",           vec3_from_floats(-0.5f, 0.92f, -0.3f), 0},
//         {&triangles[1], "t1 e01 -Z radius barely miss",    vec3_from_floats(-0.5f, 0.92f, -0.22f), 0},
//         {&triangles[1], "t1 e01 -Z radius barely hit",     vec3_from_floats(-0.5f, 0.92f, -0.19f), 1},
//         {&triangles[1], "t1 e01 -Z radius hit",            vec3_from_floats(-0.5f, 0.92f, -0.1f), 1},
//         {&triangles[1], "t1 e01 Z radius center",          vec3_from_floats(-0.5f, 0.92f, 0.0f), 1},
//         {&triangles[1], "t1 e01 +Z radius hit",            vec3_from_floats(-0.5f, 0.92f, 0.1f), 1},
//         {&triangles[1], "t1 e01 +Z radius barely hit",     vec3_from_floats(-0.5f, 0.92f, 0.19f), 1},
//         {&triangles[1], "t1 e01 +Z radius barely miss",    vec3_from_floats(-0.5f, 0.92f, 0.22f), 0},
//         {&triangles[1], "t1 e01 +Z radius miss",           vec3_from_floats(-0.5f, 0.92f, 0.3f), 0},
//         {&triangles[1], "t1 e12 -Z radius miss",           vec3_from_floats(-1.91f, 0.92f, -0.3f), 0},
//         {&triangles[1], "t1 e12 -Z radius barely miss",    vec3_from_floats(-1.91f, 0.92f, -0.22f), 0},
//         {&triangles[1], "t1 e12 -Z radius barely hit",     vec3_from_floats(-1.91f, 0.92f, -0.19f), 1},
//         {&triangles[1], "t1 e12 -Z radius hit",            vec3_from_floats(-1.91f, 0.92f, -0.1f), 1},
//         {&triangles[1], "t1 e12 Z radius center",          vec3_from_floats(-1.91f, 0.92f, 0.0f), 1},
//         {&triangles[1], "t1 e12 +Z radius hit",            vec3_from_floats(-1.91f, 0.92f, 0.1f), 1},
//         {&triangles[1], "t1 e12 +Z radius barely hit",     vec3_from_floats(-1.91f, 0.92f, 0.19f), 1},
//         {&triangles[1], "t1 e12 +Z radius barely miss",    vec3_from_floats(-1.91f, 0.92f, 0.22f), 0},
//         {&triangles[1], "t1 e12 +Z radius miss",           vec3_from_floats(-1.91f, 0.92f, 0.3f), 0},
//         {&triangles[1], "t1 center -Z radius miss",           vec3_from_floats(-1.0f, 0.4f, -0.3f), 0},
//         {&triangles[1], "t1 center -Z radius barely miss",    vec3_from_floats(-1.0f, 0.4f, -0.251f), 0},
//         {&triangles[1], "t1 center -Z radius barely hit",     vec3_from_floats(-1.0f, 0.4f, -0.249f), 1},
//         {&triangles[1], "t1 center -Z radius hit",            vec3_from_floats(-1.0f, 0.4f, -0.125f), 1},
//         {&triangles[1], "t1 center Z radius center",          vec3_from_floats(-1.0f, 0.4f, 0.0f), 1},
//         {&triangles[1], "t1 center +Z radius hit",            vec3_from_floats(-1.0f, 0.4f, 0.125f), 1},
//         {&triangles[1], "t1 center +Z radius barely hit",     vec3_from_floats(-1.0f, 0.4f, 0.249f), 1},
//         {&triangles[1], "t1 center +Z radius barely miss",    vec3_from_floats(-1.0f, 0.4f, 0.251f), 0},
//         {&triangles[1], "t1 center +Z radius miss",           vec3_from_floats(-1.0f, 0.4f, 0.3f), 0},
//     };


//     for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); ++i) {
//         rayhit_t hit = {.distance = INT32_MAX};

//         vertical_capsule_t capsule = {
//             .bottom = tests[i].bottom,
//             .height = ONE/2,
//             .radius = ONE/4,
//         };

//         const int result = vertical_capsule_triangle_intersect(tests[i].triangle, capsule, &hit);

//         if (result != tests[i].expected_result) {
//             printf("UNIT TEST FAILED: \"%s\": expected result %i, got %i\n", tests[i].description, tests[i].expected_result, result);
//             ++n_errors;

//             vertical_capsule_triangle_intersect(tests[i].triangle, capsule, &hit);
//         }
//     }

//     return n_errors;
// }

int test(void) {
    int n_errors = 0;

    // n_errors += test_scalar_math();
    // n_errors += test_vertical_capsule_aabb_intersect();
    // n_errors += test_vertical_capsule_triangle_intersect();

    return n_errors;
}
