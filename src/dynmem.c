/*
 * src/dynmem.c -- Dynamic memory management
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2008-2015 See accompanying license
 *
 */

#include <cat/dynmem.h>
#include <cat/archops.h>
#include <string.h>

union dynmempool_u {
	union align_u		u;
	struct dynmempool	p;
};

#define sizetonu(n)	(((size_t)(n) + UNITSIZE - 1) / UNITSIZE)
#define nutosize(n)	((size_t)(n) * UNITSIZE)
#define round2u(n)	(nutosize(sizetonu(n)))

#define MINNU 		sizetonu(sizeof(struct memblk)+UNITSIZE)
#define MINSZ		(MINNU * UNITSIZE)
#define MINASZ		nutosize(MINNU + 2)
#define mb2ptr(mb)	((void *)((union align_u *)(mb) + 1))
#define ptr2mb(ptr)	((struct memblk *)((union align_u *)(ptr) - 1))
#define ALLOC_BIT	((size_t)1)
#define PREV_ALLOC_BIT	((size_t)2)
#define CTLBMASK	(ALLOC_BIT|PREV_ALLOC_BIT)
#define MAX_ALLOC	round2u((size_t)~0 - UNITSIZE)
#define MBSIZE(p)	(((union align_u *)(p))->sz & ~CTLBMASK)
#define PTR2U(p, o)	((union align_u *)((char *)(p) + (o)))


#ifndef CAT_MIN_MOREMEM	
#define CAT_MIN_MOREMEM	8192
#endif /* CAT_MIN_MOREMEM */

/* global data structures */
void *(*add_mem_func)(size_t len) = NULL;


static void set_free_mb(struct memblk *mb, size_t size, size_t bits)
{
	mb->mb_len.sz = size | bits;
	PTR2U(mb, size - UNITSIZE)->sz = size;
}


static struct memblk *split_block(struct memblk *mb, size_t amt)
{
	struct memblk *nmb;
	size_t flags;

	/* caller must assure that amt is a multiple of UNITSIZE */
	abort_unless(amt % UNITSIZE == 0);

	flags = mb->mb_len.sz & CTLBMASK;
	nmb = (struct memblk *)PTR2U(mb, amt);
	set_free_mb(nmb, MBSIZE(mb) - amt, flags);
	l_ins(&mb->mb_entry, &nmb->mb_entry);
	set_free_mb(mb, amt, flags);
	return mb;
}


static void mark_free(struct memblk *mb)
{
	abort_unless(mb);
	/* note, we set the original PREV_ALLOC_BIT, but clear the ALLOC_BIT */
	set_free_mb(mb, MBSIZE(mb), mb->mb_len.sz & PREV_ALLOC_BIT);
}


#define MERGEBACK	1
#define MERGEFRONT	2
static int coalesce(struct memblk **mbparam)
{
	struct memblk *mb, *nmb;
	union align_u *unitp;
	size_t sz;
	int rv = 0;

	abort_unless(mbparam && *mbparam);
	mb = *mbparam;
	sz = MBSIZE(mb);	

	if ( !(mb->mb_len.sz & PREV_ALLOC_BIT) ) {
		unitp = (union align_u *)mb - 1;
		mb = (struct memblk *)((char *)mb - unitp->sz);
		l_rem(&mb->mb_entry);
		sz += unitp->sz;
		/* note that we are using aggressive coalescing, so this */
		/* the prev alloc bit must always be set here: be safe   */
		/* in case we decide to do delayed coalescing some time later */
		set_free_mb(mb, sz, mb->mb_len.sz & CTLBMASK);
		rv |= MERGEBACK;
	}

	unitp = PTR2U(mb, sz);
	if ( !(unitp->sz & ALLOC_BIT) ) {
		nmb = (struct memblk *)unitp;
		l_rem(&nmb->mb_entry);
		sz += MBSIZE(nmb);
		set_free_mb(mb, sz, mb->mb_len.sz & CTLBMASK);
		rv |= MERGEFRONT;
	} else {
		unitp->sz &= ~PREV_ALLOC_BIT;
	}

	*mbparam = mb;

	return rv;
}


void dynmem_init(struct dynmem *dm)
{
	l_init(&dm->dm_pools);
	l_init(&dm->dm_blocks);
	dm->dm_current = &dm->dm_blocks;
	dm->add_mem_func = NULL;
	dm->dm_init = 1;
}


