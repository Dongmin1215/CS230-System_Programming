/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "v",
    /* First member's full name */
    "Dongmin Lee",
    /* First member's email address */
    "theresaldm@kaist.ac.kr",
    /* Second member's full name (leave blank if none) */
    "Jaewon Kim",
    /* Second member's email address (leave blank if none) */
    "zxcvbn216@kaist.ac.kr"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


// Added some Macros
#define WSIZE 4             // word and header/footer size (bytes)
#define DSIZE 8             // double word size (bytes)
#define CHUNKSIZE (1<<12)   // Extend heap by this amount (bytes)
#define LISTLIMIT     20    //Number of segregated lists */
#define REALLOC_BUFFER  (1<<7) //Reallocation buffer

static inline int MAX(int x, int y) { 
    return x > y ? x : y;
}

static inline int MIN(int x, int y) { 
    return x < y ? x : y;
}


// Pack a size and allocated bit into a word
static inline size_t PACK(size_t size, int alloc) {
    return ((size) | (alloc & 0x1));
}

// Read and write a word at address p 
static inline size_t GET(void *p) {
    return  *(size_t *)p;
}


// Clear reallocation bit
static inline void REMOVE_TAG (void *p, size_t val){
  *((size_t *)p) = val;
}

// Adjust reallocation tag 
static inline size_t REMOVE_RATAG(void *p){
    return GET(p) & 0x2;
}
static inline size_t SET_RATAG(void *p){
    return GET(p) | 0x2;
}


// Preserve reallocation bit
#define PUT(p, val)       (*(unsigned int *)(p) = (val) | GET_TAG(p))


// Store predecessor or successor pointer for free blocks
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

// Read the size and allocation bit from address p
static inline size_t GET_SIZE( void *p )  {
    return GET(p) & ~0x7;
}

static inline int GET_ALLOC( void *p  ) {
    return GET(p) & 0x1;
}

static inline size_t GET_TAG( void *p )  {
    return GET(p) & 0x2;
}


// Address of block's header and footer
static inline void *HDRP(void *bp) {
    
    return ( (char *)bp) - WSIZE;
}

static inline void *FTRP(void *bp) {
    return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
}


// Address of (physically) next and previous blocks
static inline void *NEXT_BLKP(void *ptr) {
    return  ((char *)(ptr) + GET_SIZE(((char *)(ptr) - WSIZE)));
}

static inline void* PREV_BLKP(void *ptr){
    return  ((char *)(ptr) - GET_SIZE(((char *)(ptr) - DSIZE)));
}


// Address of free block's predecessor and successor entries
static inline void* PRED_PTR(void *ptr){
    return ((char *)(ptr));
}

static inline void* SUCC_PTR(void *ptr){
    return ((char*)(ptr) + WSIZE);
}

// Address of free block's predecessor and successor on the segregated list
static inline void* PRED(void *ptr){
    return (*(char **)(ptr));
}

static inline void* SUCC(void *ptr){
    return (*(char **)(SUCC_PTR(ptr)));
}


void *segregated_free_list[LISTLIMIT];

static void *extend_heap(size_t );
static void insert_node(void *bp, size_t size);
static void delete_node(void *ptr);
static void *coalesce(void *bp);
static void *place(void *ptr, size_t asize);


