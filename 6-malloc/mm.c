/*
 * mm.c
 *
 * This allocator uses segregated list to arrange free blocks.
 * Segregated sizes are powers of 2's: 2^4, 2^5, 2^6, ...
 * First fit policy is used.
 * This allocator also uses a heuristic to reduce sbrk calls:
 * if there's been a peak, allocate twice the current request,
 * otherwise just allocate the max request in history.
 *
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/**
 * Constants and macros
 */

#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define PSIZE      (sizeof(void*))      /* Size of a pointer */
#define MIN_BLKSZ  (DSIZE + PSIZE * 2)  /* Min size of a block */

#define FL_SIZE     16      /* Size of free list */
#define FL_MIN      16      /* Minimum size to be maintained by free list */

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/**
 * Free block looks like
 * +-----------------------------------+
 * | bh | prev | next | empty ... | bt |
 * +-----------------------------------+
 *
 * Allocated block looks like
 * +-----------------------------+
 * | bh | content ... | pad | bt |
 * +-----------------------------+
 * pad is assigned only when content size < 2 * PSIZE
 *
 * Prologue block looks like
 * +-----------------------+
 * | bh | prev | next | bt |
 * +-----------------------+
 * with prev = next = 0, and allocate bit of bh and bt = 1
 */

/* Given block ptr bp, compute addr of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute addr of next/prev allocated blocks */
#define NEXT_ABP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_ABP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given block ptr bp, computer addr of next/prev free blocks */
#define NEXT_FBP(bp)  (*((char**)(bp) + 1))
#define PREV_FBP(bp)  (*((char**)(bp)))

/**
 * Globals
 */
/* Pointer to free list. Segregated size as power of 2. */
static char** free_listp = 0;
static char* prologp = 0;    /* Pointer to prologue */
static size_t max_asize = 0;  /* Max alloc size so far */

/**
 * Function prototypes
 */
static void* extend_heap(size_t size);
static void* find_fit(size_t asize);
static void* coalesce(char* bp);
static void place(char* bp, size_t asize);
static int find_segidx(size_t asize);
static void delete_node(char* bp, int segidx);
static void insert_node(char* bp, int segidx);

/*
 * Initialize - return -1 on error, 0 on success.
 */
int mm_init(void) {
    int i;
    /* Create free list */
    if ((free_listp = mem_sbrk(FL_SIZE * PSIZE)) == (void*) -1) {
        return -1;
    }

    /*
     * Size includes pad(WSIZE), prelog_header(WSIZE),
     *               prev(PSIZE), next(PSIZE),
     *               prelog_tail(WSIZE), epilog_header(WSIZE).
     */
    size_t asize = 4 * WSIZE + 2 * PSIZE;   /* size to be alloc */
    size_t psize = DSIZE + 2 * PSIZE;       /* prolog size */
    /* Create prologue and epilogue */
    if ((prologp = mem_sbrk(asize)) == (void *)-1)
        return -1;

    /* Initialize prologue and epilogue */
    PUT(prologp, 0);                            /* Alignment padding */
    prologp += DSIZE;
    PUT(HDRP(prologp), PACK(psize, 1));         /* Prologue header */
    PREV_FBP(prologp) = 0;                      /* Prologue prev */
    NEXT_FBP(prologp) = 0;                      /* Prologue next */
    PUT(FTRP(prologp), PACK(psize, 1));         /* Prologue footer */
    /* Epilogue header will serve as next allocated header */
    PUT(FTRP(prologp) + WSIZE, PACK(0, 1));     /* Epilogue header */

    /* Make free lists point to prolog */
    for (i = 0; i < FL_SIZE; i++) {
        free_listp[i] = prologp;
    }

    return 0;
}

/*
 * malloc - Allocate a block with at least size bytes of payload
 */