static void dynmem_init_pool(struct dynmempool *pool, struct dynmem *dm,
			     size_t tlen, size_t ulen, union align_u *start)
{
	l_ins(dm->dm_pools.prev, &pool->dmp_entry);
	pool->dmp_total_len = tlen;
	pool->dmp_useable_len = ulen;
	pool->dmp_start = start;
}


void dynmem_add_pool(struct dynmem *dm, void *mem, size_t len)
{
	struct memblk *obj;
	union dynmempool_u *pool_u;
	union align_u *unitp;
	size_t ulen;

	ulen = nutosize(len / UNITSIZE); /* round down to unit size */
	abort_unless(ulen >= MINASZ);
	abort_unless((ulong)mem % UNITSIZE == 0);

	pool_u = mem;
	unitp = (union align_u *)(pool_u + 1);
	ulen -= sizeof(*pool_u) + UNITSIZE;
	dynmem_init_pool(&pool_u->p, dm, len, ulen, unitp);
	
	/* add sentiel at the end */
	PTR2U(unitp, ulen)->sz = ALLOC_BIT;
	obj = (struct memblk *)unitp;
	obj->mb_len.sz = ulen | PREV_ALLOC_BIT;

	dynmem_free(dm, mb2ptr(obj));
}


void *dynmem_malloc(struct dynmem *dm, size_t amt)
{
	struct list *t;
	struct memblk *mb;
	void *mm = NULL;
	size_t moreamt;

	abort_unless(dm);

	/* the first condition tests for overflow */
	amt += UNITSIZE;
	if ( !dm->dm_init || amt < UNITSIZE || amt > MAX_ALLOC )
		return NULL;

	if ( amt < MINSZ )
		amt = MINSZ;
	else
		amt = round2u(amt); /* round up size */

again:
	/* first fit search that starts at the previous allocation */
	t = dm->dm_current;
	do { 
		if ( t == &dm->dm_blocks ) {
			t = t->next;
			continue;
		}
		mb = container(t, struct memblk, mb_entry);
		if ( MBSIZE(mb) >= amt ) {
			if ( MBSIZE(mb) > amt + MINSZ )
				mb = split_block(mb, amt);
			else
				amt = MBSIZE(mb);
			dm->dm_current = mb->mb_entry.next;
			l_rem(&mb->mb_entry);
			mb->mb_len.sz |= ALLOC_BIT;
			PTR2U(mb, amt)->sz |= PREV_ALLOC_BIT;
			return mb2ptr(mb);
		}
		t = t->next;
	} while ( t != dm->dm_current );

	/* should not get here twice */
	abort_unless(mm == NULL);

	if ( add_mem_func ) {
		/* add 2 units for boundary sentinels */
		moreamt = amt + UNITSIZE + sizeof(union dynmempool_u);
		if ( moreamt < CAT_MIN_MOREMEM )
			moreamt = CAT_MIN_MOREMEM;
		mm = (*add_mem_func)(moreamt);
		if ( !mm )
			return NULL;
		else
			dynmem_add_pool(dm, mm, moreamt);
		goto again;
	}
	
	return NULL;
}


void dynmem_free(struct dynmem *dm, void *mem)
{
	struct memblk *mb;

	abort_unless(dm);
	if ( !dm->dm_init || mem == NULL )
		return;

	mb = ptr2mb(mem);
	mark_free(mb);
	coalesce(&mb);
	/* if the following block was "dm_current" adjust */
	if (((char *)dm->dm_current >= (char *)mb) && 
			((char *)dm->dm_current <  ((char *)mb + MBSIZE(mb))))
		dm->dm_current = &mb->mb_entry;
	l_ins(dm->dm_current->prev, &mb->mb_entry);
}


