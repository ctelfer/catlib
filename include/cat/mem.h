/*
 * mem.h -- memory management API
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2017 -- See accompanying license
 *
 */
#ifndef __cat_mem_h
#define __cat_mem_h
#include <cat/cat.h>
#include <cat/aux.h>

struct memmgr;
typedef void *(*alloc_f)(struct memmgr *mm, size_t size);
typedef void *(*resize_f)(struct memmgr *mm, void *old, size_t size);
typedef void  (*free_f)(struct memmgr *mm, void * tofree);


/* Base structure for a memory manager */
struct memmgr {
	alloc_f		mm_alloc;
	resize_f	mm_resize;
	free_f		mm_free;
	void *		mm_ctx;
};


/* 
 * Allocate 'len' bytes of memory from 'm'.  Returns a pointer
 * to the allocated memory or NULL if no memory is available.
 */
void *mem_get(struct memmgr *m, size_t len);

/*
 * Resize a block of memory 'mem' that 'm' allocated to length
 * 'len'.  Returns a pointer to the new memory or NULL if there
 * was an error allocating it.  If 'len' is 0, this is equivalent
 * to 'free(mem)' and returns NULL.  If 'mem' is NULL, this is 
 * equivalent to mem_get(m, len).
 */
void *mem_resize(struct memmgr *m, void *mem, size_t len);

/*
 * Free a block of memory 'mem' allocated using 'm'.
 */
void mem_free(struct memmgr *m, void *mem);


/*
 * This function is useful as an 'apply function' that gets
 * invoked over every element in some data structure.  It frees
 * each element as it gets invoked.
 */
extern void applyfree(void *data, void *memmgr);



/* 
 * This is a memory manager that operates by allocating linearly from
 * some array.  Freeing memory with this manager does not release it
 * for future use and resizing memory is unimplemented.
 */
struct arraymm {
	struct memmgr	mm;
	byte_t *	mem;
	size_t		mlen;
	size_t		fill;
	int		alignp2;
	int		hi2lo;
};


/*
 * Initialize an array memory manager 'amm' to allocate memory out of 'mem'
 * linearly either front to back (hi2lo is 0) or back to front (if
 * hi2lo is 1).  'mlen' holds the length of 'mem'.  The 'alg' parameter
 * specifies the minimum alignment that each the manager will round up
 * each allocation to.
 */
void amm_init(struct arraymm *amm, void *mem, size_t mlen, int algn, int hi2lo);

/*
 * Reset an array memory manager as if it were freshly initialized.   The
 * caller should not have any pointers to memory that 'amm' allocated
 * as this memory is all considered freed now and available for fresh
 * allocations.
 */
void amm_reset(struct arraymm *amm);

/* Return the number of bytes that 'amm' has allocated so far. */
size_t amm_get_fill(struct arraymm *amm);

/* Return the number of bytes that 'amm' has available for allocation. */
size_t amm_get_avail(struct arraymm *amm);


/* Declaration of default memmgr. */
extern struct memmgr stdmm;

#endif /* __cat_mem_h */