void* malloc(size_t size) {
    size_t asize;      /* Adjusted block size */
    char *bp;

    if (free_listp == 0){
        mm_init();
    }

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= PSIZE + PSIZE)
        /* Content should be at least 2 ptr's size */
        asize = MIN_BLKSZ;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Find a fit. Allocate when necessary */
    bp = find_fit(asize);
    place(bp, asize);
    return bp;
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(char* bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= MIN_BLKSZ) {
        delete_node(bp, -1);    /* delete BEFORE size change */
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_ABP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert_node(bp, -1);    /* insert AFTER size change */
    }
    else {
        delete_node(bp, -1);
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * find_fit - Find a fit in segregated list.
 *            Allocate when necessary.
 */
static void* find_fit(size_t asize) {
    int segidx = find_segidx(asize);

    /* Find a free block pointer; first fit */
    int i;
    char* bp;
    int found = 0;

    for (i = segidx; i < FL_SIZE; i++) {
        bp = free_listp[i];
         /* find a fit within sp */
        while (1) {
            if (GET_ALLOC(HDRP(bp))) {
                /* End of free list is prologue, prologue is allocated */
                break;
            }

            if (GET_SIZE(HDRP(bp)) >= asize) {
                found = 1;
                break;
            }

            char *next = NEXT_FBP(bp);
            if (!next) {
                break;
            }
            bp = next;
        }

        if (found) {
            return bp;
        }
    }

    bp = extend_heap(asize);

    return bp;
}

/*
 * find_segidx - Find index in segregated list
 */
static int find_segidx(size_t asize) {
    int segidx = 0;         /* Index in segregated list */
    int rsize;              /* Remaining size when finding a fit */
    for (rsize = asize / FL_MIN;
         rsize > 0 && segidx < FL_SIZE - 1;
         rsize >>= 1, segidx++);
    return segidx;
}

/*
 * insert_node - Insert bp into segidx'th free list as first node.
 *               If segidx < 0, find it.
 *               Maintain the ascending order as optimization.
 */
static void insert_node(char* bp, int segidx) {
    if (segidx < 0) {
        segidx = find_segidx(GET_SIZE(HDRP(bp)));
    }

    /* Find bp's location */
    size_t size = GET_SIZE(HDRP(bp));
    char *prev, *ptr;
    for (prev = NULL, ptr = free_listp[segidx];
         ptr != prologp && GET_SIZE(HDRP(ptr)) < size;
         prev = ptr, ptr = NEXT_FBP(ptr));

    /* Insert before ptr */
    if (prev) {     /* Insert between prev and ptr */
        NEXT_FBP(prev) = bp;
        NEXT_FBP(bp) = ptr;
        PREV_FBP(ptr) = bp;
        PREV_FBP(bp) = prev;
    } else {        /* Insert to first */
        free_listp[segidx] = bp;
        PREV_FBP(bp) = NULL;
        NEXT_FBP(bp) = ptr;
        PREV_FBP(NEXT_FBP(bp)) = bp;
    }

    /* Trick: prologp forgets prev */
    PREV_FBP(prologp) = NULL;
}

/*
 * delete_node - Delete bp from segidx'th free list.
 *               If segidx < 0, find it.
 */
static void delete_node(char* bp, int segidx) {
    if (segidx < 0) {
        segidx = find_segidx(GET_SIZE(HDRP(bp)));
    }

    char* prev = PREV_FBP(bp);
    char* next = NEXT_FBP(bp);

    /* prologp will always be the last */
    /* need to see if bp has prev */
    if (prev) {
        NEXT_FBP(prev) = next;
        PREV_FBP(next) = prev;
    } else {
        PREV_FBP(next) = NULL;      /* next no longer has prev */
        free_listp[segidx] = next;  /* first element in list becomes next */
    }

    /* Trick: prologp forgets prev */
    PREV_FBP(prologp) = NULL;
}

/*
 * extend_heap - Extend heap with at least size bytes.
 *               Return its block pointer.
 *
 *               heuristic is that, unless max was a peak, we may
 *               take max_asize as avg level of the requests, and
 *               allocate max_size as newly extended size.
 *               If max was a peak, we just allocate twice as much
 *               to save sbrk calls.
 */
static void* extend_heap(size_t size) {
    size_t words = size / WSIZE; /* Size of extension in words */
    /* Allocate an even number of words to maintain alignment */
    size_t asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if (asize > max_asize) {
        max_asize = asize;
    }

    if (max_asize > 2 * asize) {
        /* That max may just be a peak */
        asize *= 2;
    } else {
        /* Otherwise it's very likely we're just about this level */
        asize = max_asize;
    }


    char* bp;
    if ((long)(bp = mem_sbrk(asize)) == -1)
        return NULL;

    /* Initialize free block header/footer*/
    PUT(HDRP(bp), PACK(asize, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(asize, 0));         /* Free block footer */
    PUT(HDRP(NEXT_ABP(bp)), PACK(0, 1));   /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void* coalesce(char* bp) {
    char* prev_bp = PREV_ABP(bp);
    int prev_alloc = GET_ALLOC(HDRP(prev_bp));
    char* next_bp = NEXT_ABP(bp);
    int next_alloc = GET_ALLOC(HDRP(next_bp));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Cannot coalesce */
        insert_node(bp, -1);
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Coalesce next */
        delete_node(next_bp, -1);
        size += GET_SIZE(HDRP(next_bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
        insert_node(bp, -1);
    }

    else if (!prev_alloc && next_alloc) {      /* Coalesce prev */
        delete_node(prev_bp, -1);
        size += GET_SIZE(HDRP(prev_bp));
        bp = prev_bp;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_node(bp, -1);
    }

    else {                                     /* Coalesce both */
        delete_node(prev_bp, -1);
        delete_node(next_bp, -1);
        size += GET_SIZE(HDRP(prev_bp)) +
                GET_SIZE(HDRP(next_bp));
        bp = prev_bp;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_node(bp, -1);
    }

    return bp;
}

/*
 * free - Free a block
 */
void free(void *bp) {
    if (bp == 0 ||
        !GET_ALLOC(HDRP(bp))
        ) {
        return;
    }

    if (free_listp == 0){
        mm_init();
    }

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    coalesce(bp);
}

/*
 * realloc - Reallocate size for oldptr. Copy old bytes into it.
 *           If size is 0, free it.
 *           If oldptr is NULL, malloc it.
 */
void* realloc(void* oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        free(oldptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (oldptr == NULL) {
        return malloc(size);
    }

    newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(oldptr));
    if (size < oldsize) oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    free(oldptr);

    return newptr;
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size)  {
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * aligned - Return whether the pointer is aligned.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap - Check validity of heap
 */
void mm_checkheap(int lineno) {
    if (!aligned(prologp)) {
        dbg_printf("[checker] - lineno %d - prologp %p not aligned\n",
                   lineno, prologp);
    }

    if (GET_SIZE(HDRP(prologp)) != MIN_BLKSZ) {
        dbg_printf("[checker] - lineno %d - prologp %p size incorrect\n",
                   lineno, prologp);
    }

    if (!GET_ALLOC(HDRP(prologp))) {
        dbg_printf("[checker] - lineno %d - prologp %p "
                       "becomes unallocated\n",
                   lineno, prologp);
    }

    if (GET_SIZE(FTRP(prologp)) != MIN_BLKSZ) {
        dbg_printf("[checker] - lineno %d - prologp %p 's tail has "
                       "incorrect size\n",
                   lineno, prologp);
    }

    if (!GET_ALLOC(FTRP(prologp))) {
        dbg_printf("[checker] - lineno %d - prologp %p 's tail "
                       "becomes unallocated\n",
                   lineno, prologp);
    }

    /* check heap */
    char* bp = prologp;
    int last_alloc = 1;
    while (1) {
        bp = NEXT_ABP(bp);
        if (GET_ALLOC(HDRP(bp)) && !GET_SIZE(HDRP(bp))) {  /* epilog */
            break;
        }

        if (!aligned(bp)) {
            dbg_printf("[checker] - lineno %d - bp %p not aligned\n",
                       lineno, bp);
        }

        if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))) {
            dbg_printf("[checker] - lineno %d - "
                           "size of bp %p not consistent.\n",
                       lineno, bp);
            dbg_printf("          - Header size is %d, "
                           "Tail size is %d\n",
                       GET_SIZE(HDRP(bp)), GET_SIZE(FTRP(bp)));
        }

        if (GET_ALLOC(HDRP(bp)) != GET_ALLOC(FTRP(bp))) {
            dbg_printf("[checker] - lineno %d - "
                           "alloc of bp %p not consistent\n",
                       lineno, bp);
        }

        if (!last_alloc && !GET_ALLOC(HDRP(bp))) {
            dbg_printf("[checker] - lineno %d - "
                           "bp %p not coalasced\n",
                       lineno, bp);
        }
        last_alloc = GET_ALLOC(HDRP(bp));

        if (NEXT_ABP(bp) == bp) {
            dbg_printf("[checker] - bp %p has zero size\n", bp);
            break;
        }
    }

    /* check free list */
    int i;
    for (i = 0; i < FL_SIZE; i++) {
        bp = free_listp[i];

        while (1) {
            if (bp == prologp) {  /* prelog is last */
                break;
            }

            void* next = NEXT_FBP(bp);
            if (next != prologp) {
                if (PREV_FBP(next) != bp) {
                    dbg_printf("[checker] - lineno %d - "
                                   "bp %p 's next's prev isn't bp\n",
                               lineno, bp);
                }
            }

            if (!in_heap(bp)) {
                dbg_printf("[checker] - lineno %d - "
                               "bp %p is out of range\n",
                           lineno, bp);
            }

            if (GET_ALLOC(HDRP(bp))) {
                dbg_printf("[checker] - lineno %d - "
                               "bp %p in free list is not free\n",
                           lineno, bp);
            }

            size_t minsz = i == 0 ? 0 : FL_MIN << (i-1);
            size_t maxsz = i == FL_SIZE - 1 ?
                           GET_SIZE(HDRP(bp)) :
                           (unsigned) (FL_MIN << i);
            if (GET_SIZE(HDRP(bp)) < minsz ||
                GET_SIZE(HDRP(bp)) > maxsz) {
                dbg_printf("[checker] - lineno %d - "
                               "bp %p is placed in a wrong seg list\n",
                           lineno, bp);
            }

            bp = next;
        }
    }
}