static void *extend_heap(size_t size)
{
    // Pointer to allocate memory
    void *bp;
    size_t asize;  
    
    // Do the lignment 
    asize = ALIGN(size); 
    
    // Extend the heap 
    if ((bp = mem_sbrk(asize)) == (void *) - 1)
        return NULL;
    
    // Set headers and footer
    REMOVE_TAG(HDRP(bp), PACK(asize, 0)); 
    REMOVE_TAG(FTRP(bp), PACK(asize, 0)); 
    REMOVE_TAG(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  
    
    // Insert new block into appropriate list 
    insert_node(bp, asize);
    

    return coalesce(bp);
}

// Insert a block pointer into a segregated list in ascending order
static void insert_node(void *bp, size_t size) {
    int list = 0;
    void *search_bp = bp;
    void *insert_bp = NULL;
    
    // Select segregated list
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size = size >> 1;
        list += 1;
    }
    
    // Select location on list to insert pointer and keep the list in ascending order (byte)
    search_bp = segregated_free_list[list];
    while ((search_bp != NULL) && (size > GET_SIZE(HDRP(search_bp)))) {
        insert_bp = search_bp;
        search_bp = PRED(search_bp);
    }
    
    // Set predecessor and successor
    if (search_bp != NULL) {
        if (insert_bp != NULL) {
            SET_PTR(PRED_PTR(bp), search_bp);
            SET_PTR(SUCC_PTR(search_bp), bp);
            SET_PTR(SUCC_PTR(bp), insert_bp);
            SET_PTR(PRED_PTR(insert_bp), bp);
        } else {
            SET_PTR(PRED_PTR(bp), search_bp);
            SET_PTR(SUCC_PTR(search_bp), bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            
            // Add block to appropriate list
            segregated_free_list[list] = bp;
        }
    } else {
        if (insert_bp != NULL) {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), insert_bp);
            SET_PTR(PRED_PTR(insert_bp), bp);
        } else {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            
            //Add block to appropriate list 
            segregated_free_list[list] = bp;
        }
    }
    
    return;
}

// Coallesce adjacent free blocks. 
// Sort the new free block into appropriate list
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    
    // Do not coalesce with previous block if the previous block is tagged with Reallocation tag
    if (GET_TAG(HDRP(PREV_BLKP(bp))))
        prev_alloc = 1;
    
    // Return if previous and next blocks are allocated
    if (prev_alloc && next_alloc) 
        return bp;
    

    // Find the block and merge
    else if (prev_alloc && !next_alloc) {  
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
     // Adjust segregated linked lists
    insert_node(bp, size);
    
    return bp;
}


// Remove the pointer from the segregated list. And adjust pointers in predecessor and successor bloks.
static void delete_node(void *bp) {
    int list = 0;
    size_t size = GET_SIZE(HDRP(bp));
    
    // Select segregated list 
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size = size >> 1;
        list += 1;
    }
    
    if (PRED(bp) != NULL) {
        if (SUCC(bp) != NULL) {
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        } else {
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            segregated_free_list[list] = PRED(bp);
        }
    } else {
        if (SUCC(bp) != NULL) 
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        else
            segregated_free_list[list] = NULL;
        
    }
    
    return;
}


// Set headers and footers for allocated block. 
static void *place(void *ptr, size_t asize)
{
    size_t ptr_size = GET_SIZE(HDRP(ptr));
    size_t remainder = ptr_size - asize;
    
     // Remove block from list 
    delete_node(ptr);
    
    // Do not split block
    if (remainder <= DSIZE * 2) {
        PUT(HDRP(ptr), PACK(ptr_size, 1)); /* Block header */
        PUT(FTRP(ptr), PACK(ptr_size, 1)); /* Block footer */
    }
    
    else if (asize >= 100) {
        // split block 
        PUT(HDRP(ptr), PACK(remainder, 0)); 
        PUT(FTRP(ptr), PACK(remainder, 0)); 
        REMOVE_TAG(HDRP(NEXT_BLKP(ptr)), PACK(asize, 1)); 
        REMOVE_TAG(FTRP(NEXT_BLKP(ptr)), PACK(asize, 1)); 
        insert_node(ptr, remainder);
        return NEXT_BLKP(ptr);
        

    }
    
    else {
        // Split block 
        PUT(HDRP(ptr), PACK(asize, 1)); 
        PUT(FTRP(ptr), PACK(asize, 1)); 
        REMOVE_TAG(HDRP(NEXT_BLKP(ptr)), PACK(remainder, 0)); 
        REMOVE_TAG(FTRP(NEXT_BLKP(ptr)), PACK(remainder, 0)); 
        insert_node(NEXT_BLKP(ptr), remainder);
    }
    return ptr;
}





