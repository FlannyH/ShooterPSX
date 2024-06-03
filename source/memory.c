#include "memory.h"
#include "common.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

uint32_t mem_stack_temp [350  * KiB / sizeof(uint32_t)];
uint32_t mem_stack_level[960 * KiB / sizeof(uint32_t)];
uint32_t mem_stack_music[64 * KiB / sizeof(uint32_t)];
uint32_t mem_stack_entity[64 * KiB / sizeof(uint32_t)];
uint32_t mem_stack_vram_swap[1 * KiB / sizeof(uint32_t)];
uint32_t* mem_stack_scratchpad = (uint32_t*)0x1F800000;
size_t mem_stack_cursor_temp = 0;
size_t mem_stack_cursor_level = 0;
size_t mem_stack_cursor_music = 0;
size_t mem_stack_cursor_entity = 0;
size_t mem_stack_cursor_vram_swap = 0;
size_t mem_stack_cursor_scratchpad = 0;

#ifdef _DEBUG
char* mem_cat_strings[] = {
    "MEM_CAT_UNDEFINED",
    "MEM_CAT_TEXTURE",
    "MEM_CAT_MODEL",
    "MEM_CAT_MESH",
    "MEM_CAT_COLLISION",
    "MEM_CAT_AUDIO",
    "MEM_CAT_FILE",
};

char* stack_names[] = {
    "STACK_TEMP",
    "STACK_LEVEL",
    "STACK_MUSIC",
    "STACK_ENTITY",
    "STACK_VRAM_SWAP",
    "STACK_SCRATCHPAD",
};
#endif

void* allocated_memory_pointers[512] = {0};
size_t allocated_memory_size[512] = {0};
memory_category_t allocated_memory_category[512] = {0};

void* scheduled_frees[32];
int n_scheduled_frees = 0;

void* mem_stack_alloc(size_t size, stack_t stack) {
	// Convert stack enum into actual pointers and cursors
	uint32_t* base = NULL;
	size_t* cursor;

#ifdef _DEBUG
	if (stack != STACK_SCRATCHPAD) {
		printf("stack %s (id %i) has %i bytes available and we need %i bytes... ", stack_names[stack], stack, mem_stack_get_size(stack) - mem_stack_get_occupied(stack), size);
	}
#endif
	
	switch (stack) {
		case STACK_LEVEL:
			base = mem_stack_level;
			cursor = &mem_stack_cursor_level;
			break;
		case STACK_MUSIC:
			base = mem_stack_music;
			cursor = &mem_stack_cursor_music;
			break;
		case STACK_TEMP:
			base = mem_stack_temp;
			cursor = &mem_stack_cursor_temp;
			break;
		case STACK_ENTITY:
			base = mem_stack_entity;
			cursor = &mem_stack_cursor_entity;
			break;
		case STACK_VRAM_SWAP:
			base = mem_stack_vram_swap;
			cursor = &mem_stack_cursor_vram_swap;
			break;
		case STACK_SCRATCHPAD:
			base = mem_stack_scratchpad;
			cursor = &mem_stack_cursor_scratchpad;
			break;
		default:
			printf("invalid stack allocation\n");
			return NULL;
	}

#ifdef _DEBUG
	if (stack != STACK_SCRATCHPAD) {
		// Make sure there is enough space
		if (size > mem_stack_get_free(stack)) {
			printf("turns out we don't have that.\n");
			return NULL;
		}
		printf("which we have! move on :D\n");
		printf("allocated to %p\n", cursor);
	}
#endif

	// Convert size from bytes to words
	size_t actual_size_to_alloc = (size + 3) >> 2;
	void* pointer = (void*)(base + *cursor);
	*cursor += actual_size_to_alloc;
	return pointer;
}

void mem_stack_release(stack_t stack) {
	switch (stack) {
		case STACK_LEVEL:
			mem_stack_cursor_level = 0;
			break;
		case STACK_MUSIC:
			mem_stack_cursor_music = 0;
			break;
		case STACK_TEMP:
			mem_stack_cursor_temp = 0;
			break;
		case STACK_ENTITY:
			mem_stack_cursor_entity = 0;
			break;
		case STACK_VRAM_SWAP:
			mem_stack_cursor_vram_swap = 0;
			break;
		case STACK_SCRATCHPAD:
			mem_stack_cursor_scratchpad = 0;
			break;
		default:
			break;
	}
}

size_t mem_stack_get_size(stack_t stack) {
	switch (stack) {
		case STACK_LEVEL:
			return sizeof(mem_stack_level);
			break;
		case STACK_MUSIC:
			return sizeof(mem_stack_music);
			break;
		case STACK_TEMP:
			return sizeof(mem_stack_temp);
			break;
		case STACK_ENTITY:
			return sizeof(mem_stack_entity);
			break;
		case STACK_VRAM_SWAP:
			return sizeof(mem_stack_vram_swap);
			break;
		case STACK_SCRATCHPAD:
			return 1024;
			break;
		default:
			return 0;
			break;
	}
}

size_t mem_stack_get_occupied(stack_t stack) {
	switch (stack) {
		case STACK_LEVEL:
			return mem_stack_cursor_level << 2;
			break;
		case STACK_MUSIC:
			return mem_stack_cursor_music << 2;
			break;
		case STACK_TEMP:
			return mem_stack_cursor_temp << 2;
			break;
		case STACK_ENTITY:
			return mem_stack_cursor_entity << 2;
			break;
		case STACK_VRAM_SWAP:
			return mem_stack_cursor_vram_swap << 2;
			break;
		case STACK_SCRATCHPAD:
			return mem_stack_cursor_scratchpad << 2;
			break;
		default:
			return 0;
			break;
	}
}

size_t mem_stack_get_free(stack_t stack) {
	return mem_stack_get_size(stack) - mem_stack_get_occupied(stack);
}

void* mem_alloc(size_t size, memory_category_t category) {
#ifdef _DEBUG
    // printf("allocating %i bytes for category %s\n", size, mem_cat_strings[category]);
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
#ifdef _DEBUG
    for (size_t i = 0; i < 512; ++i) {
        if (allocated_memory_pointers[i] == ptr) {
            // printf("freeing %i bytes for category %s\n", allocated_memory_size[i], mem_cat_strings[allocated_memory_category[i]]);

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
#ifdef _DEBUG
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