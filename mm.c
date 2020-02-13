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
static inline unsigned long get_size(const void* p);
static inline char* HDRP(void* bp);
void *getsegListhead(int segListno);
int addtosegList(size_t size);
/* Creating struct for Doubly Linked List : Not yet implemented*/
typedef struct Node{
    struct Node *prev;
    struct Node *next;
}Node;
Node* head = NULL;
Node *segList[9];

void push(void *bp){
  size_t addNodeSize = get_size(HDRP(bp));
  int segListno = addtosegList(addNodeSize);
  head = segList[segListno];
  struct Node* newNode = (struct Node*)bp;
  newNode->next = (segList[segListno]);			//Instead of using head variable directly using the head address from segList. Earlier approach gave error
  newNode->prev = NULL;
  if(segList[segListno]!= NULL){
    (segList[segListno])->prev = newNode;
  }
  segList[segListno] = newNode;
}

/* Used when was using doubly linked list
void push(struct Node** head, void *bp){
    struct Node* newNode = (struct Node*)bp;
    newNode->next = (*head);
    newNode->prev = NULL;
    if((*head)!= NULL){
      (*head)->prev = newNode;
    }
    *head = newNode;
}
*/
void deleteNode(Node* del)  
{  
    size_t deleteNodeSize = get_size(HDRP(del));
    int segListno = addtosegList(deleteNodeSize);
    head = segList[segListno];
    //head = getsegListhead(segListno);
    /* base case */
    //getsegListhead(del);
   // if (head == NULL || del == NULL)  
     //   return;  
  
    /* If node to be deleted is head node */
    if(segList[segListno] == del)
      segList[segListno] = del->next;
    
    /*if (head == del)  
        head = del->next;  
  */
    /* Change next only if node to be  
    deleted is NOT the last node */
    if (del->next != NULL)  
        del->next->prev = del->prev;  
  
    /* Change prev only if node to be  
    deleted is NOT the first node */
    if (del->prev != NULL)  
        del->prev->next = del->next;  
}

//Node *segList[14];

/* Helper Functions for Segregated Linked list 
 * Majorly changed place and coalesce function to switch to segregated linked list. Find fit changed to iterate through segList heads now.
 * Tried using the push and delete function with directly passing head address as parameter but because of error switched to just bp for push function
 * Inside push function itself making use of the following helper function to find address of head for segList. 
 * In my coalesce and place function have push() at several positions as while running some traces, realised was calling push too early which was not updating my segList correctly
 */
//Returns the head address of the particular segList based on the parameter segListno
void *getsegListhead(int segListno){
  //unsigned long size = get_size(HDRP(bp));
  if(segListno == 0){
   /* if(segList[0]==NULL){
      segList[0] = bp;
    }*/
    head = segList[0];
  }
  else if(segListno == 1){
   /* if(segList[1] == NULL){
      segList[1] = bp;
    }*/
    head = segList[1];
  }
  else if(segListno == 2){
    /*
    if(segList[2] == NULL){
      segList[2] = bp;
    }*/
    head = segList[2];
  }
  else if(segListno == 3){
  /*  if(segList[3] == NULL){
      segList[3] = bp;
    } */
    head = segList[3];
  }
  else if(segListno == 4){
    /*if(segList[4] == NULL){
      segList[4] = bp;
    }*/
    head = segList[4];
  }
  else if(segListno == 5){
    /*if(segList[5] == NULL){
      segList[5] = bp;
    }*/
    head = segList[5];
  }
  else if(segListno == 6){
   /* if(segList[6] == NULL){
      segList[6] = bp;
    }*/
    head = segList[6];
  }
  else if(segListno == 7){
    /*if(segList[7] == NULL){
      segList[7] = bp;
    }*/
    head = segList[7];
  }
  else if(segListno == 8){
    head = segList[8];
  }
  return NULL;
 /* else if(segListno == 9){
    head = segList[9];
  }
  else if(segListno == 10){
    head = segList[10];
  }
  else if(segListno == 11){
    head = segList[11];
  }
  else if(segListno == 12){
    head = segList[12];
  }
  else if(segListno == 13){
    head = segList[13];
  }
  return NULL;
  */
}

