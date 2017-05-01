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
#define MAX_FILE_PAGE (16*1024*1024/PAGE_SIZE)
#define MAX_PAGE 5//(8*1024*1024/PAGE_SIZE)
#define MAX_SWAP_PAGE (16*1024*1024/PAGE_SIZE)

/* double word alignment */
#define ALIGNMENT  8

/* define segaction */
struct sigaction sa;

/* place memory manager at the begining of each page */
typedef struct memoryManager_t{
    int pageId;
    char* mem_state;
    char* mem_heap; /* point to the first byte of page */
    char* mem_brk; /* points to the last byte of page puls 1*/
    char* mem_max_addr; /* max page addr plus 1 */
    char* heap_listp;
    pageDir* threadPtr;

}memoryManager;

memoryManager* pages[MAX_PAGE];

memoryManager* filePages[MAX_FILE_PAGE];

/* init memory when a thread has been created */

int GET_FREE_PAGE(memoryManager* p);
void SET_FREE_PAGE(memoryManager* p);
void SET_USE_PAGE(memoryManager* p);
int GET_IN_MEM_STATE(memoryManager* p);
void SET_IN_MEM_STATE(memoryManager* p);
void SET_IN_FILE_STATE(memoryManager* p);

int mem_init();
int find_free_frame();

/* interface provided to thread lib*/
void* myallocate(size_t size, char* fileName, int lineNo, int flag);
void mydeallocate(void *ptr, char* fileName, int lineNo, int flag);

void page_protect(int flag);

void setPageTableThreadPtr(pageDir* page_dir,int pageNum);

void page_release(int pageNum);

void vmem_printf(void);

void seg_fault_handle_init();
#endif /* memlib_h */
