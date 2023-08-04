#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

char* mem_cat_strings[] = {
    "MEM_CAT_UNDEFINED",
    "MEM_CAT_TEXTURE",
    "MEM_CAT_MODEL",
    "MEM_CAT_MESH",
    "MEM_CAT_COLLISION",
    "MEM_CAT_AUDIO",
    "MEM_CAT_FILE",
};

void* allocated_memory_pointers[512] = {0};
size_t allocated_memory_size[512] = {0};
memory_category_t allocated_memory_category[512] = {0};

void* scheduled_frees[32];
int n_scheduled_frees = 0;

void* mem_alloc(size_t size, memory_category_t category) {
#ifdef DEBUG
    printf("allocating %i bytes for category %s\n", size, mem_cat_strings[category]);
    void* result = NULL;
    for (size_t i = 0; i < 512; ++i) {
        if (allocated_memory_pointers[i] == NULL) {
            result = malloc(size);
            allocated_memory_pointers[i] = result;
            allocated_memory_size[i] = size;
            allocated_memory_category[i] = category;
            break;
        }
    }
    return result;
#else
    return malloc(size);
#endif
}

void mem_free(void* ptr) {
#ifdef DEBUG
    for (size_t i = 0; i < 512; ++i) {
        if (allocated_memory_pointers[i] == ptr) {
            printf("freeing %i bytes for category %s\n", allocated_memory_size[i], mem_cat_strings[allocated_memory_category[i]]);

            allocated_memory_pointers[i] = NULL;
            allocated_memory_size[i] = 0;
            allocated_memory_category[i] = 0;
            break;
        }
    }
#endif
    free(ptr);
}

void mem_delayed_free(void* ptr) {
    scheduled_frees[n_scheduled_frees++] = ptr;
}

void mem_free_scheduled_frees() {
    for (int i = 0; i < n_scheduled_frees; ++i) {
        mem_free(scheduled_frees[i]);
    }
}

void mem_debug() {
#ifdef DEBUG
    // Sum up each category
    size_t size_per_category[16] = {0};
    size_t size_total = 0;
    for (size_t i = 0; i < 512; ++i) {
        size_per_category[allocated_memory_category[i]] += allocated_memory_size[i];
        size_total += allocated_memory_size[i];
    }
    printf("\n--------MEMORY-DEBUG--------\n");
    printf("MEM_CAT_UNDEFINED: %i bytes\n", size_per_category[MEM_CAT_UNDEFINED]);
    printf("MEM_CAT_TEXTURE:   %i bytes\n", size_per_category[MEM_CAT_TEXTURE]);
    printf("MEM_CAT_MODEL:     %i bytes\n", size_per_category[MEM_CAT_MODEL]);
    printf("MEM_CAT_MESH:      %i bytes\n", size_per_category[MEM_CAT_MESH]);
    printf("MEM_CAT_COLLISION: %i bytes\n", size_per_category[MEM_CAT_COLLISION]);
    printf("MEM_CAT_AUDIO:     %i bytes\n", size_per_category[MEM_CAT_AUDIO]);
    printf("MEM_CAT_FILE:      %i bytes\n", size_per_category[MEM_CAT_FILE]);
    printf("total:             %i bytes\n", size_total);
#endif
}