#include<errno.h>
#include<stdio.h>
#include "csapp.h"

#define MAX_HEAP (1 << 20)

static char *mem_heap;
static char *mem_brk;
static char *mem_max_addr;

static char *heap_listp = 0;

void mem_init(void){
    mem_heap = (char *)Malloc(MAX_HEAP);
    mem_brk = (char *)mem_heap;
    mem_max_addr = (char *)(mem_heap + MAX_HEAP);
}


void *mem_sbrk(int incr){
    char *old_brk = mem_brk;

    if((incr < 0 || ((mem_brk + incr) > mem_max_addr))){
        errno = ENOMEM;
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *) - 1;
    }
    mem_brk += incr;
    return (void*)old_brk;
}

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
#define NEXT_BLKP(bp) ((char *)(bp) + GETSIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GETSIZE(((char *)(bp) - DSIZE)))

int mm_init(void){
    /*Create the initial empty heap*/
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *) - 1)  return -1;
    PUT(heap_listp, 0); //start
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1)); //Prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); //Prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); //Epilogue header

    /*extent the empty heap with a free block of CHUNKSIZE bytes*/
    if(extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
    return 0;
}   

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

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) return bp;

    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = prev_alloc;
    }

    else{
        size += GET_SIZE(FTRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = prev_alloc;
    }

    return bp;
}


void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

void *mm_malloc(size_t size){
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0) return NULL;
    if(size < DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

static void *find_fit(size_t asize){
    void* bp;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && asize < GET_SIZE(HDRP(bp)))
          return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= 2 * DSIZE){
        csize -= asize;
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize, 0));
        PUT(FTRP(bp), PACK(csize, 0));
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
