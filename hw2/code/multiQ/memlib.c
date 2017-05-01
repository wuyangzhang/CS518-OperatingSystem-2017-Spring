
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

int
GET_FREE_PAGE(memoryManager* p){
    return ((int)p->mem_state) & 0x01;
}

void
SET_FREE_PAGE(memoryManager* p){
    p->mem_state = ((int)p->mem_state) & 0xFFFE;
}

void
SET_USE_PAGE(memoryManager* p){
    p->mem_state = ((int)p->mem_state) | 0x01;
}

int
GET_IN_MEM_STATE(memoryManager* p){
    return ((int)p->mem_state) & 0x02;
}

void
SET_IN_MEM_STATE(memoryManager* p){
    p->mem_state = ((int)p->mem_state) | 0x02;
}

void
SET_IN_FILE_STATE(memoryManager* p){
    p->mem_state = ((int)p->mem_state) & 0xFFFD;
}

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)
void
PROTECT_PAGE(memoryManager* p, int flag){
    if(flag == 0){
        if( (mprotect(p->mem_heap, PAGE_SIZE, PROT_NONE)) != 0){
            handle_error("mprotect");
        }
    }else if(flag == 1){
        if( (mprotect(p->mem_heap, PAGE_SIZE, PROT_READ | PROT_WRITE)) != 0){
            handle_error("mprotect");
        };
    }else{
        perror("unrecognized flag!\n");
        return;
    }
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
        pages[i]->threadPtr = (pageDir*)malloc(sizeof(pageDir));
        memset(pages[i]->threadPtr,0,sizeof(pageDir));
        pages[i]->pageId = i;
        pages[i]->mem_heap = next;
        pages[i]->mem_state = next;
        pages[i]->mem_brk = pages[i]->mem_heap;
        pages[i]->mem_max_addr = pages[i]->mem_heap + PAGE_SIZE;
        pages[i]->heap_listp = 0;
        next += PAGE_SIZE;
    }
    
    for(i = 0; i < MAX_FILE_PAGE; i++){
        filePages[i] = (memoryManager*)malloc(sizeof(memoryManager));
        filePages[i]->threadPtr = (pageDir*)malloc(sizeof(pageDir));
        memset(filePages[i]->threadPtr,0,sizeof(pageDir));
        filePages[i]->pageId = i;
        filePages[i]->mem_heap = i*PAGE_SIZE;
        filePages[i]->mem_state = 0;
        filePages[i]->mem_brk = i*PAGE_SIZE;
        filePages[i]->mem_max_addr = filePages[i]->mem_heap + PAGE_SIZE;
        filePages[i]->heap_listp = 0;
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
    SET_USE_PAGE(page);
    return 0;
}

int 
file_init(){
    int X = 16*1024*1024-1;
    FILE *fp = fopen("hardDrive", "r+");
    fseek(fp, X , SEEK_SET);
    fputc('\0', fp);

    fseek(fp, 0 , SEEK_SET);
    char test[] = "helloworld!";
    char* t = test;
    fwrite(t, sizeof(char), sizeof(test), fp);

    fclose(fp);
    return 0;
}

/*
 * mem_to_file - Move page from memory to hard disk(file)
 */
void
mem_to_file(memoryManager* memPage, int pageNum){
    
    int mem_p_offset = memPage->mem_brk - memPage->mem_heap;
    PROTECT_PAGE(memPage, 1);
    if(fileWriter(memPage->mem_heap, mem_p_offset, pageNum)==-1){
        printf("page offset not in the correct range.\n");
        return;
    }
    filePages[pageNum]->heap_listp = memPage->heap_listp;
    filePages[pageNum]->mem_brk = filePages[pageNum]->mem_heap+mem_p_offset;
    filePages[pageNum]->mem_state = memPage->mem_state;
    filePages[pageNum]->threadPtr = memPage->threadPtr;
    filePages[pageNum]->threadPtr->page_redirect = filePages[pageNum]->pageId;
    filePages[pageNum]->threadPtr->in_memory_state = 0;
    
    memPage->threadPtr = (pageDir*)malloc(sizeof(pageDir));
    memset(memPage->threadPtr,0,sizeof(pageDir));
    memPage->mem_state = memPage->mem_heap;
    memPage->mem_brk = memPage->mem_heap;
    memPage->heap_listp = 0;
}

