#ifndef __cat_mem_h
#define __cat_mem_h
#include <cat/cat.h>
#include <cat/aux.h>

struct memmgr;
typedef void *(*alloc_f)(struct memmgr *mm, size_t size);
typedef void *(*resize_f)(struct memmgr *mm, void *old, size_t size);
typedef void  (*free_f)(struct memmgr *mm, void * tofree);

struct memmgr {
	alloc_f		mm_alloc;
	resize_f	mm_resize;
	free_f		mm_free;
	void *		mm_ctx;
};


void *mem_get(struct memmgr *m, size_t len);
void *mem_resize(struct memmgr *m, void *mem, size_t len);
void mem_free(struct memmgr *m, void *mem);

extern void applyfree(void *data, void *memmgr);


struct arraymm {
	struct memmgr	mm;
	byte_t *	mem;
	size_t		mlen;
	size_t		fill;
	int		alignp2;
	int		hi2lo;
};


void amm_init(struct arraymm *amm, void *mem, size_t mlen, int algn, int hi2lo);
void amm_reset(struct arraymm *amm);
size_t amm_get_fill(struct arraymm *amm);
size_t amm_get_avail(struct arraymm *amm);

#endif /* __cat_mem_h */
