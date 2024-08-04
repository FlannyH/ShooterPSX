#ifndef MEMORY_H
#define MEMORY_H
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

typedef enum {
    STACK_TEMP,
    STACK_LEVEL,
    STACK_MUSIC,
    STACK_ENTITY,
    STACK_VRAM_SWAP,
	N_STACK_TYPES,
} stack_t;

void mem_init(void);
void* mem_alloc(size_t size, memory_category_t category);
void* mem_stack_alloc(size_t size, stack_t stack);
void mem_stack_release(stack_t stack);
size_t mem_stack_get_marker(stack_t stack);
void mem_stack_reset_to_marker(stack_t stack, size_t marker);
size_t mem_stack_get_size(stack_t stack);
size_t mem_stack_get_occupied(stack_t stack);
size_t mem_stack_get_free(stack_t stack);
void mem_free(void* ptr);
void mem_delayed_free(void* ptr);
void mem_free_scheduled_frees(void);
void mem_debug(void);

extern char* stack_names[];

#endif