void
file_to_mem(memoryManager* filePage, memoryManager* memPage){
    int page_addr_hard = filePage->pageId*PAGE_SIZE;
    FILE *fp = fopen("hardDrive", "r+");
    fseek(fp, page_addr_hard , SEEK_SET);
    int page_offset = (int)(filePage->mem_brk-filePage->mem_heap);
    char *buf = (char*)malloc(page_offset);
    fread(buf, sizeof(char),page_offset, fp);
    fclose(fp);
    
    memcpy(memPage->mem_heap, buf, page_offset);
    memPage->mem_state = filePage->mem_state;
    memPage->heap_listp = filePage->heap_listp;
    memPage->threadPtr = filePage->threadPtr;
    memPage->threadPtr->in_memory_state = 1;
    memPage->threadPtr->page_redirect = memPage->pageId;
    memPage->mem_brk = memPage->mem_heap + page_offset;
    
    // Clean page in hard drive
    filePage->heap_listp = 0;
    filePage->mem_brk = filePage->mem_heap;
    filePage->mem_state = filePage->mem_heap;
    filePage->threadPtr = (pageDir*)malloc(sizeof(pageDir));
    memset(filePage->threadPtr,0,sizeof(pageDir));
    
    char* c = (char*)malloc(PAGE_SIZE);
    memset(c,0,PAGE_SIZE);
    fileWriter(c, PAGE_SIZE, filePage->pageId);
}