#ifndef CAT_MIN_ALLOC_SHRINK
#define CAT_MIN_ALLOC_SHRINK 128
#endif  /* CAT_MIN_ALLOC_SHRINK */
static void shrink_alloc_block(struct dynmem *dm, struct memblk *mb, size_t sz)
{
	size_t nbsz;
	struct memblk *nmb;
	union align_u *unitp;

	/* caller should assure that sz is a multiple of UNITSIZE */
	abort_unless(sz % UNITSIZE == 0);
	nbsz = MBSIZE(mb) - sz;
	if ( !dm->dm_init || nbsz < MINSZ )
		return;

	unitp = PTR2U(mb, MBSIZE(mb));
	if ( !(unitp->sz & ALLOC_BIT) ) {
		/* expand the next block?  OK if small since no fragmentation */
		nbsz += MBSIZE(unitp);
		l_rem(&((struct memblk *)unitp)->mb_entry);
		/* remove the block and fall through to create the new one */
	} else if ( nbsz < CAT_MIN_ALLOC_SHRINK ) {
		/* only shrink current block if size savings is worth it */
		return;
	} 
	nmb = (struct memblk *)PTR2U(mb, sz);
	set_free_mb(nmb, nbsz, PREV_ALLOC_BIT);
	l_ins(dm->dm_current, &nmb->mb_entry);
	mb->mb_len.sz = sz | (mb->mb_len.sz & CTLBMASK);
}


void *dynmem_realloc(struct dynmem *dm, void *omem, size_t newamt)
{
	void *nmem = NULL;
	struct memblk *mb;
	union align_u *lenp;
	size_t tsz;

	abort_unless(dm);
	if ( newamt == 0 ) {
		dynmem_free(dm, omem);
		return NULL;
	}

	if ( !omem )
		return dynmem_malloc(dm, newamt);

	tsz = round2u(newamt);
	if ( (tsz < newamt) || (tsz > MAX_ALLOC - UNITSIZE) )
		return NULL;
	newamt = tsz + UNITSIZE;

	mb = ptr2mb(omem);
	if ( mb->mb_len.sz >= newamt ) {
		shrink_alloc_block(dm, mb, newamt);
		return omem;
	}

	/* See if we can just add the next adjacent block */
	lenp = PTR2U(mb, MBSIZE(mb));
	if ( !(lenp->sz & ALLOC_BIT) )  {
		struct memblk *nmb = (struct memblk *)lenp;
		tsz = MBSIZE(mb) + MBSIZE(nmb);
		if ( tsz >= newamt ) {
			size_t delta = tsz - newamt;
			if ( MBSIZE(nmb) > delta + MINSZ )
				nmb = split_block(nmb, delta);
			/* remove the block and merge it with the alloced one */
			l_rem(&nmb->mb_entry);
			lenp = PTR2U(nmb, MBSIZE(nmb));
			lenp->sz |= PREV_ALLOC_BIT;
			/* addition doesn't colide with flags since they are */
			/* in the low order bits */
			mb->mb_len.sz += MBSIZE(nmb);
			return mb2ptr(mb);
		}
	}

	/* at this point we need a completely new block and must copy */
	nmem = dynmem_malloc(dm, newamt);
	if ( nmem ) { 
		/* may copy more than the original alloc, but still in bounds */
		memcpy(nmem, omem, MBSIZE(mb));
		dynmem_free(dm, omem);
	}

	return nmem;
}


void dynmem_each_pool(struct dynmem *dm, apply_f f, void *ctx)
{
	abort_unless(dm);
	l_apply(&dm->dm_pools, f, ctx);
}


void dynmem_each_block(struct dynmempool *pool, apply_f f, void *ctx)
{
	union align_u *unitp, *endp;
	struct dynmem_block_fake blk;

	abort_unless(pool);
	abort_unless(f);

	unitp = pool->dmp_start;
	endp = (union align_u *)((char *)unitp + pool->dmp_useable_len);
	while ( unitp < endp ) { 
		blk.ptr = unitp;
		blk.allocated = (unitp->sz & ALLOC_BIT) != 0;
		blk.prev_allocated = (unitp->sz & PREV_ALLOC_BIT) != 0;
		blk.size = MBSIZE(unitp);
		f(&blk, ctx);
		unitp = PTR2U(unitp, MBSIZE(unitp));
	}
}


/* ----------------- Two-Layer Segregated Fit ----------------- */


STATIC_BUG_ON(bad_lg2_unitsize, ((1 << TLSF_LG2_UNITSIZE) != UNITSIZE));

#if CAT_DEBUG_LEVEL > 0
#define ASSERT(x) abort_unless(x)
#else /* CAT_DEBUG_LEVEL > 0 */
#define ASSERT(x)
#endif /* CAT_DEBUG_LEVEL > 0 */

