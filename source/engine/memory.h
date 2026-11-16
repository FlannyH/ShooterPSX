#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
    MEM_CAT_UNDEFINED,
    MEM_CAT_TEXTURE,
    MEM_CAT_MODEL,
    MEM_CAT_MESH,
    MEM_CAT_COLLISION,
    MEM_CAT_AUDIO,
    MEM_CAT_FILE,
    N_MEM_CATS
} memory_category_t;

typedef enum {
    STACK_TEMP,
    STACK_LEVEL,
    STACK_MUSIC,
    STACK_ENTITY,
    STACK_PERSISTENT,
    N_STACK_TYPES,
} stack_t;

void mem_init(void);

void* mem_stack_alloc(size_t size, stack_t stack);
void mem_stack_release(stack_t stack);
size_t mem_stack_get_marker(stack_t stack);
void mem_stack_reset_to_marker(stack_t stack, size_t marker);
size_t mem_stack_get_size(stack_t stack);
size_t mem_stack_get_occupied(stack_t stack);
size_t mem_stack_get_free(stack_t stack);

void* mem_dynamic_alloc(size_t size, memory_category_t category, const char* name);
size_t mem_dynamic_get_size(memory_category_t category);
size_t mem_dynamic_get_occupied(memory_category_t category);
size_t mem_dynamic_get_free(memory_category_t category);
void mem_dynamic_free(void* ptr);
void mem_dynamic_delayed_free(void* ptr);
void mem_dynamic_cleanup(void);
void mem_debug(void);

void mem_copy(void* to, const void* from, size_t bytes);
void mem_string_copy(char* to, const char* from, size_t max_bytes);

#ifdef _DEBUG
char* mem_stack_get_name(int i);
char* mem_dynamic_get_name(int i);
#endif

#ifdef _LEVEL_EDITOR
typedef struct {
    void* ptr;
    char* name;
    size_t size;
    memory_category_t category;
} mem_cat_debug_info_t;
mem_cat_debug_info_t mem_get_dynamic_debug_info(int i);
#endif

#ifdef __cplusplus
}
#endif

#endif
