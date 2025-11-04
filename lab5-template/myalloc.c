#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "myalloc.h"

int statusno = 0;
static void* _arena_start = NULL; // pointer to the start of the memory arena
static void* _arena_end = NULL; // pointer to the end of the memory arena
static size_t arena_size = 0;

//Part 1
//*****************************************************


// myinit() should initialize the memory allocator with an arena of at least
int myinit(size_t size){
    printf("Initializing arena\n");
    printf("...requested size %zu bytes\n", size);

    // gets the sysytem's memory page size
    // Memory is managed by the OS in chunks called pages.
    // mmap() can only allocate memory in whole pages.
    size_t page_size = getpagesize();
    printf("...pagesize is %zu bytes\n", page_size);

    // Chech the size exeeds MAX_ARENA_SIZE and zero or negative
    if(size > MAX_ARENA_SIZE || size <= 0){
        statusno = ERR_BAD_ARGUMENTS;
        return ERR_BAD_ARGUMENTS;
    }

    // Adjust size to be a multiple of page_size
    // This line rounds up the requested size to 
    // the nearest multiple of the page siz
    /* 
    For eg:-
    If page_size = 4096 and size = 1 byte
    size_adjust = 1 + 4096 - 1 = 4096
    size = 4096 - (4096 % 4096) = 4096
    4096 is a multiple of 4096
    so it allocates at least one full page.
    */
    size_t adjusted_size = ((size + page_size - 1) / page_size) * page_size;
    printf("...adjusted size to %zu bytes\n", adjusted_size);
    size = adjusted_size;

    // call mmap to allocate memory
    printf("...mapping area with mmap()\n");
    // NULL: let the OS choose the address in memory
    // size: how much memory to allocate
    // PROT_READ|PROT_WRITE: memory can be read and written
    // MAP_PRIVATE|MAP_ANONYMOUS: not backed by any file
    // -1 and 0: Required when using MAP_ANONYMOUS — means “no file descriptor”.
    _arena_start = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    if (_arena_start == MAP_FAILED) {
        perror("...mmap failed\n");
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    arena_size = adjusted_size;
    _arena_end = (char*)_arena_start + arena_size;

    printf("...arena starts at %p\n", _arena_start);
    printf("...arena ends at %p\n", _arena_end);

    return arena_size;

}

int mydestroy() {
    printf("Destroying Arena:\n");
    if (_arena_start != NULL) {
        printf("...unmapping arena with munmap()\n");
        if (munmap(_arena_start, arena_size) != 0) {
            perror("...munmap failed");
            statusno = ERR_SYSCALL_FAILED;
            return ERR_SYSCALL_FAILED;
        }

        _arena_start = NULL;
        _arena_end = NULL;
        arena_size = 0;

        return 0;
    } else {
        statusno = ERR_UNINITIALIZED;
        return ERR_UNINITIALIZED; 
    }
}
