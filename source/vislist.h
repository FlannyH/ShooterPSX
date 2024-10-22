#ifndef VISLIST_H
#define VISLIST_H
#include "structs.h"
#include "memory.h"

typedef struct {
    uint32_t sections_0_31;
    uint32_t sections_32_63;
    uint32_t sections_64_95;
    uint32_t sections_96_127;
} visfield_t;

typedef struct {
    svec3_t min;
    svec3_t max;
    uint32_t child_or_vis_index;
} visbvh_node_t;

typedef struct {
    visbvh_node_t* bvh_root;
    visfield_t* vislists;
} vislist_t;

vislist_t vislist_load(const char* path, int on_stack, stack_t stack);

#endif
