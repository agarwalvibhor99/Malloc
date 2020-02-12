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
#define DEBUG

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

/*
 * Changing WSIZE and DSIZE for 64 bits system. All static Inline functions
 * here are referred textbook Computer Systems A Programmer's Perspective which were defined as MACROS
 * This Implementation referred from Textbook
 */

#define WSIZE       8       /* Word and header/footer size(bytes) */
#define DSIZE       16       /* Double word size (bytes) */
#define CHUNKSIZE   (1<<5) /*Extended heap by this amount (bytes) */ 
static void* heap_listp = NULL;
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
/* Creating struct for Doubly Linked List : Not yet implemented*/
typedef struct Node{
    //unsigned int data;
    struct Node *prev;
    struct Node *next;
}Node;
Node* head = NULL;
void push(struct Node** head, void *bp){                //Function push to add the address of free blocks to the doubley linked list 
    struct Node* newNode = (struct Node*)bp;            // Using bp instead of malloc which gives the address of free blocks
    newNode->next = (*head);                            //Always adding to the head therefore assigning next of newNode as current head
    newNode->prev = NULL;                               //Also since adding as first node setting previous of it as NULL
    if((*head)!= NULL){                                 //Checking if the list isn't empty, to build a connection with the old head setting its prev to newNode as that newNode will become the head now.
      (*head)->prev = newNode;
    }
    *head = newNode;                                    //Since adding always as first element, setting it as new head.\
}

void deleteNode(Node* del)  
{  
    /* Case when the list is empty or the node to be deleted is NULL */
    if (head == NULL || del == NULL)  
        return;  
  
    /* If node to be deleted is head node */
    if (head == del)  
        head = del->next;  
  
    //Since removing a node and middle, making sure the doubly linked list still remain connected with following to if conditions

    /* If del is not the last node which we check using its next then making a connection with the next node of it with the previous of del to not break the link*/
    if (del->next != NULL)  
        del->next->prev = del->prev;  
  
    if (del->prev != NULL)          //This condition to check we aren't deleting the first node as checking if prev isn't NULL
        del->prev->next = del->next;        //And then linking the node previous to which we are deleting with the node next to we are deleting 
 
    return;  
}

void *seg_list[10];
/*
    0 - {1}
    1 - {2}
    2 - {3,4}
    3 - {5.8}
    4 - {9.16}
    5 - {17,32}
    6 - {33,64}
    7 - {65,128}
    8 - (129,256)
    9 - {257,512}
    10 - {513,1024}
    11 - {1025,2048}
    12 - {2049,4096}
    13 - {4097,infinity}
*/
void selectList(void *bp){
    size_t size = get_size(HDRP(bp));
    if(size == 1){
        seg_list[0] = bp;
        head = seg_list[0];

    }
    elif(size == 2){
        seg_list[1] = bp;
        head = seg_list[1];
    }
    elif(size=>3 || size <= 4){
        seg_list[2] = bp;
        head = seg_list[2];
    
    }
    elif(size=>5 || size <= 8){
        seg_list[3] = bp;
        head = seg_list[3];
    }
    elif(size=>9 || size <= 16){
        seg_list[4] = bp;
        head = seg_list[4];
    }
    elif(size=>17 || size <= 32){
        seg_list[5] = bp;
        head = seg_list[5];
    }
    elif(size=>33 || size <= 64){
        seg_list[6] = bp;
        head = seg_list[6];
    }
    elif(size=>65 || size <= 128){
        seg_list[7] = bp;
        head = seg_list[7];
    }
    elif(size=>129 || size <= 256){
        seg_list[8] = bp;
        head = seg_list[8];
    }
    elif(size=>257 || size <= 512){
        seg_list[9] = bp;
        head = seg_list[9];
    }
    elif(size=>513 || size <= 1024){
        seg_list[10] = bp;
        head = seg_list[10];
    }
    elif(size=>1025 || size <= 2048){
        seg_list[11] = bp;
        head = seg_list[11];
    }
    elif(size=>2049 || size <= 4096){
        seg_list[12] = bp;
        head = seg_list[12];
    }
    elif(size=>4097){
        seg_list[13] = bp;
        head = seg_list[13];
    }
}
/*void delete(struct Node** head){
    (*head) = (*head)->next;

}*/


