#ifdef _PSX
#include "../common.h"
#include "file.h"
#include "memory.h"

#include <string.h>
#include <stdio.h>

#define PC_SEEK_MODE_START 0
#define PC_SEEK_MODE_CURR 1
#define PC_SEEK_MODE_END 2
#define PC_ACCESS_MODE_READ_ONLY 0
// todo: more access modes

void pc_init() { 
    __asm__ volatile("break 0, 0x101"); 
}

int pc_open(const char* filename, int access_mode) {
    register const char* r_fn __asm__("a1") = filename;
    register int r_am __asm__("a2")         = access_mode;
    register int status __asm__("v0");
    register int info __asm__("v1");

    __asm__ volatile("break 0, 0x103" : "=r"(status), "=r"(info) : "r"(r_fn), "r"(r_am) : "memory");

    return status < 0 ? -info : info;
}

int pc_read(int file_handle, int length, void* destination) {
    register int r_fn __asm__("a1")   = file_handle;
    register int r_len __asm__("a2")  = length;
    register void* r_dest __asm__("a3") = destination;
    register int status __asm__("v0");
    register int info __asm__("v1");

    __asm__ volatile("break 0, 0x105" : "=r"(status), "=r"(info) : "r"(r_fn), "r"(r_len), "r"(r_dest) : "memory");

    return status < 0 ? -info : info;
}

int pc_lseek(int file_handle, int offset, int seek_mode) {
    register int r_fn __asm__("a1")   = file_handle;
    register int r_off __asm__("a2")  = offset;
    register int r_mode __asm__("a3") = seek_mode;
    register int status __asm__("v0");
    register int info __asm__("v1");

    __asm__ volatile("break 0, 0x107" : "=r"(status), "=r"(info) : "r"(r_fn), "r"(r_off), "r"(r_mode) : "memory");

    return status < 0 ? -info : info;
}

int pc_close(int file_handle) { 
    register int r_fn __asm__("a1") = file_handle;
    register int status __asm__("v0");
    register int info __asm__("v1");

    __asm__ volatile("break 0, 0x104" : "=r"(status), "=r"(info) : "r"(r_fn) : "memory");
    
    return status < 0 ? -info : info;
}

int pc_file_size(int file_handle) { 
    const int end = pc_lseek(file_handle, 0, PC_SEEK_MODE_END);
    const int start = pc_lseek(file_handle, 0, PC_SEEK_MODE_START);
    return end - start;
}

void file_init(const char* path) {
    (void)path;
    pc_init();
}

int file_read(const char* path, uint32_t** destination, size_t* size, int on_stack, stack_t stack) {
    // Open file
    printf("QWERTYtest1\n");
    int file_handle = pc_open(path, PC_ACCESS_MODE_READ_ONLY);
    printf("QWERTYtest2\n");
    if (file_handle < 0) {
        printf("[ERROR] Error loading file \"%s\" via pcdrv!\n");
        return 0;
    }

    // Get file size and seek to start
    int end = pc_lseek(file_handle, 0, PC_SEEK_MODE_END);
    int start = pc_lseek(file_handle, 0, PC_SEEK_MODE_START);
    int data_size = (end - start);

    // Allocate space and read data
    if (on_stack) *destination = mem_stack_alloc(data_size, stack);
    else *destination = mem_alloc(data_size, MEM_CAT_FILE);
    
    // Read :D
    pc_read(file_handle, data_size, *destination);
    *size = data_size;

    pc_close(file_handle);
    
    return 1;
}

#endif