static void tlsf_init_pool(struct tlsfpool *pool, struct tlsf *tlsf,
		           size_t tlen, size_t ulen, union align_u *start);

union tlsfpool_u {
	union align_u		u;
	struct tlsfpool		p;
};

void tlsf_init(struct tlsf *tlsf)
{
	int i;
	struct list *listp;
	struct tlsf_l2 *tl2;

	abort_unless((1 << TLSF_LG2_UNITSIZE) == UNITSIZE);
	abort_unless(tlsf);

	memset(tlsf, 0, sizeof(*tlsf));
	l_init(&tlsf->tlsf_pools);
	for (i = 0; i < TLSF_NUMHEADS; i++)
		l_init(&tlsf->tlsf_lists[i]);
	listp = tlsf->tlsf_lists;

	/* NOTE, even though we initialize them, we never use bins */
	/* for allocations of less that TLSF_MINSZ blocks.  This will */
	/* typically be about 3 lists wasted.  But it makes the rest */
	/* of the code cleaner. */
	for (i = 0; i < TLSF_NUML2; i++) {
		tl2 = &tlsf->tlsf_l1[i];
		tl2->tl2_blists = listp;
		if ( i < TLSF_NUMSMALL )
			tl2->tl2_nblists = 1 << i;
		else
			tl2->tl2_nblists = TLSF_FULLBLLEN;
		listp += tl2->tl2_nblists;
	}

	abort_unless(listp - tlsf->tlsf_lists <= TLSF_NUMHEADS);
}


static int calc_tlsf_indices(struct tlsf *tlsf, size_t len)
{
	int i, j, n;
	/* len must be a multiple of 4! */
	ASSERT((len > 0) && (len & (UNITSIZE-1)) == 0);

	n = tlsf_nlz(len);
	ASSERT(n > 1 && n <= (TLSF_SZ_BITS - TLSF_LG2_UNITSIZE));
	/* 1 << (i + LG2_UNITSIZE) is the first 1 bit in len */
	i = (TLSF_SZ_BITS - 1 - TLSF_LG2_UNITSIZE) - n;
	/* subtract off the most significant bit */
	j = len - (1 << (i + TLSF_LG2_UNITSIZE));
	if ( len < (TLSF_FULLBLLEN * UNITSIZE) )
		j /= UNITSIZE;
	else
		j >>= i - (TLSF_L2_LEN - TLSF_LG2_UNITSIZE);
	return (i << 8) + j;
}


static int round_next_tlsf_size(struct tlsf *tlsf, size_t amt, int idx)
{
	int l1 = idx >> 8;
	int l2 = idx & 0xFF;
	size_t bktlen;
	ASSERT(tlsf && amt && idx > 0);

	/* the buffers up to including the first full-buffer list all */
	/* have buckets that match the size of all requests exactly */
	if ( l1 <= TLSF_L2_LEN )
		return idx;
	bktlen = (1 << (l1 + TLSF_LG2_UNITSIZE));
	bktlen += bktlen / TLSF_FULLBLLEN * l2;
	if ( amt <= bktlen )
		return idx;

	l2 += 1;
	if ( l2 >= tlsf->tlsf_l1[l1].tl2_nblists ) { 
		l1 += 1;
		l2 = 0;
	}
	if ( l1 >= TLSF_LG2_ALIM - TLSF_LG2_UNITSIZE )
		return -1;
	return (l1 << 8) + l2;
}


static void tlsf_ins_blk(struct tlsf *tlsf, struct memblk *mb, int l1, int l2)
{
	struct tlsf_l2 *tl2;
	ASSERT(tlsf);
	tlsf->tlsf_l1bm |= ((tlsf_sz_t)1 << l1);
	tl2 = &tlsf->tlsf_l1[l1];
	tl2->tl2_bm |= ((tlsf_sz_t)1 << l2);
	l_ins(&tl2->tl2_blists[l2], &mb->mb_entry);
}

static void tlsf_ins_blk_c(struct tlsf *tlsf, struct memblk *mb)
{
	int idx;
	idx = calc_tlsf_indices(tlsf, MBSIZE(mb));
	tlsf_ins_blk(tlsf, mb, idx >> 8, idx & 0xFF);
}

