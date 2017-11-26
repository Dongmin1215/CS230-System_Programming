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

#define REMOVE_TAG(p,val)       (*((size_t *)p) = val)

// Adjust reallocation tag
#define REMOVE_RATAG(p)         (GET(p) & 0x2)
#define SET_RATAG(p)            (GET(p) | 0x2)

// Given the block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Number of seegrated list
#define LISTLIMIT       20
#define PRED_BP(bp)       ((char *)(bp))

#define SUCC_BP(bp)       ((char *)(bp) + WSIZE)


void *segregated_free_list[LISTLIMIT];
static void *extend_heap(size_t words);
static void insert_node(void *bp, size_t size);
static void *coalesce(void *bp);
static char *heap_listp;


static void *coalesce(void * bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
        return bp;

    else if (prev_alloc && !next_alloc){

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

    }

    else if (!prev_alloc && next_alloc){

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }

    else{

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

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
    if ((long)(bp = mem_sbrk(size)) == -1)
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


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

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
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

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














