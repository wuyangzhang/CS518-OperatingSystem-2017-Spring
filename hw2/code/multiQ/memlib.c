
/* memlib.c
 * We emulate the memory system as an array of bytes.
 *
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <signal.h>

#include "memlib.h"

/* -------------------------------------------------------------------------- */
/*              Marco Definitions for physical memory mapping               */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Doubleword size (bytes) */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#/* Set and get the availability bit */

inline char*
GET_FREE_PAGE(memoryManager* p){
    return ((*(unsigned int *)(p->mem_heap + 1)) | 0x1);
}

inline void
SET_FREE_PAGE(memoryManager* p){
    (*(unsigned int *)(p->mem_heap + 1)) & 0x0;
}

inline void
SET_USE_PAGE(memoryManager* p){
    (*(unsigned int *)(p->mem_heap + 1)) & 0x1;
}

/* --------------------------------------------------------------------------
 *                  Phrase 2: Virtual Memory
 *
 *                   0                 10 11                 0
 *  +----------------+-------------------+-------------------+
 *                      PAGE number            Offset
 *  +----------------+-------------------+-------------------+
 *                   virtual address
 *
 *                    +----------+
 *       .--------------->|Page Table|-----------.
 *      /                 +----------+            |
 *  0   |  10 11 0                            0   V  10 11 0
 * +---------+----+                          +---------+----+
 * |Page Nr  | Ofs|                          |Frame Nr | Ofs|
 * +---------+----+                          +---------+----+
 *  Virt Addr   |                             Phys Addr   ^
 *               \_______________________________________/
 *
 * -------------------------------------------------------------------------- */



/* Marco definitions that transfer malloc & free functions to self designed functions */
enum MEMORY_LIB_LEVEL{
    MEMORY_LIB = 0,
    MEMORY_THREAD
};

enum MEM{
    VIRTUAL_MEM = 0,
    PHYSICAL_MEM
};

//#define malloc(x)   myallocate(x, __FILE__, __LINE__, THREADREQ)
//#define free(x)     mydeallocate(x, __FILE__, __LINE__, THREADREQ)