/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // list counter
    int cnt;
    char *heap_start = NULL;

    // Initialize array of pointers to segregated free list
    for (cnt = 0; cnt < LISTLIMIT; cnt ++)
        segregated_free_list[cnt] = NULL;

    // Allocate memory for the initial empty heap
    if ((long)(heap_start = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    REMOVE_TAG(heap_start, 0);
    REMOVE_TAG(heap_start + (1 * WSIZE), PACK(DSIZE, 1));
    REMOVE_TAG(heap_start + (2 * WSIZE), PACK(DSIZE, 1));
    REMOVE_TAG(heap_start + (3 * WSIZE), PACK(0, 1));
    

    if (extend_heap(1 << 6) == NULL)
        return -1;


    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t _size;       // Adjusted block size 
    size_t extend_size; // Amount to extend heap if no fit 
    void *bp = NULL;    //Pointer 
    int cnt = 0;        // List counter */
    
    // For the size 0
    if (size == 0)
        return NULL;
    
    // Adjust block size to include boundary tags and alignment requirements 
    if (size <= DSIZE)
        _size = 2 * DSIZE;
    else
        _size = ALIGN(size + DSIZE);
    
    
    // Select a free block of sufficient size from segregated list
    size_t search_size = _size;
    while (cnt < LISTLIMIT) {
        if ((cnt == LISTLIMIT - 1) || ((search_size <= 1) && (segregated_free_list[cnt] != NULL))) {
            bp= segregated_free_list[cnt];

            // Ignore blocks that are too small or marked with the reallocation bit
            while ((bp != NULL) && ((_size > GET_SIZE(HDRP(bp))) || (GET_TAG(HDRP(bp)))))
                bp = PRED(bp);
            
            if (bp != NULL)
                break;
        }
        
        search_size = search_size >> 1;
        cnt += 1;
    }
    
    // Extend the heap if no free blocks of sufficient size are found
    if (bp == NULL) {
        extend_size = MAX(_size, CHUNKSIZE);
        
        if ((bp = extend_heap(extend_size)) == NULL)
            return NULL;
    }
    
    // Place and divide block 
    bp = place(bp, _size);
    
    
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    REMOVE_RATAG(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    insert_node(bp, size);
    coalesce(bp);

    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *new_bp = bp;           // Pointer to be returned 
    size_t new_size = size;    
    int remain;                 // Adequacy of block sizes 
    int extend_size;            
    int buffer;                 
    
    // Ignore when the size is 0
    if (size == 0)
        return NULL;
    
    // Adjust block size to include boundary tag and alignment requirements 
    if (new_size <= DSIZE) 
        new_size = 2 * DSIZE;
    else 
        new_size = ALIGN(size + DSIZE);
    
    
    // Add overhead requirements 
    new_size += 1 << 7;
    
    // Calculate block buffer 
    buffer = GET_SIZE(HDRP(bp)) - new_size;
    
    //Allocate more space if overhead falls below the minimum 
    if (buffer < 0) {

        // Check if next block is a free block or the epilogue block
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))) {
            remain = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - new_size;
            if (remain < 0) {
                extend_size = MAX(-remain, CHUNKSIZE);
                if (extend_heap(extend_size) == NULL)
                    return NULL;
                remain += extend_size;
            }
            
            delete_node(NEXT_BLKP(bp));
            
            // Do not split block
            REMOVE_TAG(HDRP(bp), PACK(new_size + remain, 1)); 
            REMOVE_TAG(FTRP(bp), PACK(new_size + remain, 1)); 
        } else {
            new_bp = mm_malloc(new_size - DSIZE);
            memcpy(new_bp, bp, MIN(size, new_size));
            mm_free(bp);
        }
        buffer = GET_SIZE(HDRP(new_bp)) - new_size;
    }
    
    // Tag the next block if block overhead drops below twice the overhead 
    if (buffer < 2 * (1 << 7))
        SET_RATAG(HDRP(NEXT_BLKP(new_bp)));
    

    return new_bp;
}














