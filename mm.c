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
/* Reference: Computer Systems: A Programmer's Perspective
 *
 * CHECKPOINT 1: Did implicit implemenation. Throughput and utilization both were very low as we checked all block in memory irrespective of if they were allocated or free
 * CHECKPOINT 2: Used Segregated Free List which stored only the free block. Didn't had to iterate through complete heap to find for free block and only the SegList. Throughput was maximum but Utilization was less.
 * CHECKPOINT 3: Footer Optimization to increase utility as now we are getting rid of footers in allocated block and saving memory 
 * Using Segregated Explicit List with Footer Optmization
 * 
 * Basic Structure
 *
 * Allocated block				Free block    			Segregated List(Keep free blocks: SL0, SL1, ... is the head and points to first node for each segList for different size)
 *|-----------------|		|-------------------|      [SL0]  [SL1]   [SL2]  [SL3]  [SL4]  [SL5]  [SL6]  [SL7]   [SL8]  
 *|				   	|		|	   Header		|      	 |		|		|	   |	  |		 |		|	   |	   |
 *|	     Header		|		|___________________|		[ ]	   [ ]     [ ]	  [ ]    [ ]    NULL   [ ]	  [ ]     NULL
 *|-----------------|		|					|		 |		|		|      |      |				|	   |
 *|					|		|		Prev		|		[ ]	   [ ]	   NULL   NULL   NULL		   NULL	  NULL
 *|					|		|___________________|		 |		|
 *|    				|		|					|		[ ]	   NULL
 *|	    Payload		|		|		Next		|        |
 *|					|		|___________________|       NULL
 *|					|		|					|			
 *|					|		|	   Footer	 	|
 *|_________________|		|___________________|
 * No footer in   			Footer are there in
 * Allocated blocks         free blocks
 *
 * SEGREGATED LIST for FREE BLOCKS : 8 SEGLIST based on size
 * SegList is pointer array of Struct Node which has two pointers prev and next. 
 * 
 * For making it 16 bit aligned and keeping in mind about freeing allocated block, the minimum size is 32 bytes.
 * Each free block has 8 bytes for header and 8 bytes for footer which stores the size, allocation bit for itself and one for the previous block
 * Allocated block have a header of 8 bytes which stores the size, allocation bit for itself and allocation bit for
 * previous block.
 *
 * Malloc Function: Malloc function is used to allocate memory based on the size passed and keeping alignment in mind. 
 *				    This function calls find_fit() to find the bp to allocate memory at and place. Also calling extend_heap()
 *					if there is no place currently in the heap to allocate the sufficient memory.
 *
 * Coalesce Function is used to join 2 or more free adjacent blocks when extendheap or free is called.
 *
 * Find Fit Function: Using First Fit implementation in Segregated List. Start by checking segregated list based on size
 * 					  User ask for and then check all the following if we don't find free block.
 *
 * Place Function: Find fit returns the bp position from Segregated List and place function accordingly change allocation
 * 				   bits, update the size and return the pointer.
 * 
 * place_realloc Function: Created a new palce for place as in realloc working on already allocated block of memory and we
 *						   don't need to call remove.
 *
 * Realloc Function: Adjusting already allocated block based on the size passed by user which could be either same, smaller
 * 					 or bigger.
 *
 * Push and Delete Function: Used to add and delete the free block from the segregated list. Push always at the head of the list.
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
/* Creating struct for Doubly Linked List */
typedef struct Node{
	struct Node *prev;
	struct Node *next;
}Node;
Node* head = NULL;
/* To store the head of all 9 segList which is initialized as NULL in mm_init */
Node *segList[9];

/*
 * push function which has pointer bp passed as reference, finds correct segList based on size
 * and then add to the head of segList. Using segList[segListno] as the head variable directly
 */
void push(void *bp){
	size_t addNodeSize = get_size(HDRP(bp));
	int segListno = addtosegList(addNodeSize);
	struct Node* newNode = (struct Node*)bp;
	newNode->next = (segList[segListno]);			
	newNode->prev = NULL;
	if(segList[segListno]!= NULL){
		(segList[segListno])->prev = newNode;
	}
	segList[segListno] = newNode;
}