static void tlsf_rem_blk(struct tlsf *tlsf, struct memblk *mb, int l1, int l2)
{
	struct tlsf_l2 *tl2;
	l_rem(&mb->mb_entry);
	tl2 = &tlsf->tlsf_l1[l1];
	if ( l_isempty(&tl2->tl2_blists[l2]) ) { 
		tl2->tl2_bm &= ~((tlsf_sz_t)1 << l2);
		if ( tl2->tl2_bm == 0 )
			tlsf->tlsf_l1bm &= ~((tlsf_sz_t)1 << l1);
	}
}

static void tlsf_rem_blk_c(struct tlsf *tlsf, struct memblk *mb)
{
	int idx;
	idx = calc_tlsf_indices(tlsf, MBSIZE(mb));
	tlsf_rem_blk(tlsf, mb, idx >> 8, idx & 0xFF);
}


/* remove a block, split a block in 2 and reinsert the second one, return */
/* the first block */
static struct memblk *tlsf_split_blk(struct tlsf *tlsf, struct memblk *mb, 
		                     size_t amt)
{
	struct memblk *nmb;
	size_t flags;

	/* caller must assure that amt is a multiple of UNITSIZE */
	ASSERT(amt % UNITSIZE == 0);

	tlsf_rem_blk_c(tlsf, mb);
	flags = mb->mb_len.sz & CTLBMASK;
	nmb = (struct memblk *)PTR2U(mb, amt);
	set_free_mb(nmb, MBSIZE(mb) - amt, flags);
	set_free_mb(mb, amt, flags);

	tlsf_ins_blk_c(tlsf, nmb);
	return mb;
}


static void tlsf_coalesce_and_insert(struct tlsf *tlsf, struct memblk *mb)
{
	struct memblk *nmb;
	union align_u *unitp;
	size_t sz;

	ASSERT(mb);
	/* mark the block free first */
	sz = MBSIZE(mb);
	set_free_mb(mb, sz, mb->mb_len.sz & PREV_ALLOC_BIT);

	/* join with the previous block if it is free */
	if ( !(mb->mb_len.sz & PREV_ALLOC_BIT) ) {
		unitp = (union align_u *)mb - 1;
		mb = (struct memblk *)((char *)mb - unitp->sz);
		sz += unitp->sz;
		tlsf_rem_blk_c(tlsf, mb);
		/* note that we are using aggressive coalescing, so this */
		/* the prev alloc bit must always be set here: be safe   */
		/* in case we decide to do delayed coalescing some time later */
		set_free_mb(mb, sz, PREV_ALLOC_BIT);
	}

	/* join with the next block if it is free */
	unitp = PTR2U(mb, sz);
	if ( !(unitp->sz & ALLOC_BIT) ) {
		nmb = (struct memblk *)unitp;
		tlsf_rem_blk_c(tlsf, nmb);
		sz += MBSIZE(nmb);
		set_free_mb(mb, sz, PREV_ALLOC_BIT);
	} else {
		/* otherwise clear the next block's PREV_ALLOC_BIT */
		unitp->sz &= ~PREV_ALLOC_BIT;
	}

	tlsf_ins_blk_c(tlsf, mb);
}


void tlsf_add_pool(struct tlsf *tlsf, void *mem, size_t len)
{
	struct memblk *obj;
	union tlsfpool_u *pool_u;
	union align_u *unitp;
	size_t ulen;

	ulen = nutosize(len / UNITSIZE); /* round down to UNITSIZE */
	ASSERT(ulen >= TLSF_MINPOOL + sizeof(*pool_u));
	ASSERT((ulong)mem % UNITSIZE == 0);

	pool_u = mem;
	unitp = (union align_u *)(pool_u + 1);
	ulen -= sizeof(*pool_u) + UNITSIZE;
	tlsf_init_pool(&pool_u->p, tlsf, len, ulen, unitp);
	
	/* add sentiel at the end */
	PTR2U(unitp, ulen)->sz = ALLOC_BIT;
	obj = (struct memblk *)unitp;
	obj->mb_len.sz = ulen | PREV_ALLOC_BIT | ALLOC_BIT;

	tlsf_free(tlsf, mb2ptr(obj));
}


