/*
 * mem.c -- memory management interface
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/mem.h>
#include <stdlib.h>


void *mem_get(struct memmgr *mm, size_t len)
{
	if ( !mm || !mm->mm_alloc )
		return NULL;
	return mm->mm_alloc(mm, len);
}


void *mem_resize(struct memmgr *mm, void *mem, size_t len)
{
	if ( !mm || !mm->mm_resize )
		return NULL;
	return mm->mm_resize(mm, mem, len);
}


void mem_free(struct memmgr *mm, void *mem)
{
	if ( !mm || !mm->mm_free )
		return;
	mm->mm_free(mm, mem);
}


void applyfree(void *data, void *mp)
{
	struct memmgr *mm = mp;
	mm->mm_free(mm, data);
}


static void *amm_get_hi2lo(struct memmgr *mm, size_t len)
{
	struct arraymm *amm = container(mm, struct arraymm, mm); 
	size_t nu = (len + (1 << amm->alignp2) - 1) >> amm->alignp2;
	if ( nu > amm->mlen - amm->fill )
		return NULL;
	amm->fill += nu;
	return amm->mem - (amm->fill << amm->alignp2);
}


static void *amm_get_lo2hi(struct memmgr *mm, size_t len)
{
	struct arraymm *amm = container(mm, struct arraymm, mm); 
	size_t nu = (len + (1 << amm->alignp2) - 1) >> amm->alignp2;
	void *a;
	if ( nu > amm->mlen - amm->fill )
		return NULL;
	a = amm->mem + (amm->fill << amm->alignp2);
	amm->fill += nu;
	return a;
}


void amm_init(struct arraymm *amm, void *mem, size_t mlen, int align, int hi2lo)
{
	int i = 0;
	abort_unless(amm && mem && mlen > 0 && align >= 0 && 
		     !(align & (align-1)));
	if ( align == 0 )
		align = sizeof(cat_align_t);
	while ( (align >>= 1) > 0 ) /* Dumb log base-2 */
		++i;
	amm->fill = 0;
	amm->alignp2 = i;
	amm->mlen = mlen >> i;
	if ( hi2lo ) {
		amm->mm.mm_alloc = amm_get_hi2lo;
		amm->mem = (byte_t *)mem + (amm->mlen << i);
	} else {
		amm->mm.mm_alloc = amm_get_lo2hi;
		amm->mem = mem;
	}
	amm->mm.mm_resize = NULL;
	amm->mm.mm_free = NULL;
	amm->mm.mm_ctx = amm;
	amm->hi2lo = hi2lo;
}


void amm_reset(struct arraymm *amm)
{
	abort_unless(amm);
	amm->fill = 0;
}


size_t amm_get_fill(struct arraymm *amm)
{
	abort_unless(amm);
	return amm->fill << amm->alignp2;
}


size_t amm_get_avail(struct arraymm *amm)
{
	abort_unless(amm);
	return (amm->mlen - amm->fill) << amm->alignp2;
}


static void * std_alloc(struct memmgr *mm, size_t size) 
{
	abort_unless(mm && size > 0);
	return malloc(size);
}


static void * std_resize(struct memmgr *mm, void *old, size_t newsize) 
{
	abort_unless(mm && newsize > 0);
	return realloc(old, newsize);
}


static void std_free(struct memmgr *mm, void *old) 
{
	abort_unless(mm);
	free(old);
}


struct memmgr stdmm = { 
	std_alloc,
	std_resize,
	std_free, 
	&stdmm
};
