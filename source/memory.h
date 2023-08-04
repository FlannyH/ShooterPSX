#include <stddef.h>

typedef enum {
    MEM_CAT_UNDEFINED,
    MEM_CAT_TEXTURE,
    MEM_CAT_MODEL,
    MEM_CAT_MESH,
    MEM_CAT_COLLISION,
    MEM_CAT_AUDIO,
    MEM_CAT_FILE,
} memory_category_t;

void* mem_alloc(size_t size, memory_category_t category);
void mem_free(void* ptr);
void mem_delayed_free(void* ptr);
void mem_free_scheduled_frees();
void mem_debug();