static void tlsf_init_pool(struct tlsfpool *pool, struct tlsf *tlsf,
		           size_t tlen, size_t ulen, union align_u *start)
{
	l_ins(tlsf->tlsf_pools.prev, &pool->tpl_entry);
	pool->tpl_total_len = tlen;
	pool->tpl_useable_len = ulen;
	pool->tpl_start = start;
}


static struct list *tlsf_find_blk(struct tlsf *tlsf, int *idx)
{
	tlsf_sz_t n;
	int l1, l2;
	struct tlsf_l2 *tl2;
	ASSERT(tlsf && idx);
	l1 = (*idx) >> 8;
	l2 = (*idx) & 0xFF;

	/* first search for best fit within l2 bin */
	tl2 = &tlsf->tlsf_l1[l1];
	n = tl2->tl2_bm & ~(((tlsf_sz_t)1 << l2) - 1);
	if ( n == 0 ) {
		tlsf_sz_t tmp = (tlsf_sz_t)1 << l1;
		/* if there is no fit, search for the closest l1 */
		/* rule out the current l1 */
		n = tlsf->tlsf_l1bm & ~(tmp | (tmp - 1));
		if ( n == 0 ) /* if still nothing, we're out of luck */
			return NULL;
		l1 = tlsf_ntz(n);
		tl2 = &tlsf->tlsf_l1[l1];
		l2 = tlsf_ntz(tl2->tl2_bm);
	} else { 
		l2 = tlsf_ntz(n);
	} 

	ASSERT(l2 < tl2->tl2_nblists);
	*idx = (l1 << 8) + l2;
	return &tl2->tl2_blists[l2];
}


static void *tlsf_extract(struct tlsf *tlsf, struct list *head, size_t amt, 
			  int idx)
{
	struct memblk *mb = container(l_head(head), struct memblk, mb_entry);
	union align_u *nextp;
	ASSERT(MBSIZE(mb) >= amt);
	if ( MBSIZE(mb) - amt >= TLSF_MINSZ ) {
		mb = tlsf_split_blk(tlsf, mb, amt);
	} else {
		tlsf_rem_blk(tlsf, mb, idx >> 8, idx & 0xFF);
	}
	mb->mb_len.sz |= ALLOC_BIT;
	nextp = PTR2U(mb, MBSIZE(mb));
	nextp->sz |= PREV_ALLOC_BIT;
	return mb2ptr(mb);
}


void *tlsf_malloc(struct tlsf *tlsf, size_t req_size)
{
	int idx;
	struct list *head;
	void *mm = NULL;
	size_t amt = req_size;

	ASSERT(tlsf);

	/* the first condition tests for overflow */
	if ( amt < (TLSF_MINSZ - UNITSIZE) ) {
		amt = TLSF_MINSZ;
	} else {
		/* add in one UNIT and round up to UNITSIZE */
		amt += (UNITSIZE << 1) - 1;
		amt &= ~(UNITSIZE - 1);
		/* check for overflow or size too big */
		if ( amt >= TLSF_ALIM || amt < req_size )
			return NULL;
	}
	idx = calc_tlsf_indices(tlsf, amt);
	idx = round_next_tlsf_size(tlsf, amt, idx);
	if ( idx < 0 )
		return NULL;

again:
	if ( (head = tlsf_find_blk(tlsf, &idx)) != NULL ) {
		return tlsf_extract(tlsf, head, amt, idx);
	}

	/* should not get here twice */
	ASSERT(mm == NULL);

	if ( add_mem_func ) {
		size_t moreamt;
		/* add 2 units for boundary sentinels */
		moreamt = amt + UNITSIZE + sizeof(union tlsfpool_u);
		if ( moreamt < CAT_MIN_MOREMEM )
			moreamt = CAT_MIN_MOREMEM;
		mm = (*add_mem_func)(moreamt);
		if ( !mm )
			return NULL;
		else
			tlsf_add_pool(tlsf, mm, moreamt);
		goto again;
	}
	
	return NULL;
}


void tlsf_free(struct tlsf *tlsf, void *mem)
{
	ASSERT(tlsf);
	if ( mem == NULL )
		return;
	tlsf_coalesce_and_insert(tlsf, ptr2mb(mem));
}