//Function to determine the ith number of the segregated linkedlist to be used based on size

int addtosegList(size_t size){
  /*if(size == 1)
    return 0;
  if (size ==2)
    return 1;
  else if(size >= 3 && size <= 4)
    return 2;
  else if(size >= 5 && size <= 8)
    return 3;
  else if(size >= 9 && size <= 16)
    return 4;*/
  if(size == 32)
    return 0;
  else if(size >= 33 && size <= 64)
    return 1;
  else if (size >= 65 && size <= 128)
    return 2;
  else if(size >= 129 && size <= 256)
   return 3;
  else if(size >= 257 && size <= 512)
   return 4;
  else if(size >= 513 && size <= 1024)
   return 5;
  else if(size >= 1025 && size <= 2048)
   return 6;
  else if(size >= 2049 && size <= 4096)
   return 7;
  else if(size >= 4097)
    return 8;
  else
    return -1;
} 


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
/* Functions Reference Textbook Computer Systems A Programmer's Perspective
 * While deleting nodes had to explicitly cast to Node* otherwise error as parameters is expected to be Node*  
 * of deleteNode
*/
static void *coalesce(void *bp){
    size_t prev_alloc = get_alloc(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = get_alloc(HDRP(NEXT_BKLP(bp)));
    size_t size = get_size(HDRP(bp));
    //int segListno = addtosegList(size);
    if (prev_alloc && next_alloc){  //Case 1 
        //getsegListhead(segListno);
        //push(&head,bp);
        //push(&segList[segListno], bp);
        push(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc){     //Case 2 
  //find the size of next block in order to find its correct list
  ////once you found the correct list
  ////you remove next node
  //
  ////
  //you update the size size + next size
  //you update header and footer to hvaer a new nblock
  //
  //using ythe updated size, you find the correct lsit
  //you add the new node to the corrc list
  //
      size += get_size(HDRP(NEXT_BKLP(bp)));
        deleteNode((Node*)(NEXT_BKLP(bp))); //Removing next free block as will be coalesced with the current one
        //segListno = addtosegList(size);     //To find correct segListno with new size after coalescing
        //getsegListhead(segListno);
        //push(&head, bp);
        //push(&segList[segListno], bp);
        //push(bp);
        put(HDRP(bp), pack(size, 0));
        put(FTRP(bp), pack(size, 0));
        push(bp);
        //return bp;
    }

    else if (!prev_alloc && next_alloc){    //Case 3 
    size += get_size(HDRP(PREV_BLKP(bp)));
    //Remove the current free block and add the address of previous block as it is coalesced
    //remove(bp);
    //push(&head, PREV_BLKP(bp));
    deleteNode((Node*)PREV_BLKP(bp));            //Removing old free block
    //segListno = addtosegList(size);    //Finding correct segList based on new size
    //push(&segList[segListno], bp);
   // push(bp);
    put(FTRP(bp), pack(size, 0));
    put(HDRP(PREV_BLKP(bp)), pack(size, 0));
   // push(bp);
    bp = PREV_BLKP(bp);
    push(bp);
    }

    else{                               //Case 4
    	deleteNode((Node*)(NEXT_BKLP(bp)));
        deleteNode((Node*)PREV_BLKP(bp));          //Now deleting both prev and next free blocks from list
        size += get_size(HDRP(PREV_BLKP(bp))) + get_size(FTRP(NEXT_BKLP(bp)));
        //segListno = addtosegList(size);    //Finding correct segList based on new size
        //push(&segList[segListno], bp);
       // push(bp);
        put(HDRP(PREV_BLKP(bp)), pack(size, 0));
        put(FTRP(NEXT_BKLP(bp)), pack(size, 0));
        bp = PREV_BLKP(bp);
        push(bp);
    }
    return bp;
}

/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
static void *find_fits(size_t asize){
    /* First-fit search */
    void *bp;
    int segListno = addtosegList(asize);
    getsegListhead(segListno);
    Node* node = segList[segListno];
    while (segListno <=8){ 
      while (node != NULL) {
    	bp = node;   
        if (asize <= get_size(HDRP(bp))){
          return bp;
        }
        node = node->next;  
      }
      segListno++;
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
   // int segListno = addtosegList(csize-asize);
    if((csize - asize) >= (2*DSIZE)){
        deleteNode((Node*)bp);
        put(HDRP(bp), pack(asize, 1));
        put(FTRP(bp), pack(asize, 1));
        //deleteNode((Node*)bp);
        bp = NEXT_BKLP(bp);
       /* getsegListhead(segListno);
    	//push(&head, bp);
        push(&segList[segListno], bp);*/
       // segListno = addtosegList(czise-asize);
        put(HDRP(bp), pack(csize-asize, 0));
        put(FTRP(bp), pack(csize-asize, 0));
        //getsegListhead(segListno);
        //push(&segList[segListno], bp);
        push(bp);
    }
    else{
        put(HDRP(bp), pack(csize, 1));
        put(FTRP(bp), pack(csize, 1));
        deleteNode((Node*)bp);
    }
}

static void place_realloc(void *bp, size_t asize){
      size_t csize = get_size(HDRP(bp));
      //int segListno = addtosegList(csize-asize);
      if((csize - asize) >= (2*DSIZE)){
        put(HDRP(bp), pack(asize, 1));
        put(FTRP(bp), pack(asize, 1));
        bp = NEXT_BKLP(bp);
        //getsegListhead(segListno);
        //push(&head, bp);
        //push(&segList[segListno], bp);
        //push(bp);
        put(HDRP(bp), pack(csize-asize, 0));
        put(FTRP(bp), pack(csize-asize, 0));
        push(bp);
        }
        else{
          put(HDRP(bp), pack(csize, 1));
          put(FTRP(bp), pack(csize, 1));
          //deleteNode((Node*)bp);
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
  int listno = 0;
  head = NULL;
  while(listno <= 8){
      segList[listno] = NULL;
      listno++;
  } 
  if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    return false;
  put(heap_listp, 0);
  put(heap_listp + (1*WSIZE), pack(DSIZE, 1));
  put(heap_listp + (2*WSIZE), pack(DSIZE, 1));
  put(heap_listp + (3*WSIZE), pack(0,1));
  heap_listp += (2*WSIZE); //This make heap_listp points at the first header
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
  for(bp = heap_listp ; get_size(HDRP(bp))>0; bp = NEXT_BKLP(bp)){
  printf("\n H: %p \tbp: %p \t f: %p \tS: %lu \tA: %lu\n",HDRP(bp), bp, FTRP(bp), get_size(HDRP(bp)), get_alloc(HDRP(bp)));
  }
  printf("\n\nSegregated Linked List: \n");
  int listno = 0;
  while(listno <= 8 ){
    getsegListhead(listno);
    Node* node = (head);
    printf("\n\n List: %d\n", listno);
    while(node != NULL){
    bp = node;
    printf("\n H: %p \tbp: %p \t f: %p \tS: %lu \tA: %lu\n",HDRP(bp), bp, FTRP(bp), get_size(HDRP(bp)), get_alloc(HDRP(bp)));
    node = node->next;
    }
   listno++; 
  }






 /* printf("\n\nLinked List: \n");
  Node* node = (head);
  while (node != NULL) {
    bp = node;   
    printf("\n H: %p \tbp: %p \t f: %p \tS: %lu \tA: %lu\n",HDRP(bp), bp, FTRP(bp), get_size(HDRP(bp)), get_alloc(HDRP(bp)));
   // printf("\nAllocation Bit: %lu \tSinze: %lu", get_alloc(bp), get_size(HDRP(bp)));
   // if (!get_alloc(HDRP(bp)) && (asize <= get_size(HDRP(bp)))){
    //  return bp;
    //}
    node = node->next;  
  }*/                                 
#endif /* DEBUG */
    return true;
}