void* page_sbrk(void* page, int incr);
static void *extend_heap(void* page, size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(void* page, size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);




/*
 * - mem_init
 * Initialize the space for all pages that
 * emulate a continuous 8 MB physical memory
 *
 *          Frame Format
 * ---------------------------------------
 *          page_available
 *          threadId;
 *          mem_heap;     (point to the first byte of page)
 *          mem_brk;      (points to the last byte of page puls 1)
 *          mem_max_addr  (max page addr plus 1 )
 *
 *          padding
 *          prologue header
 *          prologue footer
 *          epilogue header
 * 
 *              frame 1
 * ------------------------------------- <-------------- mem_heap & mem_brk
 * |                                   | 
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * ------------------------------------- <-------------- mem_max_addr
 * 
 *               frame 2
 * ------------------------------------- <-------------- mem_heap & mem_brk
 * |                                   | 
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * |                                   |
 * ------------------------------------- <-------------- mem_max_addr
 * 
*/
int
mem_init(){

    char *emulated_memory;
    
    /* memory align to the size of sysconf(_SC_PAGE_SIZE) */
    posix_memalign( (void*)(&emulated_memory), sysconf(_SC_PAGE_SIZE), MAX_PAGE*PAGE_SIZE);
    char* next = emulated_memory;
    
    /* init pages for physical pages */
    int i;
    for(i = 0; i < MAX_PAGE; i++){
        pages[i] = (memoryManager*)malloc(sizeof(memoryManager));
        pages[i]->pageId = i;
        pages[i]->mem_heap = next;
        pages[i]->mem_brk = pages[i]->mem_heap;
        pages[i]->mem_max_addr = pages[i]->mem_heap + PAGE_SIZE;
        pages[i]->heap_listp = 0;
        next += PAGE_SIZE;
    }
    
    return 0;
}

/* -thread_mem_init
 * init memory when a thread has been create
 */
int thread_mem_init(int threadId){
    
    return 0;
}

/*
 *  page_init: initialize a new page
 * *  *  frame format:
 *              frame 1
 * ------------------------------------- <-------------- mem_heap heap_listp
 * |                                   | 4 Byte
 * |                                   | <-------------- prologue header
 * |                                   | 8 Byte
 * |                                   | <-------------- progogue footer 
 * |                                   | 12 Byte
 * |                                   | <-------------- epilogue header
 * |                                   |
 * |                                   |
 * |                                   |
 * ------------------------------------- <-------------- mem_max_addr
 * 
 */

int
page_init(memoryManager* page){
    
    SET_USE_PAGE(page);
    /*cast to physical page or virtual page */


    if((page->heap_listp = page_sbrk(page, 4*WSIZE)) == (void*) -1){
        return -1;
    }

    PUT(page->heap_listp, 0);
    /* Prologue header */
    PUT(page->heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    /* Prologue footer */
    PUT(page->heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    /* Epilogue header */
    PUT(page->heap_listp + (3*WSIZE), PACK(0, 1));
    /* update heap_listp, pointing to the first block */
    page->heap_listp += (2*WSIZE);
    
    /* Extend the empty heap with a free block of BLOCK bytes
    if (extend_heap(page, BLOCK_SIZE/WSIZE) == NULL)
        return -1;
    */
    return 0;
}

/*
 * getNextPage - Get the next page 
 */
memoryManager*
getNextPage(memoryManager* page){
    if(page->pageId >= MAX_PAGE - 1){
        return NULL;
    }else{
        return pages[page->pageId + 1];
    }
}

void*
page_sbrk(void* page, int incr){
    char *old_brk = ((memoryManager*)page)->mem_brk;
    
    if((incr < 0 ) || ((((memoryManager*)page)->mem_brk + incr) > ((memoryManager*)page)->mem_max_addr)){
        return (void*) -1;
    }
    
    ((memoryManager*)page)->mem_brk += incr;
    return (void*) old_brk;
}

/*
 * allocate_frame - Find a void frame
 */
int
allocate_frame(){
    int i;
    for(i = 0; i < MAX_PAGE; i++){
        if(GET_FREE_PAGE(pages[i]) == 0){
            return i;
        }
    }
    perror("Error: cannot find available page!\n");
    return -1;
}

/*
    -swap_frame
 */
void
swap_frame(memoryManager* oldPage, memoryManager* newPage){
    SET_USE_PAGE(newPage);
    newPage->mem_brk = newPage->heap_listp + (oldPage->heap_listp - oldPage->mem_brk);
    memcpy(newPage-> mem_heap, oldPage->mem_heap, PAGE_SIZE);
}

/*
    -page protect
 *  protect the pages when they are swapped out
 */

void
page_protect(int* usedPage, int flag){
    
    int i = 0;
    while(usedPage[i] != -1){
        if(flag == 0){
           mprotect(pages[usedPage[i]]->mem_heap, PAGE_SIZE, PROT_NONE);
        }else if(flag == 1){
            mprotect(pages[usedPage[i]]->mem_heap, PAGE_SIZE, PROT_READ | PROT_WRITE);
        }else{
            perror("unrecognized flag!\n");
            return;
        }
        i++;
    }
    
}

static void 
handler(int sig, siginfo_t *si, void *unused){
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
}


/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */
void *
page_malloc(memoryManager* page, size_t size){
    size_t asize;      /* Adjusted block size */
    char *bp;
    
    if (page->heap_listp == 0){
        page_init(page);
    }
    /* Ignore spurious requests */
    if (size <= 0)
        return NULL;
    
    /* Decline if requests for more than page size */
    if (size > PAGE_SIZE)
        return NULL;
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // align SIZE
    
    /* Search the free list for a fit */
    if ((bp = find_fit(page, asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    
    /* No fit found. Find more memory from the current page */
    if ((bp = extend_heap(page, asize/WSIZE)) != NULL){
         place(bp, asize);
         return bp;
    }
    
    /* Allocate a new frame. Check the availability of the upper frame*/
    if(GET_FREE_PAGE(getNextPage(page)) != 0){
        /* swap the upper frame */
        int frame;
        if((frame = allocate_frame()) == -1){
            /* TODO swap the frame */
        }else{
            /* swap the upper frame to this new frame */
            if(pages[frame]->heap_listp == 0){
                //page_init(pages[frame]);
            }
            //swap_frame(getNextPage(page), pages[frame]);
        }
       
    }
    
    /* update a new assigned current page for the current thread */
    //updateFrame(getCurrentRunningThread());
    return page_malloc(getNextPage(page), size);
   
}

/*
 *  myallocate - Wrap page allocate function for the thread interface
 */
void*
myallocate(size_t size, char* fileName, int lineNo, int flag){
    printf("thread %d allocates %zu\n ", getCurrentRunningThread()->_self_id, size);
    if(flag == MEMORY_LIB){
        
    }else if(flag == MEMORY_THREAD){
        int page = getCurrentRunningThread()->currentPage;
        return page_malloc(pages[page], size);
    }else{
        perror("Error: Unrecognized flag!\n");
    }

    return NULL;
}

/*
 * mm_free 
 */
void
page_free(void* page, void *bp){
    if(bp == 0)
        return;
    
    size_t size = GET_SIZE(HDRP(bp));
    
    if ( ((memoryManager*)page)->heap_listp == 0){
        page_init(((memoryManager*)page));
    }
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 *  wrap page deallocate function for the thread interface
 */
void
mydeallocate(void *ptr, char* fileName, int lineNo, int flag){
    //unsigned int pageNum = virtualPageMapping(getVirtualPageOfRunningThread());
    return page_free(pages[1], ptr);}

/*
 * page_release- release page resources to other threads
 */
void page_release(int pageNum){
    SET_FREE_PAGE(pages[pageNum]->mem_heap);
}

/*
 * Extend heap with free block and return its block pointer
 * Note 8MB memory is the whole memory size, heap is able to
 * gradually extends to a larger size
 */

static void *extend_heap(void* page, size_t words)
{
    char *bp;
    size_t size;
    
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = page_sbrk(page, size)) == -1)
        return NULL;
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) {            /* Case 1 */
        return bp;
    }
    
    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }
    
    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    return bp;
}


/*
 * Place block of asize bytes at start of free block bp
 * and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    
    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */

static void *find_fit(void* page, size_t asize)
{
    
    /* First fit search */
    void *bp;
    
    for (bp = ((memoryManager*)page)->heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* No fit */
    
}

static void printblock(void *bp)
{
    size_t hsize, halloc, fsize, falloc;
    
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));
    
    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }
    
    printf("%p: header: [%p:%c] footer: [%p:%c]\n", bp,
           hsize, (halloc ? 'a' : 'f'),
           fsize, (falloc ? 'a' : 'f'));
}

/*
 * -update_address_shuffle_page
 *  Based on the input address, the memory should track the page including this address.
 * If this address is protected by other threads, the memory manager will indicate the 
 * current address of this address.
 *   0x7f79c79f5018, page start 0x7f79c79f5008

 */
void*
update_address_shuffle_page(void* addr){
    
}

void
mem_printf(void){
    int i;
    for(i = 0; i < MAX_PAGE; i++){
        printf(" mem heap %p, brk %p, max %p heaplist%p\n", pages[i]->mem_heap, pages[i]->mem_brk, pages[i]->mem_max_addr, pages[i]->heap_listp);
    }
}

void
pageProtect_handler(int sig, siginfo_t *si, void* unused){
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    sleep(2);
}

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)
void test_pageProtect(){
    char* emulated_memory;
    posix_memalign( (void*)(&emulated_memory), sysconf(_SC_PAGE_SIZE),PAGE_SIZE);
    
    printf("start addr %p\n", emulated_memory);
    
    
    printf("%p\n", pages[1]->mem_heap);
    int r;
    if( ( r = mprotect(pages[1]->mem_heap, PAGE_SIZE, PROT_NONE)) != 0){
        handle_error("mprotect");
    }

    *pages[1]->heap_listp = 'a';
    printf("%s\n", pages[1]->heap_listp );

}
 int
 main(){
     mem_init();
     //mem_printf();
     char *p = page_malloc(pages[1], PAGE_SIZE - 32);

     char *s = page_malloc(pages[1], 128);

     swap_frame(pages[1], pages[2]);
     
     struct sigaction sa;
     sa.sa_flags = SA_SIGINFO;
     sigemptyset(&sa.sa_mask);
     sa.sa_sigaction = pageProtect_handler;
     if(sigaction(SIGSEGV, &sa, NULL) == -1){
         printf("Fatal error setting up signal handler\n");
         exit(EXIT_FAILURE);
     }
     
     
     
     
     test_pageProtect();
     //printf("page %p, block %p\n",  pages[1]->mem_brk, p);

     //printf("page %p, block %p\n",  pages[2]->mem_brk, s);
     
     //page_malloc(pages[1], 11);
     //page_malloc(pages[1], PAGE_SIZE);
     return 0;
}