int 
fileWriter(char* c, int char_offset, int pageNum){
    
    if(char_offset>PAGE_SIZE||char_offset<0) {
        return -1;
    }
    
    FILE *fp = fopen("hardDrive", "r+");

    fseek(fp, pageNum*PAGE_SIZE , SEEK_SET);
    fwrite(c, sizeof(char), char_offset, fp);
    fclose(fp);
    
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
find_free_frame(){
    int i;
    for(i = 0; i < MAX_PAGE; i++){
        if(pages[i]->threadPtr->in_use_state == 0){
            return i;
        }
    }
    printf(("Memory Full!\n"));
    return -1;
}

int
find_free_file_frame(){
    int i;
    for(i = 0; i < MAX_FILE_PAGE; i++){
        if(filePages[i]->threadPtr->in_use_state == 0){
            return i;
        }
    }
    perror("Error: Hard Drive Full!\n");
    return -1;
}

void setPageTableThreadPtr(pageDir* page_dir, int pageNum){
    pages[pageNum]->threadPtr = page_dir;
}

/*
    -swap_frame
 */
void
swap_frame(memoryManager* oldPage, memoryManager* newPage){
    
    int old_offset = oldPage->mem_brk - oldPage->mem_heap;
    char* swapSpace = (char*)malloc(old_offset);
    
    memcpy(swapSpace, oldPage->mem_heap, old_offset);
    
    memcpy(oldPage->mem_heap, newPage->mem_heap, newPage->mem_brk - newPage->mem_heap);
    oldPage->mem_brk = oldPage->mem_heap + (newPage->mem_brk - newPage->mem_heap);
    
    memcpy(newPage->mem_heap, swapSpace, old_offset);
    newPage->mem_brk = newPage->mem_heap + old_offset;
    
    free(swapSpace);
    
    pages[newPage->pageId]->threadPtr = pages[oldPage->pageId]->threadPtr;
    pages[newPage->pageId]->threadPtr->page_redirect = newPage->pageId;
    
    char* tmpstate = oldPage->mem_state; 
    oldPage->mem_state = newPage->mem_state;
    newPage->mem_state = tmpstate;
    
    
    char* tmpheap_listp = oldPage->heap_listp; 
    oldPage->heap_listp = newPage->heap_listp;
    newPage->heap_listp = tmpheap_listp;
    
    
    //pages[oldPage->pageId]->threadPtr = getCurrentRunningThread()->pageTable[oldPage->pageId];
    allocateFrame(getCurrentRunningThread(), oldPage->pageId);  
    
}

/*
    -page protect
 *  protect the pages when they are swapped out
 */

void
page_protect(int flag){
    printf("%d protect page for thread %d\n", flag, getCurrentRunningThread()->_self_id);
    
    printf("Thread %d can use page: ",getCurrentRunningThread()->_self_id);
    for(int i = 0; i < MAX_PAGE; i++){ 
        if(getCurrentRunningThread()->pageTable[i]->in_use_state == 0 || getCurrentRunningThread()->pageTable[i]->in_memory_state == 0 || getCurrentRunningThread()->pageTable[i]->page_redirect != i ){
            PROTECT_PAGE(pages[i], flag);
        }else{
            printf("%d. ",i);
        } 
    }
    printf("\n");
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
    
    
    /* 
     * The current requested page is full.
     * Allocate a new frame. Check the availability of the upper frame
     */
    memoryManager* nextPage = getNextPage(page);
    if(GET_FREE_PAGE(nextPage) == 0){
        //updateFrame(getCurrentRunningThread(), nextPage->pageId);
        allocateFrame(getCurrentRunningThread(), nextPage->pageId);
        return page_malloc(nextPage, size);
    }
    
    /* 
     * The next page is not available, but can find other available pages
     * swap the upper frame to this new frame 
     */
    int frame;
    if((frame = find_free_frame()) != -1){
        PROTECT_PAGE(nextPage,1);
        PROTECT_PAGE(pages[frame],1);
        //swap the next to this new frame
        swap_frame(nextPage, pages[frame]);
        //use this next frame
        PROTECT_PAGE(pages[frame],0);
        return page_malloc(nextPage, size);
    }
    int pageNum;
    if((int pageNum = find_free_file_frame())!=-1){
        mem_to_file(nextPage);
        return page_malloc(nextPage, size);
    }
    
    
    
    /* update a new assigned current page for the current thread */
    //updateFrame(getCurrentRunningThread());
    
    return NULL;
}

/*
 *  myallocate - Wrap page allocate function for the thread interface
 */
void*
myallocate(size_t size, char* fileName, int lineNo, int flag){
    
    
    if(flag == MEMORY_LIB){
        
    }
    
    if(flag == MEMORY_THREAD){
        int page = getCurrentRunningThread()->currentUsePage;
        printf("thread %d allocates %zu in page %d\n ", getCurrentRunningThread()->_self_id, size, page);
        /* initiate a new page for this thread 
        if(page == -1){
            allocateFrame(getCurrentRunningThread());
        }
        */
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
    SET_FREE_PAGE(pages[pageNum]);
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
        //printf("bp: %p, size: %p, next: %p, alloc: %d, blocksize: %d\n",bp,GET_SIZE(HDRP(bp)),NEXT_BLKP(bp),GET_ALLOC(HDRP(bp)),GET_SIZE(HDRP(bp)));
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
    
    printf("%p: header: [%zd:%c] footer: [%p:%c]\n", bp,
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

int
getPageNumFromAddr(void* addr){
    int offset = (int)addr - (int)pages[0]->mem_heap;
    return offset/4096;
}

void
pageProtect_handler(int sig, siginfo_t *si, void* unused){
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    int pageNum = getPageNumFromAddr(si->si_addr);
    printf("Page Number is %d.\n",pageNum);
    if(GET_FREE_PAGE(pages[pageNum])==0){
        PROTECT_PAGE(pages[pageNum], 1);
    }else if(getCurrentRunningThread()->pageTable[pageNum]->in_memory_state==0){
        PROTECT_PAGE(pages[pageNum],1);
        mem_to_file(pages[pageNum]);
        file_to_mem(filePages[getCurrentRunningThread()->pageTable[pageNum]->page_redirect], pages[pageNum]);
    }else if(getCurrentRunningThread()->pageTable[pageNum]->page_redirect!=pageNum){
        int newPageNum = getCurrentRunningThread()->pageTable[pageNum]->page_redirect;
        PROTECT_PAGE(pages[pageNum],1);
        PROTECT_PAGE(pages[newPageNum],1);
        swap_frame(pages[pageNum], pages[getCurrentRunningThread()->pageTable[pageNum]->page_redirect]);
        PROTECT_PAGE(pages[newPageNum],0);
    }
    sleep(2);
}

void 
seg_fault_handle_init(){
    
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = pageProtect_handler;
    if(sigaction(SIGSEGV, &sa, NULL) == -1){
        printf("Fatal error setting up signal handler\n");
        exit(EXIT_FAILURE);
    }
}

void
mem_printf(void){
    int i;
    for(i = 0; i < MAX_PAGE; i++){
        printf(" mem heap %p, brk %p, max %p heaplist%p\n", pages[i]->mem_heap, pages[i]->mem_brk, pages[i]->mem_max_addr, pages[i]->heap_listp);
    }
}


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

void test_swap(){
    mem_init();
    int *p = page_malloc(pages[1], 100);
    *p = 100;
    int *q = page_malloc(pages[2], 200);
    *q = 200;
    
    //printf("page1 p %p, page2 q %p\n", pages[1]->mem_heap, pages[2]->mem_heap);

    printf("page1 p %d, page2 q %d\n", *p, *q);

    printf("page1 brk %d, page2 brk %d\n", pages[1]->mem_brk - pages[1]->mem_heap, pages[2]->mem_brk - pages[2]->mem_heap);
    
    swap_frame(pages[1], pages[2]);
    
    printf("page1 p %d, page2 q %d\n", *p, *q);
    printf("page1 brk %d, page2 brk %d\n", pages[1]->mem_brk - pages[1]->mem_heap, pages[2]->mem_brk - pages[2]->mem_heap);
    
    
   int test =  (( ((int)p) % 4096 )+ pages[2]->mem_heap);
   printf("test %p", test);
      
}

void test_check_state(){
    int x = GET_FREE_PAGE(pages[1]);
    printf("%d\n",x);
    SET_USE_PAGE(pages[1]);
    x = GET_FREE_PAGE(pages[1]);
    printf("%d\n",x);
    SET_USE_PAGE(pages[1]);
    x = GET_FREE_PAGE(pages[1]);
    printf("%d\n",x);
    SET_FREE_PAGE(pages[1]);
    x = GET_FREE_PAGE(pages[1]);
    printf("%d\n",x);
    SET_FREE_PAGE(pages[1]);
    x = GET_FREE_PAGE(pages[1]);
    printf("%d\n",x);
    
    x = GET_IN_MEM_STATE(pages[1]);
    printf("%d\n",x);
    SET_IN_MEM_STATE(pages[1]);
    x = GET_IN_MEM_STATE(pages[1]);
    printf("%d\n",x);
    SET_USE_PAGE(pages[1]);
    SET_IN_MEM_STATE(pages[1]);
    x = GET_IN_MEM_STATE(pages[1]);
    printf("%d\n",x);
    SET_IN_FILE_STATE(pages[1]);
    x = GET_IN_MEM_STATE(pages[1]);
    printf("%d\n",x);
    SET_IN_FILE_STATE(pages[1]);
    x = GET_IN_MEM_STATE(pages[1]);
    printf("%d\n",x);
}

 int
 man(){
    
     //test_swap();
     
     file_init();
     

    mem_init();
    int i = find_free_file_frame();
    printf("%d\n",i);
    
    
     /*
      
     struct sigaction sa;
     sa.sa_flags = SA_SIGINFO;
     sigemptyset(&sa.sa_mask);
     sa.sa_sigaction = pageProtect_handler;
     if(sigaction(SIGSEGV, &sa, NULL) == -1){
         printf("Fatal error setting up signal handler\n");
         exit(EXIT_FAILURE);
     }
     */
     
     
      //printf("page %p, block %p\n",  pages[1]->mem_brk, p);

     //printf("page %p, block %p\n",  pages[2]->mem_brk, s);
     
     //page_malloc(pages[1], 11);
     //page_malloc(pages[1], PAGE_SIZE);
     return 0;
}