/* 
 * delete function gets the bp address as the reference of the node to be deleted. 
 * find the segList for that bp by using the size of bp and then delete that node and update
 * prev and next pointers.
 */
void delete(Node* del)  
{   
	/* Finding the size of node we are going to delete to find the segList to delete it from*/
	size_t deleteNodeSize = get_size(HDRP(del));
	int segListno = addtosegList(deleteNodeSize);
	/* When the Node passed to delete is empty */
	if(del == NULL)
		return;
	/* segList[segListno] is the address of head for that particular seg list and is treated as head here */

	/* If condition to check if segList[segListno] which is head is the not we want to delete and accordingly changing head */
	if(segList[segListno] == del)
		segList[segListno] = del->next;

	/* 
	 * We will change the next of the one which we are deleting if it is not NULL i.e. it is not the Last Node
	 * and making the link between the previous and next node to the one we are deleting
	 */
	if (del->next != NULL)  
		del->next->prev = del->prev;  

	/* Checking if it isn't the first node and accordingly making the connection with current next node to the node we are deleting*/

	if (del->prev != NULL)  
		del->prev->next = del->next;  

	/* there are two conditions are we are using doubly linked list */
}

//Node *segList[14];

/* Helper Functions for Segregated Linked list 
 * Majorly changed place and coalesce function to switch to segregated linked list. Find fit changed to iterate through segList heads now.
 * Tried using the push and delete function with directly passing head address as parameter but because of error switched to just bp for push function
 * Inside push function itself making use of the following helper function to find address of head for segList. 
 * In my coalesce and place function have push() at several positions as while running some traces,
 * realised was calling push too early which was not updating my segList correctly
 */

/* Returns the head address of the particular segList based on the parameter segListno */
void *getsegListhead(int segListno){
	if(segListno == 0){
		head = segList[0];
	}
	else if(segListno == 1){
		head = segList[1];
	}
	else if(segListno == 2){
		head = segList[2];
	}
	else if(segListno == 3){
		head = segList[3];
	}
	else if(segListno == 4){
		head = segList[4];
	}
	else if(segListno == 5){
		head = segList[5];
	}
	else if(segListno == 6){
		head = segList[6];
	}
	else if(segListno == 7){
		head = segList[7];
	}
	else if(segListno == 8){
		head = segList[8];
	}
	return NULL;
}

/* Function returns the ith number of the segregated linkedlist to be used based on size */

int addtosegList(size_t size){
	if(size ==32)
		return 0;
	else if(size <= 64)
		return 1;
	else if (size <= 128)
		return 2;
	else if(size <= 256)
		return 3;
	else if(size <= 512)
		return 4;
	else if(size <= 1024)
		return 5;
	else if(size <= 2048)
		return 6;
	else if(size <= 4096)
		return 7;
	else if(size >= 4097)
		return 8;
	else
		return -1;

}

/* Static Inline Functions from Computer Systems A Programmer's Perspective Book*/

/* was defined as Macros in the book changed into a static inline function*/

/* Added some helper function for footer optmization like get_prev_alloc() and updated pack function to include prev_alloc bit */

static inline unsigned long max(long x, long y){
	return ((x) > (y)? (x) : (y));
}