/* Static Inline Functions from Computer Systems A Programmer's Perspective Book*/

/* was defined as Macros in the book changed into a static inline function*/

static inline unsigned long max(long x, long y){
    return ((x) > (y)? (x) : (y));
}

/* This function packs/sets together the size and the allocation bit */
static inline unsigned long pack(unsigned long size, unsigned long alloc){
    return ((size) | (alloc));
}

/* reads and returns the word referenced by argument p */

static inline size_t get(const void* p){
   return (*(unsigned long *)(p));
}

/* stores val in the word pointed at by argument p */

static inline void put(const void* p, unsigned long val){
    (*(unsigned long *)(p) = (val));
}

/* This function returns the size from header */

static inline unsigned long get_size(const void* p){
    return (get(p) & ~0xF); 
}

/* This function returns the allocation bit from header */

static inline unsigned long get_alloc(const void* p){
    return (get(p) & 0x1);
}

/* This function takes the pointer to the header */

static inline char* HDRP(void* bp){
    return ((char *)(bp) - WSIZE);
}

/* This function takes the pointer to the footer */

static inline char* FTRP(void* bp){
    return ((char *)(bp) + get_size(HDRP(bp)) - DSIZE);
}

/* This function takes pointer to the point where the next data starts */

static inline char* NEXT_BKLP(void* bp){
    return ((char *)(bp) + get_size(((char *)(bp) - WSIZE)));
}

/* This function takes pointer to the point where the previous data starts */

static inline char* PREV_BLKP(void* bp){
    return ((char *)(bp) - get_size(((char *)(bp) - DSIZE)));
}

/*
 * Additional Functions Reference Textbook Computer Systems A Programmer's Perspective
 */
