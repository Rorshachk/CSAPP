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


#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define WSIZE 4 //word size
#define DSIZE 8 //double word size
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc)) //pack a size and allocated into a word

/*read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
 
/*read the size and allocated fields from address p, p is the header or footer*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/*compute the address of p's header and footer*/
/*The header need to be setted before the footer*/
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/*compute the address of nexr and previous block*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*Get and put the pre and next block in the list*/
#define GET_PRE(p) (GET(p))
#define PUT_PRE(p, val) (PUT(p, (unsigned int)val))
#define GET_NEXT(p)  (*((unsigned int *)p + 1))
#define PUT_NEXT(p, val) (*((unsigned int *)p + 1) = (unsigned int)(val))

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */


static char *heap_listp;
static unsigned int *starter = 0;

static void *extend_heap(size_t words);
int mm_init(void);
static void *find_fit(size_t asize);
void realloc_place(void *bp, int asize);
static void *try_coalesce(void *bp, int asize, int *flag);
void add_to_list(void *ptr);
void erase_from_list(void *ptr);
static void *coalesce(void *bp);
void place(void *bp, size_t asize);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);


static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    /*Allocate an even number of words to maintain alignment*/
    size = (words & 1) ? (words+1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1) return NULL; //extend the memory

    PUT(HDRP(bp), PACK(size, 0)); //new block's header
    PUT(FTRP(bp), PACK(size, 0)); //new block's footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //new epilogue's header

    return coalesce(bp);
}


int mm_init(void)
{
    mem_init();
    if((heap_listp=(char*)mem_sbrk(6*WSIZE)) == (char*) - 1) return -1;
    starter = (unsigned int *)mem_heap_lo() + 1; //The starter took two words of the initial heap, which are pre and next
    PUT(heap_listp, 0);
    PUT(heap_listp + 3*WSIZE, PACK(DSIZE, 1)); //Prologue header
    PUT(heap_listp + 4*WSIZE, PACK(DSIZE, 1));  //Prologue footer
    PUT(heap_listp + 5*WSIZE, PACK(0, 1)); //Epilogue header
    PUT_PRE(starter, 0);
    PUT_NEXT(starter, 0);

    heap_listp += 4 * WSIZE;

    /*extent an initial chunk*/
    if(extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
    return 0;
}


static void *find_fit(size_t asize){
    unsigned int bp = 0;
    for(bp = GET_NEXT(starter); bp; bp = GET_NEXT(bp)){
    //    printf("%d\n", bp);
        if(GET_SIZE(HDRP(bp)) > asize)
          return (void*)bp;
    }
    return NULL;
}


void realloc_place(void *bp, int asize){
    size_t csize = GET_SIZE(HDRP(bp));
    if(csize - asize >= 2 * DSIZE){
        csize -= asize;
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize, 0));
        PUT(FTRP(bp), PACK(csize, 0));
        coalesce(bp);
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

static void *try_coalesce(void *bp, int asize, int *flag){
    void *pre = PREV_BLKP(bp), *nxt = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(FTRP(pre));
    size_t next_alloc = GET_ALLOC(HDRP(nxt));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) return bp;

    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(nxt));
        if(size >= asize){
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(nxt), PACK(size, 0));
            erase_from_list(nxt);
            *flag = 1;
        }
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(FTRP(pre));
        if(size >= asize){
            PUT(HDRP(pre), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
            erase_from_list(pre);
            bp = pre;
        }
    }
    else{
        size += GET_SIZE(FTRP(pre)) + GET_SIZE(HDRP(nxt));
        if(size > asize){
            PUT(HDRP(pre), PACK(size, 0));
            PUT(FTRP(nxt), PACK(size, 0));
            erase_from_list(pre);
            erase_from_list(nxt);
            bp = pre;
        }
    }
    return bp;
}

void add_to_list(void *ptr){
    unsigned int starter_next = GET_NEXT(starter);
    PUT_NEXT(starter, ptr);
    PUT_PRE(ptr, starter);
    PUT_NEXT(ptr, starter_next);
    if(starter_next) PUT_PRE(starter_next, ptr);
}


void erase_from_list(void *ptr){
    unsigned int pre, nxt;
    pre = GET_PRE(ptr);
    nxt = GET_NEXT(ptr);
    if(nxt) PUT_PRE(nxt, pre);
    PUT_NEXT(pre, nxt);
}

static void *coalesce(void *bp){
    void *pre = PREV_BLKP(bp), *nxt = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(FTRP(pre));
    size_t next_alloc = GET_ALLOC(HDRP(nxt));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        add_to_list(bp);
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(nxt));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(nxt), PACK(size, 0));
        erase_from_list(nxt);
        add_to_list(bp);
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(FTRP(pre));
        PUT(HDRP(pre), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        erase_from_list(pre);
        add_to_list(pre);
        bp = pre;
    }
    else{
        size += GET_SIZE(FTRP(pre)) + GET_SIZE(HDRP(nxt));
        PUT(HDRP(pre), PACK(size, 0));
        PUT(FTRP(nxt), PACK(size, 0));
        erase_from_list(pre);
        erase_from_list(nxt);
        add_to_list(pre);
        bp = pre;
    }
    return bp;
}


void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    erase_from_list(bp);
    if((csize - asize) >= 2 * DSIZE){
        csize -= asize;
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize, 0));
        PUT(FTRP(bp), PACK(csize, 0));
        coalesce(bp);
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extend_size;
    void *bp;

    if(size == 0) return NULL;
    if(size < DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE); //DSIZE-1是向上舍入

    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    extend_size = MAX(CHUNKSIZE, asize);
    if((bp=extend_heap(extend_size/WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr){

    if(ptr == 0) return ;

    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t asize;
    if(ptr == NULL) return mm_malloc(size);
    if(size == 0){
        mm_free(ptr);
        return 0;
    }

    if(size < DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    int old_size = GET_SIZE(HDRP(ptr));
    if(asize > old_size){
        void *new_ptr; int flag = 0;
        new_ptr = try_coalesce(ptr, size, &flag);
        if(flag){
            realloc_place(new_ptr, asize);
            return new_ptr;
        }
        else if(new_ptr != ptr){
            memcpy(new_ptr, ptr, old_size - DSIZE);
            realloc_place(new_ptr, asize);
            return new_ptr;
        }
        else{
            new_ptr = mm_malloc(asize);
            memcpy(new_ptr, ptr, old_size - DSIZE);
            mm_free(ptr);
            return new_ptr;
        }
    }else{
        realloc_place(ptr, asize);
        return ptr;
    }
}