/* This function packs/sets together the prev allocation bit, the size and the allocation bit */
static inline unsigned long pack(unsigned long size, unsigned long prev_alloc, unsigned long alloc){
	return ((prev_alloc<<1) | (size) | (alloc));
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

/* This function returns the prev allocation bit from header */                                                                                                       
static inline unsigned long get_prev_alloc(const void* p){                           
	return ((get(p) & 0x2) >> 1);                                                      
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

static inline char* NEXT_BLKP(void* bp){
	return ((char *)(bp) + get_size(((char *)(bp) - WSIZE)));
}

/* This function takes pointer to the point where the previous data starts */

static inline char* PREV_BLKP(void* bp){
	return ((char *)(bp) - get_size(((char *)(bp) - DSIZE)));
}

/*
 * Additional Functions Reference Textbook Computer Systems A Programmer's Perspective
 */
/* 
 * Takes the size as input and allocate even number of blocks for alignment.
 * Also, create free block header, footer and update epilogue after extending heapsize
 * This returns coalesce, to merge any two consecutive free blocks
 */
static void *extended_heap(size_t words){
	char *bp;
	size_t size;
	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words*WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	put(HDRP(bp), pack(size, get_prev_alloc(HDRP(bp)), 0));              /* Free block header */
	put(FTRP(bp), pack(size, get_prev_alloc(HDRP(bp)), 0));       /* Free block footer */
	put(HDRP(NEXT_BLKP(bp)), pack(0, 0, 1));			/* New epilogue header */ 

	/* Coalesce if the previous block was free */
	return coalesce(bp);  
}

/* Functions Reference Textbook Computer Systems A Programmer's Perspective
 * Coealesce functions joins any two consistent free blocks and update size. Adjusting size and allocation bits of current and prev block. 
 * Also, adjusting the prev_alloc bit in NEXT blocks once they are merged and added to segList as free blocks. There are 4 cases
 * 1) When both the next and prev blocks are allocated and no need of merging anything, but udpate next block that prev block is free
 * 2) Next block is free, then I join the next block and update the seglist with correct free block
 * 3) Prev blo k is free, then I join the prev block and update the segList with correct free block
 * 4) Both prev and next block are free, then I join the three block update size and update the segList.
 * While deleting nodes had to explicitly cast to Node* otherwise error as parameters is expected to be Node*  
 * of delete
 * 
 */
static void *coalesce(void *bp){
	size_t prev_alloc = get_prev_alloc(HDRP(bp));
	size_t next_alloc = get_alloc(HDRP(NEXT_BLKP(bp)));
	size_t size = get_size(HDRP(bp));
	/* Case 1: Both prev and next allocated */
	if (prev_alloc && next_alloc){  												 
		put(HDRP(NEXT_BLKP(bp)), pack(get_size(HDRP(NEXT_BLKP(bp))), 0, 1 ));
		push(bp);
		return bp;
	}
	/* Case 2: Prev block allocated and next block is free */
	else if (prev_alloc && !next_alloc){    										  
		size += get_size(HDRP(NEXT_BLKP(bp)));
		delete((Node*)(NEXT_BLKP(bp))); 
		put(HDRP(bp), pack(size, 1, 0));  
		put(FTRP(bp), pack(size, 1, 0));
		put(HDRP(NEXT_BLKP(bp)), pack(get_size(HDRP(NEXT_BLKP(bp))), 0, 1 ));
		push(bp);
	}
    /* Case 3: Prev block is free and next is allocated */
	else if (!prev_alloc && next_alloc){    
		size += get_size(HDRP(PREV_BLKP(bp)));
		delete((Node*)PREV_BLKP(bp));           
		put(FTRP(bp), pack(size, 1, 0));        
		put(HDRP(PREV_BLKP(bp)), pack(size, 1, 0)); 
		put(HDRP(NEXT_BLKP(bp)), pack(get_size(HDRP(NEXT_BLKP(bp))), 0, 1 )); //Setting the next block prev_alloc bit in header as 0 as we are coalescing free blocks
		bp = PREV_BLKP(bp);
		push(bp);
	}
	/* Case 4: Prev block and next block are both free */
	else{                               
		delete((Node*)(NEXT_BLKP(bp)));
		delete((Node*)PREV_BLKP(bp));          //Now deleting both prev and next free blocks from list
		size += get_size(HDRP(PREV_BLKP(bp))) + get_size(FTRP(NEXT_BLKP(bp)));
		put(HDRP(PREV_BLKP(bp)), pack(size, 1, 0));
		put(FTRP(NEXT_BLKP(bp)), pack(size, 1, 0));
	    put(HDRP(NEXT_BLKP(bp)), pack(get_size(HDRP(NEXT_BLKP(bp))), 0, 1 ));
		bp = PREV_BLKP(bp);
		push(bp);
	}
	return bp;
}

/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
/* 
 * find_fits function first based on the size find the segList where we should place the block,
 * Then if it finds enough size in that segList then return that block bp to acllocate or 
 * else check all the bigger block after it if there is place to allocate and return bp
 * else if nothing is found return NULL so extendheap can be called by find_fits callee. 
 */
static void *find_fits(size_t asize){
	/* First-fit search */
	void *bp;
	int segListno = addtosegList(asize);
	getsegListhead(segListno);
	Node* node = segList[segListno];
	/* While to check all segList bigger than the seglist based on size */
	while (segListno <=8){ 
		node = segList[segListno];       
		while (node != NULL) {
			bp = node;   
			if (asize <= get_size(HDRP(bp))){
				return bp;
			}
			node = node->next;  
		}
		segListno++;
	}
	return NULL; /* No fit */
	//#endif 
}

/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
/* This is called after find_fit return the correct bp to allocate memory, 
 * This function allocate the user required memory and if there is still place left more than 32 bytes,
 * it splits the block and mark the left out as free and add to segList else allocate complete block and remove
 * it from segList find_fits might be returning very big block from a bigger segList and then it is very
 * important to split and add the splitted free block to correct segList based on new size.
 */

static void place(void *bp, size_t asize){
	size_t csize = get_size(HDRP(bp));
	if((csize - asize) >= (2*DSIZE)){
		delete((Node*)bp);					
		put(HDRP(bp), pack(asize, 1, 1));
		bp = NEXT_BLKP(bp);
		/* Updating the splitted free block header and footer and adding it to segList */
		put(HDRP(bp), pack(csize-asize, 1, 0));	
		put(FTRP(bp), pack(csize-asize, 1, 0));
		push(bp);
	}
	else{

		put(HDRP(bp), pack(csize, 1, 1)); 
		put(HDRP(NEXT_BLKP(bp)), pack(get_size(HDRP(NEXT_BLKP(bp))), 1, get_alloc(HDRP(NEXT_BLKP(bp))))); 
		delete((Node*)bp);
	}
}
/*
   Need a different place for realloc call, was get segfault on some traces. Similar to place but 
   Realloc changes already allocated blocks and thus there is no need to delete from 
   any free list which caused seg fault. 
   */
static void place_realloc(void *bp, size_t asize){
	size_t csize = get_size(HDRP(bp));
	int prev_alloc = get_prev_alloc(HDRP(bp));
	if((csize - asize) >= (2*DSIZE)){
		put(HDRP(bp), pack(asize, prev_alloc, 1)); 	/* packing with prev_alloc as the previous block here can be allocated or free */
		bp = NEXT_BLKP(bp);
		put(HDRP(bp), pack(csize-asize, 1, 0));
		put(FTRP(bp), pack(csize-asize, 1, 0));
		coalesce(bp);
		//push(bp);
	}
	else{
		put(HDRP(bp), pack(csize, prev_alloc, 1));
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
/* mm_init function initialises out heap with starting alignment of 16 bytes and 
 * make prologue and epilogue and call extend heap for initializing heap.
 * Also, set all the segList and the head global pointer as NULL
 */
bool mm_init(void)
{  
	/* IMPLEMENT THIS */
	int listno = 0;
	head = NULL;
	while(listno <= 8){
		segList[listno] = NULL;
		listno++;
	} 
	if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
		return false;
	put(heap_listp, 0);
	put(heap_listp + (1*WSIZE), pack(DSIZE, 0, 1));
	put(heap_listp + (2*WSIZE), pack(DSIZE, 1, 1));
	put(heap_listp + (3*WSIZE), pack(0, 1, 1));
	heap_listp += (2*WSIZE); //This make heap_listp points at the first header
	void *bp = extended_heap(CHUNKSIZE/WSIZE);
	if(bp == NULL)
		return false;
	return true;
}

/*
 * malloc
 */
/*
 * This functions takes the size of memory to be allocated and if size is return NULL
 * if size not zero it finds the size to align 16 bytes. If less than 16 bytes then
 * size for allocation is 32 bytes which is minimum required size keeping header and 
 * footer in mind. If size is bigger than DSIZE then call the align function to find 
 * 16 bytes aligned size. It then calls find_fits to find place to allocate memory in heap
 * and return bp if find_fits return NULL then extend_heap is called to increase size 
 * of heap and return bp.
 */
void* malloc(size_t size)
{
	size_t asize;
	size_t extendsize;
	void *bp;
	//mm_checkheap(__LINE__);
	if(size == 0)
		return NULL;

	if(size <= DSIZE)
		asize = 2*DSIZE;
	else{

	/* Changed method to find asize as now we don't want footer in allocated blocks. 
	 * Calling align functino with WSIZE added in the paramter to size which will return the required 
	 * aligned address to make use of only headers in allocated blocks
	 */
		asize =align(size+WSIZE); 
	}
	if((bp = find_fits(asize)) != NULL){
		place(bp, asize);
		//mm_checkheap(__LINE__);
		return bp;
	}

	extendsize = max(asize, CHUNKSIZE);
	if((bp = extended_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
    //mm_checkheap(__LINE__);
	return bp;
}

/*
 * free
 */
/* Functions Reference Textbook Computer Systems A Programmer's Perspective */
/*
 * free function unallocate a allocated block. The block that has to be unallocated is passed 
 * as parameter. Find the prev_alloc bit, set the allocation bit in Header and Footer now since
 * free to 0 and setting the next block prev_alloc bit as 0. Then calling coalesce to merge 
 * any consecutive free block and add accordingly to the segList.
 */
void free(void* ptr)
{
	/* IMPLEMENT THIS */
	size_t size = get_size(HDRP(ptr));
	if(get_alloc(HDRP(ptr))==0)
		return; 
	int prev_alloc = get_prev_alloc(HDRP(ptr)); 
	put(HDRP(ptr), pack(size, prev_alloc, 0));
	put(FTRP(ptr), pack(size, prev_alloc, 0));
	put(HDRP(NEXT_BLKP(ptr)), pack(get_size(HDRP(NEXT_BLKP(ptr))), 0, get_alloc(HDRP(NEXT_BLKP(ptr)))));
	coalesce(ptr);
}

/*
 * realloc
 */

/* 
 * realloc function takes five cases:
 * 1) if argument passed as ptr to realloc is NULL return malloc 
 * 2) if the user in realloc passed 0 as new required size then free the allocated oldptr
 * 3) If the new size is same as the current size of oldptr then return the same oldptr as no change required
 * 4) If new size is less than the size of currently allocated block, then find the new 16 bit aligned size and called 
 *    place realloc and return the oldptr as the block ptr didn't change
 * 5) If none of the above case then we call malloc and use memcpy to copy data to new ptr returned by malloc
 *    and free position where data is currently held
 */
void* realloc(void* oldptr, size_t size)
{
	/* IMPLEMENT THIS */
	size_t asize;
	/* Case 1 */
	if(oldptr == NULL){         /* This call is equivalent to malloc */
		return malloc(size);
	}
	/* Case 2 */
	if(size == 0){
		free(oldptr);
		return NULL;
	}
	/* Case 3 */
	size_t old_size = get_size(HDRP(oldptr)) - 2*WSIZE;   // Storing the initial ptr size
	if(size == old_size){
		return oldptr;
	}
	/* Case 4 */
	else if (size<old_size){             // If new size is less than the old size

		if(size <= DSIZE)
			asize = 2*DSIZE;
		else
			asize = align(size+WSIZE); 

		place_realloc(oldptr, asize);
		return oldptr;
	}
	/* Case 5 */
	void *newPointer = malloc(size);        
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
	/* 
	 * Mostly called mm_checkheap(1) in gdb whenever wanted to check the heap and segList 
	 * and that would make the comparison easier. Also called this in malloc at times to 
	 * make sure I'm adding blocks correctly to heap
	 */
	void *bp;

	/* Printing the complete Heap */
	dbg_printf("\n\nHeap: \n");		
	for(bp = heap_listp ; get_size(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
		dbg_printf("\n H: %p \tbp: %p \t F: ",HDRP(bp), bp);
			if(get_alloc(HDRP(bp)) == 1)
				dbg_printf("N/A \t\t\tS: %lu \tPrevA: %lu \tA: %lu\n", get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));
			else
				dbg_printf("%p \t\tS: %lu \tPrevA: %lu \tA: %lu\n", FTRP(bp), get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));
	
		/* to check if prev alloc bit in Header of next block is correctly set */
		dbg_assert(get_alloc(HDRP(bp))== get_prev_alloc(HDRP(NEXT_BLKP(bp))));
		/* To check coealesce work correctly */
		if(get_alloc(HDRP(bp)) == 0){
			dbg_assert(get_alloc(HDRP(NEXT_BLKP(bp))) == 1);
			dbg_assert(get_prev_alloc(HDRP(bp)) == 1);
			
		}
		
	}
	/* For Epilogue */
	dbg_printf("\n H: %p \tbp: %p \t F: ",HDRP(bp), bp);
			if(get_alloc(HDRP(bp)) == 1)
				dbg_printf("N/A \t\t\tS: %lu \tPrevA: %lu \tA: %lu\n", get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));
			else
				dbg_printf("%p \t\tS: %lu \tPrevA: %lu \tA: %lu\n", FTRP(bp), get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));


    /* Printing the All the Segregated List */
	dbg_printf("\n\nSegregated Linked List: \n");
	int listno = 0;
	while(listno <= 8){
		getsegListhead(listno);
		Node* node = (head);
		dbg_printf("\n\n List: %d\n", listno);
		while(node != NULL){
			bp = node;
			dbg_printf("\n H: %p \tbp: %p \t F: ",HDRP(bp), bp);
			if(get_alloc(HDRP(bp)) == 1)
				dbg_printf("N/A \t\tS: %lu \tPrevA: %lu \tA: %lu\n", get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));
			else
				dbg_printf("%p \t\tS: %lu \tPrevA: %lu \tA: %lu\n", FTRP(bp), get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));
			node = node->next;
			dbg_assert(get_alloc(bp) == 0);
			dbg_assert(get(HDRP(bp)) == get(FTRP(bp)));
		}
		listno++;
	}

	/* To check assert conditions in segList */
	listno = 0;
	while(listno <= 8){
		getsegListhead(listno);
		Node* node = head;
		while(node != NULL){
			bp = node;

			//assert(get_alloc(HDRP(bp)) == 0);

			size_t size = get_size(HDRP(bp));
			int correctsegList = addtosegList(size);

			dbg_assert(correctsegList == listno);

			dbg_assert(get(HDRP(node)) == get(FTRP(node)));

			node = node->next;
		}
		listno++;
	}


	/* Seg List before footer optimization
	printf("\n\nSegregated Linked List: \n");
	int listno = 0;
	while(listno <= 8){
		getsegListhead(listno);
		Node* node = (head);
		printf("\n\n List: %d\n", listno);
		while(node != NULL){
			bp = node;
			dbg_printf("\n H: %p \tbp: %p \t f: %p \tS: %lu \t\tPrevA: %lu \tA: %lu\n",HDRP(bp), bp, FTRP(bp), get_size(HDRP(bp)), get_prev_alloc(HDRP(bp)), get_alloc(HDRP(bp)));
			node = node->next;
		}
		listno++;
	} */


	/* Used this when footer optimization wasn't there */
	/*
	   for(bp = heap_listp ; get_size(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
	   dbg_assert(get(HDRP(bp)) == get(FTRP(bp)));
	   }
	*/                                 
#endif /* DEBUG */
	return true;
}
