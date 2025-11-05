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

typedef struct chunk {
    size_t size;          // size of the data area
    int free;             // 1 = free, 0 = allocated
    struct chunk* next;   // next chunk in linked list
} chunk_t;

static node_t* free_list = NULL;
static chunk_t* chunk_list_head = NULL;

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

    node_t* head = (node_t*)_arena_start;
    head->size = arena_size - sizeof(node_t);
    head->is_free = 1;
    head->fwd = NULL;
    head->bwd = NULL;
    free_list = head;  


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

//***************************************************** 

//Part 4
//*****************************************************

void* myalloc(size_t size) {
    if (_arena_start == NULL) { statusno = ERR_UNINITIALIZED; return NULL; }
    if (size == 0) { statusno = ERR_BAD_ARGUMENTS; return NULL; }

    node_t* current = free_list;

    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // Split if enough space
            if (current->size >= size + sizeof(node_t) + 1) {
                node_t* new_node = (node_t*)((char*)current + sizeof(node_t) + size);
                new_node->size = current->size - size - sizeof(node_t);
                new_node->is_free = 1;
                new_node->fwd = current->fwd;
                new_node->bwd = current;
                if (current->fwd) current->fwd->bwd = new_node;
                current->fwd = new_node;
                current->size = size;
            }
            current->is_free = 0;
            statusno = 0;
            return (void*)(current + 1);
        }
        current = current->fwd;
    }

    statusno = ERR_OUT_OF_MEMORY;
    return NULL;
}

void myfree(void* ptr) {
    if (_arena_start == NULL) { statusno = ERR_UNINITIALIZED; return; }
    if (ptr == NULL) { statusno = ERR_BAD_ARGUMENTS; return; }

    node_t* node = (node_t*)ptr - 1;  // header is just before the pointer
    node->is_free = 1;

    if (node->fwd && node->fwd->is_free) {
        node->size += sizeof(node_t) + node->fwd->size;
        node->fwd = node->fwd->fwd;
        if (node->fwd) node->fwd->bwd = node;
    }

    if (node->bwd && node->bwd->is_free) {
        node->bwd->size += sizeof(node_t) + node->size;
        node->bwd->fwd = node->fwd;
        if (node->fwd) node->fwd->bwd = node->bwd;
    }

    statusno = 0;
}
