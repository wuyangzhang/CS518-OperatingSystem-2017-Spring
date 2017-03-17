/* memlib.c
 * We emulate the memory system as an array of bytes.
 *
 *
 */

#ifndef memlib_h
#define memlib_h

#include <unistd.h>

#include "my_pthread_t.h"

/* Emulated Memory */

#define BLOCK_SIZE  64
#define PAGE_SIZE 4096
#define MAX_PAGE (8*1024*1024/PAGE_SIZE)
#define MAX_SWAP_PAGE (16*1024*1024/PAGE_SIZE)


/* double word alignment */
#define ALIGNMENT  8

/* place memory manager at the begining of each page */
typedef struct memoryManager_t{
    int pageId;
    char* mem_heap; /* point to the first byte of page */
    char* mem_brk; /* points to the last byte of page puls 1*/
    char* mem_max_addr; /* max page addr plus 1 */
    char* heap_listp;
}memoryManager;

static memoryManager* pages[MAX_PAGE];

/* init memory when a thread has been created */

int mem_init();
int allocate_frame();

/* interface provided to thread lib*/
void* myallocate(size_t size, char* fileName, int lineNo, int flag);
void mydeallocate(void *ptr, char* fileName, int lineNo, int flag);

void page_release(int pageNum);

void vmem_printf(void);
#endif /* memlib_h */