static void tlsf_shrink_blk(struct tlsf *tlsf, struct memblk *mb, size_t sz)
{
	size_t delta, nbsz;
	struct memblk *nmb;
	union align_u *unitp;

	/* caller should assure that sz is a multiple of UNITSIZE */
	ASSERT(sz % UNITSIZE == 0);
	delta = nutosize((MBSIZE(mb) - sz) / UNITSIZE);
	if ( delta < TLSF_MINSZ )
		return;

	unitp = PTR2U(mb, MBSIZE(mb));
	if ( !(unitp->sz & ALLOC_BIT) ) {
		nmb = (struct memblk *)unitp;
		nbsz = MBSIZE(nmb);
		tlsf_rem_blk_c(tlsf, nmb);
		nbsz += delta;
		nmb = (struct memblk *)((char *)nmb - delta);
	} else if ( delta < CAT_MIN_ALLOC_SHRINK ) {
		/* only shrink current block if size savings is worth it */
		return;
	} else { 
		unitp->sz &= ~PREV_ALLOC_BIT;
		nbsz = delta;
		nmb = (struct memblk *)PTR2U(mb, sz);
	}
	set_free_mb(nmb, nbsz, PREV_ALLOC_BIT);
	tlsf_ins_blk_c(tlsf, nmb);
	mb->mb_len.sz = sz | (mb->mb_len.sz & CTLBMASK);
}


void *tlsf_realloc(struct tlsf *tlsf, void *omem, size_t newamt)
{
	void *nmem = NULL;
	struct memblk *mb;
	union align_u *lenp;
	size_t tsz, osize;

	ASSERT(tlsf);
	if ( newamt == 0 ) {
		tlsf_free(tlsf, omem);
		return NULL;
	}

	if ( !omem )
		return tlsf_malloc(tlsf, newamt);

	tsz = round2u(newamt);
	if ( (tsz < newamt) || (tsz > MAX_ALLOC - UNITSIZE) )
		return NULL;
	newamt = tsz + UNITSIZE;

	mb = ptr2mb(omem);
	osize = MBSIZE(mb) - sizeof(union align_u); 
	if ( mb->mb_len.sz >= newamt ) {
		tlsf_shrink_blk(tlsf, mb, newamt);
		return omem;
	}

	/* See if we can just add the next adjacent block */
	lenp = PTR2U(mb, MBSIZE(mb));
	if ( !(lenp->sz & ALLOC_BIT) )  {
		size_t delta = round2u(newamt - MBSIZE(mb));
		size_t nextsz = MBSIZE(lenp);
		if ( delta <= nextsz ) {
			struct memblk *nmb = (struct memblk *)lenp;
			if ( nextsz > delta + TLSF_MINSZ )
				nmb = tlsf_split_blk(tlsf, nmb, delta);
			else
				tlsf_rem_blk_c(tlsf, nmb);
			lenp = PTR2U(nmb, MBSIZE(nmb));
			lenp->sz |= PREV_ALLOC_BIT;
			/* addition doesn't colide with flags since they are */
			/* in the low order bits */
			mb->mb_len.sz += MBSIZE(nmb);
			return mb2ptr(mb);
		}
	}

	/* at this point we need a completely new block and must copy */
	nmem = tlsf_malloc(tlsf, newamt);
	if ( nmem ) { 
		/* may copy more than the original alloc, but still in bounds */
		memcpy(nmem, omem, osize);
		tlsf_free(tlsf, omem);
	}

	return nmem;
}


void tlsf_each_pool(struct tlsf *tlsf, apply_f f, void *ctx)
{
	ASSERT(tlsf);
	ASSERT(f);
	l_apply(&tlsf->tlsf_pools, f, ctx);
}


void tlsf_each_block(struct tlsfpool *pool, apply_f f, void *ctx)
{
	union align_u *unitp, *endp;
	struct tlsf_block_fake blk;

	ASSERT(pool);
	ASSERT(f);

	unitp = pool->tpl_start;
	endp = (union align_u *)((char *)unitp + pool->tpl_useable_len);
	while ( unitp < endp ) { 
		blk.ptr = unitp;
		blk.allocated = (unitp->sz & ALLOC_BIT) != 0;
		blk.prev_allocated = (unitp->sz & PREV_ALLOC_BIT) != 0;
		blk.size = MBSIZE(unitp);
		f(&blk, ctx);
		unitp = PTR2U(unitp, MBSIZE(unitp));
	}
}

