/*
 * mm.c
 *
 * Name: Vibhor Agarwal
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Also, read malloclab.pdf carefully and in its entirety before beginning.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
// #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

/* What is the correct alignment? */
#define ALIGNMENT 16

/* Basic constants */

#define WSIZE       4       /* Word and header/footer size(bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE   (1<<12) /*Extended heap by this amount (bytes) */ 
/* Static Inline Functions */

static size_t max(size_t x, size_t y){
    return ((x) > (y)? (x) : (y));
}

static size_t pack(int size, int alloc){
    return ((size) | (alloc));
}

static unsigned size_t get(const void* p){
   return (*(unsigned int *)(p));
}

static void put(const void* p, int val){
    return (*(unsigned int *)(p) = (val));
}

static size_t get_size(const void* p){
  return (get(p) & ~0x7); 
}

static size_t get_alloc(const void* p){
    return 8(get(p) & 0x1);
}

static void* HDRP(void* bp){
    ((char *)(bp) - WSIZE);
}

static void FTRP(void* bp){
    ((char *)(bp) + get_size(HDRP(bp)) - DSIZE)
}

static void NEXT_BKLP(void* bp){
    ((char *)(bp) + get_size(((char *)(bp) - WSIZE)));
}

static void PREV_BLKP(void* bp){
    ((char *)(bp) - get_size(((char *)(bp) - DSIZE)));
}

/*
 * Additional Functions
 */

/*
* coalesce
*/

static void *coalesce(void *bp){
    size_t prev_alloc = get_alloc(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = get_alloc(HDRP(NEXT_BKLP(bp)));
    size_t size = get_size(HDRP(bp));

    if (prev_alloc && next_alloc){  /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc){    /* Case 2 */
        size += get_size(HDRP(NEXT_BKLP(bp)));
        put(HDRP(bp), pack(size, 0));
        put(FTRP(bp), pack(size, 0));
    }

    else if (!prev_alloc && next_alloc){    /* Case 3 */
    size += get_size(HDRP(PREV_BLKP(bp)));
    put(FTRP(bp), pack(size, 0));
    put(HDRP(PREV_BLKP(bp)), pack(size, 0));
    bp = PREV_BLKP(bp);
    }

    else{
        size += get_size(HDRP(PREV_BLKP(bp))) + get_size(FTRP(NEXT_BKLP(bp)));
        put(HDRP(PREV_BLKP(bp)), pack(size, 0));
        put(FTRP(NEXT_BKLP(bp)), pack(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * extended_heap
 */

static void *extended_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words+ WSKZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initializew free block header/footer and the epologue header */
    put(HDRP(bp), pack(size, 0));       /* Free block header */
    put(FTRP(bp), pack(size, 0));       /* Free block footer */
    put(HDRP(NEXT_BKLP(bp)), pack(0,1));/* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * find_fits
 */
static void *find_fits(size_t asize){
    /* First-fit search */
    void *bp;
    for(bp = heap_listp; get_size(HDRP(bp))>0; bp = NEXT_BKLP(bp)){
        if (!get_alloc(HDRP(bp)) && (asize <= get_size(HDRP(bp)))){
            return bp;
        }
    }
    return NULL; /* No fit */
//#endif 
}

/*
 * place
 */
static void place(void *bp, size_t asize){
    size_t csize = get_size(HDRP(bp));

    if((csize - asize) >= (2*DSIZE)){
        put(HDRP(bp), pack(asize, 1));
        put(FTRP(bp), pack(asize, 1));
        bp = NEXT_BKLP(bp);
        put(HDRP(bp), pack(csize-asize, 0));
        put(FTRP(bp), pack(csize-asize), 0);
    }
    else{
        put(HDRP(bp), pack(csize, 1));
        put(FTRP(bp), pack(csize, 1));
    }
}



/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void)
{
    /* IMPLEMENT THIS */
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    put(heap_listp, 0);
    put(heap_listp + (1*WSIZE), pack(DSIZE, 1));
    put(heap_listp + (2*WSIZE), pack(DSIZE, 1));
    put(heap_listp + (3*WSIZE), pack(0,1));
    heap_listp += (2*WSIZE);

    if(extended_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    /* IMPLEMENT THIS */
    size_t asize;
    size_t extendsize;
    void *bp;


    if(size == 0)
        return NULL;


    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);


    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
}

/*
 * free
 */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    size_t size = get_size(HDRP(ptr));

    put(HDRP(ptr), pack(size, 0));
    put(FTRP(bp), pack(size,0));
    coalesce(bp);
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    return NULL;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}



/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */
#endif /* DEBUG */
    return true;
}
