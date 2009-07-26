/*
 * mem.c -- funtions needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/mem.h>


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


static void *amm_get(struct memmgr *mm, size_t len)
{
	struct arraymm *amm = (struct arraymm *)mm;
	size_t nu = (len + amm->alignp2 - 1) >> amm->alignp2;
	void *a;
	if ( nu > amm->mlen - amm->fill )
		return NULL;
	if ( amm->hi2lo ) {
		amm->fill += nu;
		a = amm->mem - (amm->fill << amm->alignp2);
	} else {
		a = amm->mem + (amm->fill << amm->alignp2);
		amm->fill += nu;
	}
	return a;
}


void amm_init(struct arraymm *amm, void *mem, size_t mlen, int align, int hi2lo)
{
	int i = 0;
	abort_unless(amm && mem && mlen > 0 && align >= 0 && 
		     !(align & (align-1)));
	if ( align == 0 )
		align = sizeof(cat_align_t);
	amm->mm.mm_alloc = amm_get;
	amm->mm.mm_resize = NULL;
	amm->mm.mm_free = NULL;
	amm->mm.mm_ctx = amm;
	amm->fill = 0;
	amm->mlen = mlen / align;
	if ( hi2lo )
		amm->mem = (byte_t *)mem + (amm->mlen * align);
	else
		amm->mem = mem;
	while ( align > 0 ) { /* dumb log base-2 */
		++i;
		align >>= 1;
	}
	amm->alignp2 = i;
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


#if CAT_USE_STDLIB
#include <stdlib.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */

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
