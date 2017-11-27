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
    "jaewon"
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

#define MAX(x,y) ((x) > (y)? (x) : (y))
#define MIN(x,y) ((x) < (y)? (y) : (x))

#define PACK(size, alloc)       ((size) | (alloc))

// Pack a size and allocated bit into a word
#define GET(p)          (*(unsigned int *)(p))
#define SET_BP(p, bp)   (*(unsigned int *)(p) = (unsigned int)(bp))

// Preserve reallocation bit
#define PUT(p, val)     (*(unsigned int *)(p) = (val) | GET_TAG(p))

// Given 
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Read the size and allocated fields from address p
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)   (GET(p) & 0x1)
#define GET_TAG(p)      ((GET(p) & 0x2))

#define DELETE_TAG(p,val)       (*((size_t *)p) = val)

// Adjust reallocation tag
#define REMOVE_RATAG(p)         (GET(p) & 0x2)
#define SET_RATAG(p)            (GET(p) | 0x2)

// Given the block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Number of seegrated list
#define LISTLIMIT       20

// Address of free block's predecessor and succesor entries
#define PRED_BP(bp)       ((char *)(bp))
#define SUCC_BP(bp)       ((char *)(bp) + WSIZE)

// Address of free block's prdecessor and succesor on the segregated list
#define PRED(bp)        (*(char **)(bp))
#define SUCC(bp)        (*(char **)(SUCC_BP(bp)))

void *segregated_free_list[LISTLIMIT];
static void *extend_heap(size_t words);
static void insert_node(void *bp, size_t size);
static void delete_node(void *bp);
static void *coalesce(void *bp);
static char *heap_listp;
static void *new_allocate(void *bp, size_t size);

static void *coalesce(void * bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (GET_TAG(HDRP(PREV_BLKP(bp))))
        prev_alloc = 1;

    if (prev_alloc && next_alloc)
        return bp;

    else if (prev_alloc && !next_alloc){
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    }

    else if (!prev_alloc && next_alloc){
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }

    else{
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }

    insert_node(bp, size);
    return bp;


}

static void *new_allocate(void *bp, size_t size){
    size_t bp_size = GET_SIZE(HDRP(bp));
    size_t remain = bp_size - size;

    // Remove blcok from list
    delete_node(bp);


    if (remain <= DSIZE * 2){
        // Block header and footer
        PUT(HDRP(bp), PACK(bp_size, 1));
        PUT(FTRP(bp), PACK(bp_size, 1));
    }

    else if (size >= 100){

        // Split block
        // Block header and footer
        PUT(HDRP(bp), PACK(remain, 0));
        PUT(FTRP(bp), PACK(remain, 0));
        DELETE_TAG(HDRP(NEXT_BLKP(bp)), PACK(size, 1));
        DELETE_TAG(FTRP(NEXT_BLKP(bp)), PACK(size, 1));
        insert_node(bp, remain);
        return NEXT_BLKP(bp);

    }

    else{

        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        DELETE_TAG(HDRP(NEXT_BLKP(bp)), PACK(remain, 0));
        DELETE_TAG(FTRP(NEXT_BLKP(bp)), PACK(remain, 0));
        insert_node(NEXT_BLKP(bp), remain);
    }

    return bp;

}

/* 
 * Extend_heap : Extend the heap with system call.
 */

static void *extend_heap(size_t words)
{
    char *bp;               // Pointer to allocate memory newly
    size_t size;            // Align the size

    size = ALIGN(words);
    // Allocate an even number of words to maintain alignment
    if ((bp = mem_sbrk(size)) == (void *) -1)
        return NULL;

    // Initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    insert_node(bp, size);
    return coalesce(bp);

}


static void insert_node(void *bp, size_t size)
{
    int list = 0;
    void *search_bp = bp;
    void *insert_bp = NULL;

    while ((list < LISTLIMIT - 1) && (size > 1)){
        size = size >> 1;
        list += 1;
    }

    // Set the location on the llist to insert the pointer and keep the list aligned
    // by byte size in ascending order
    search_bp = segregated_free_list[list];
    while ((search_bp != NULL) && (size > GET_SIZE(HDRP(search_bp)))){
        insert_bp = search_bp;
        search_bp = PRED_BP(search_bp);
    }


    // Set predecessor and successor
    if (search_bp != NULL){

        if (insert_bp != NULL){

            SET_BP(PRED_BP(bp), search_bp);
            SET_BP(SUCC_BP(search_bp), bp);
            SET_BP(SUCC_BP(bp), insert_bp);
            SET_BP(PRED_BP(insert_bp), bp);
        }
        else{
            SET_BP(PRED_BP(bp), search_bp);
            SET_BP(SUCC_BP(search_bp), bp);
            SET_BP(SUCC_BP(bp), NULL);

            // Add block to appropriate list
            segregated_free_list[list] = bp;
        }
    }

    else{
        if (insert_bp != NULL){
            SET_BP(PRED_BP(bp), NULL);
            SET_BP(SUCC_BP(bp), insert_bp);
            SET_BP(PRED_BP(insert_bp), bp);
        }
        else{
            SET_BP(PRED_BP(bp), NULL);
            SET_BP(SUCC_BP(bp), NULL);

            //Add block to appropriate list
            segregated_free_list[list] = bp;

        }


    }


}

static void delete_node(void *bp){
    int list = 0;
    size_t size = GET_SIZE(HDRP(bp));

    // Select segregated list
    while ((list < LISTLIMIT - 1) && (size > 1)){
        size = size >> 1;
        list += 1;
    }

    if (PRED(bp) != NULL){
        if (SUCC(bp) != NULL){
            SET_BP(SUCC_BP(PRED(bp)), SUCC(bp));
            SET_BP(PRED_BP(SUCC(bp)), PRED(bp));
        }
        else{
            if (SUCC(bp) != NULL)
                SET_BP(PRED_BP(SUCC(bp)), NULL);
            else
                segregated_free_list[list] = NULL;
        }
    }


    return;

}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // List counter
    int cnt;
    char *heap_start;

    for (cnt = 0; cnt < LISTLIMIT; cnt++)
        segregated_free_list[cnt] = NULL;

    // Initialize array of pointers to segregated free list
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) - 1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(-1, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;


    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t _size = 0;
    size_t extend_size = 0;
    void *bp = NULL;
    size_t search_size = _size;
    int cnt = 0;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        _size = 2 * DSIZE;
    else
        _size = ALIGN(size + DSIZE);

    while (cnt < LISTLIMIT){
        if ((cnt == LISTLIMIT - 1) || ((search_size <= 1) && (segregated_free_list[cnt] != NULL))){
            bp = segregated_free_list[cnt];

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
    if (bp == NULL){

        extend_size = MAX(_size, CHUNKSIZE);

        if ((bp = extend_heap(extend_size)) == NULL)
            return NULL;

    }
    bp = new_allocate(bp, _size);

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
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