static void *extended_heap(size_t words){
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words*WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    put(HDRP(bp), pack(size, 0));
    //printf("%lu\n", *(unsigned long*)(heap_listp));       /* Free block header */
    put(FTRP(bp), pack(size, 0));       /* Free block footer */
    put(HDRP(NEXT_BKLP(bp)), pack(0,1));/* New epilogue header */ // size 0 allocation 1

    /* Coalesce if the previous block was free */
    return coalesce(bp);  
}
/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
static void *coalesce(void *bp){
    size_t prev_alloc = get_alloc(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = get_alloc(HDRP(NEXT_BKLP(bp)));
    size_t size = get_size(HDRP(bp));
  
    if (prev_alloc && next_alloc){  //Case 1 
        push(&head,bp);                         //Both prev and next block are already allocated, so just adding the new block to linkedlist
        return bp;
    }

    else if (prev_alloc && !next_alloc){     //Case 2 
        size += get_size(HDRP(NEXT_BKLP(bp)));
        deleteNode((Node*)(NEXT_BKLP(bp))); //Removing next free block as will be coalesced with the current one
        push(&head, bp);                        //Adding the starting position to the linkedlist of free block
        put(HDRP(bp), pack(size, 0));
        put(FTRP(bp), pack(size, 0));
    }

    else if (!prev_alloc && next_alloc){    //Case 3 
    size += get_size(HDRP(PREV_BLKP(bp)));
    //Remove the current free block and add the address of previous block as it is coalesced
    //remove(bp);
    //push(&head, PREV_BLKP(bp));                           //No change requried as previous block already free we just need to update it's header
    put(FTRP(bp), pack(size, 0));
    put(HDRP(PREV_BLKP(bp)), pack(size, 0));
    bp = PREV_BLKP(bp);
    }

    else{                               //Case 4
    	deleteNode((Node*)(NEXT_BKLP(bp)));                  //Updating header of previous block and deleting the next free block as three coalesced as one single free block
        size += get_size(HDRP(PREV_BLKP(bp))) + get_size(FTRP(NEXT_BKLP(bp)));
        put(HDRP(PREV_BLKP(bp)), pack(size, 0));
        put(FTRP(NEXT_BKLP(bp)), pack(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
static void *find_fits(size_t asize){
    /* First-fit search */
    void *bp;

    Node* node = (head);                                    //Iterating through the complete explicit linked list to find appropriate free block using while like we did in pointer lab
    while (node != NULL) {
    	bp = node;   
        if (!get_alloc(HDRP(bp)) && (asize <= get_size(HDRP(bp)))){
            return bp;
        }
        node = node->next;  
    }

 /*   
    for(bp = heap_listp; get_size(HDRP(bp))>0; bp = NEXT_BKLP(bp)){
        if (!get_alloc(HDRP(bp)) && (asize <= get_size(HDRP(bp)))){
            return bp;
        }
    }
    */
    return NULL; /* No fit */
//#endif 
}

/* Functions Reference Textbook Computer Systems A Programmer's Perspective */

static void place(void *bp, size_t asize){
    size_t csize = get_size(HDRP(bp));

    if((csize - asize) >= (2*DSIZE)){
        put(HDRP(bp), pack(asize, 1));
        put(FTRP(bp), pack(asize, 1));
        deleteNode((Node*)bp); //Deleting this block from doubly linked list as it is now allocated 
        bp = NEXT_BKLP(bp);                     //Going to the next splitted block to add to the doubly linked list
    	push(&head, bp);                           //Adding the new free block to doubly linked list 
        put(HDRP(bp), pack(csize-asize, 0));
        put(FTRP(bp), pack(csize-asize, 0));
    }
    else{
        put(HDRP(bp), pack(csize, 1));
        put(FTRP(bp), pack(csize, 1));
        deleteNode((Node*)bp);                  //Since entire block is being used and there is no splitting just removing the block from linkedlist
    }
}

static void place_realloc(void *bp, size_t asize){          //A new place for realloc function as we don't delete node here which was causing segfault on trace_file 6 while deleting node
      size_t csize = get_size(HDRP(bp));
      if((csize - asize) >= (2*DSIZE)){
        put(HDRP(bp), pack(asize, 1));
        put(FTRP(bp), pack(asize, 1));
        bp = NEXT_BKLP(bp);                     //No delete as we call this when newsize<size user request and in that case no use of linkedlist
        push(&head, bp);
        put(HDRP(bp), pack(csize-asize, 0));
        put(FTRP(bp), pack(csize-asize, 0));
        }
        else{
          put(HDRP(bp), pack(csize, 1));
          put(FTRP(bp), pack(csize, 1));
          //deleteNode((Node*)bp);         //Similarly not deleting for that particular reason which caused segfault when deleting as didn't existed in free explicit list         
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

/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
bool mm_init(void)
{  
    /* IMPLEMENT THIS */
  //  struct Node* head = NULL;
  head = NULL;
  if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    return false;
  put(heap_listp, 0);
  put(heap_listp + (1*WSIZE), pack(DSIZE, 1));
  put(heap_listp + (2*WSIZE), pack(DSIZE, 1));
  put(heap_listp + (3*WSIZE), pack(0,1));
  heap_listp += (2*WSIZE); 
  void *bp = extended_heap(CHUNKSIZE/WSIZE);
  if(bp == NULL)
    return false;
  //push(&head,bp);
  return true;
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    void *bp;


    if(size == 0)
        return NULL;


    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = align(size) + DSIZE;//DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);// use alignment function


    if((bp = find_fits(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    // if(*(head->next->next) == NULL){


   	// 	struct Node* newNode = (struct Node*)bp;
  		// newNode->next = (*head);
    // 	newNode->prev = NULL;
    // 	*head = newNode;

    // 	//*head->next = bp;     
    // }
    extendsize = max(asize, CHUNKSIZE);
    if((bp = extended_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * free
 */
/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
void free(void* ptr)
{
    /* IMPLEMENT THIS */
    size_t size = get_size(HDRP(ptr));  
    put(HDRP(ptr), pack(size, 0));
    put(FTRP(ptr), pack(size,0));
    coalesce(ptr);
    // deleteNode(ptr);   
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    //size_t extendsize;
    size_t asize;
    if(oldptr == NULL){         /* This call is equivalent to malloc */
       // printf("Calling Malloc");
        return malloc(size);
    }
    if(size == 0){
        //printf("Calling Free");
        free(oldptr);
        return NULL;
    }
    size_t old_size = get_size(HDRP(oldptr)) - 2*WSIZE;   // Storing the initial ptr size
    if(size == old_size){
        return oldptr;
    }
    else if (size<old_size){             // If new size is less than the old size
        
        if(size <= DSIZE)
            asize = 2*DSIZE;
        else
            asize = align(size) + DSIZE;

        place_realloc(oldptr, asize);
        return oldptr;
    }


    // else if( !get_alloc(HDRP(PREV_BLKP(oldptr))) && (get_size(HDRP(PREV_BLKP(oldptr)))) + get_size(HDRP(oldptr)) - 16) > size){           // To check previous block is free and adds up to enough size to realloc
    //     printf("On adjacent left");
    //     if(size <= DSIZE)
    //         asize = 2*DSIZE;
    //     else
    //         asize = align(size) + DSIZE;
    //     void *newPointer = PREV_BLKP(oldptr);
    //     place(PREV_BLKP(oldptr), asize);
    //     memcpy(PREV_BLKP(oldptr), oldptr, get_size(HDRP(oldptr)) - 16);
    //     return newPointer;

    // }
    // else if( !get_alloc(HDRP(NEXT_BKLP(oldptr))) && (get_size(NEXT_BKLP(HDRP(oldptr))) + get_size(HDRP(oldptr)) - 16) > size){       // To check next block is free and adds up to enough size to realloc
    //     printf("On adjacent right");
    //     if(size <= DSIZE)
    //         asize = 2*DSIZE;
    //     else
    //         asize = align(size) + DSIZE;
    //     void *newPointer = NEXT_BKLP(oldptr);
    //     place(newPointer, asize);
    //     memcpy(newPointer, oldptr, get_size(HDRP(oldptr)) - 16);
    //     return newPointer;

    // }
   
    void *newPointer = malloc(size);        /* If none of the above case calling malloc and copying the current data to the new position and freeing the position where the data is currently held */
    if(newPointer){
        memcpy(newPointer, oldptr, size);            
        free(oldptr);
        return newPointer;
     }

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
  void *bp;
  printf("\n\nHeap: \n");
  for(bp = heap_listp ; get_size(HDRP(bp))>0; bp = NEXT_BKLP(bp)){
  printf("\n H: %p \tbp: %p \t f: %p \tS: %lu \tA: %lu\n",HDRP(bp), bp, FTRP(bp), get_size(HDRP(bp)), get_alloc(HDRP(bp)));
  }
  printf("\n\nLinked List: \n");
  Node* node = (head);
  while (node != NULL) {
    bp = node;   
    printf("\n H: %p \tbp: %p \t f: %p \tS: %lu \tA: %lu\n",HDRP(bp), bp, FTRP(bp), get_size(HDRP(bp)), get_alloc(HDRP(bp)));
   // printf("\nAllocation Bit: %lu \tSinze: %lu", get_alloc(bp), get_size(HDRP(bp)));
   // if (!get_alloc(HDRP(bp)) && (asize <= get_size(HDRP(bp)))){
    //  return bp;
    //}
    node = node->next;  
  }                                 
#endif /* DEBUG */
    return true;
}
