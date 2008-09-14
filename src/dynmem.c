#include <cat/dynmem.h>
#if CAT_USE_STDLIB
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */

/* core alignment type */
union align_u {
	long		l;
	size_t		sz;
};

union dynmempool_u {
	union align_u		u;
	struct dynmempool	p;
};

/* XXX assumes that alignment is a power of 2 */
#define UNITSIZE	sizeof(union align_u)
#define sizetonu(n)	(((size_t)(n) + UNITSIZE - 1) / UNITSIZE)
#define nutosize(n)	((size_t)(n) * UNITSIZE)
#define round2u(n)	(nutosize(sizetonu(n)))

struct memblk {
	union align_u		mb_len;
	struct list		mb_entry;
};

#define MINNU 		sizetonu(sizeof(struct memblk)+UNITSIZE)
#define MINSZ		(MINNU * UNITSIZE)
#define MINASZ		(nutosize(MINNU + nutosize(2)))
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
	abort_unless((unsigned long)mem % UNITSIZE == 0);

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
	if ( amt < UNITSIZE || amt > MAX_ALLOC )
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
	int rv;
	struct memblk *mb;

	abort_unless(dm);
	if ( mem == NULL )
		return;

	mb = ptr2mb(mem);
	mark_free(mb);
	rv = coalesce(&mb);
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
	if ( nbsz < MINSZ )
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

#if CAT_HAS_FIXED_WIDTH

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

	memset(tlsf, 0, sizeof(tlsf));
	l_init(&tlsf->tlsf_pools);
	for (i = 0; i < NUMHEADS; i++)
		l_init(&tlsf->tlsf_lists[i]);
	listp = tlsf->tlsf_lists;
	for (i = 0; i < NUML2; i++) {
		tl2 = &tlsf->tlsf_l1[i];
		tl2->tl2_blists = listp;
		if ( i < NUMSMALL )
			tl2->tl2_nblists = 1 << i;
		else
			tl2->tl2_nblists = 1 << TLSF_L2_LEN;
		listp += tl2->tl2_nblists;
	}
}


/* assumes CHAR_BIT == 8 */
static int nlz(tlsf_sz_t x)
{
	int i = 1, b, n = 0;
	do {
		b = nlz8(x >> (TLSF_SZ_BITS - (i << 3)));
		n += b;
		i++;
	} while ( b == 8 && i <= sizeof(x) );
	return n;
}


/* assumes CHAR_BIT == 8 */
static int pop(tlsf_sz_t x)
{
	int n = 0;
	while (x) {
		n += nbits8(x);
		x >>= 8;
	}
	return n;
}


static int ntz(tlsf_sz_t x)
{
	/* number of  bits in a mask consisting of all 1s after the  */
	/* last bit set */
	return pop(~x & (x-1));
}


static void calc_tlsf_indices(struct tlsf *tlsf, size_t len, int *l1, int *l2)
{
	int i, j, n;
	/* len must be a multiple of 16! */
	abort_unless((len & (UNITSIZE-1)) == 0);
	abort_unless(l1);
	abort_unless(l2);

	n = nlz(len);
	abort_unless(n > 0 && n < TLSF_SZ_BITS);
	i = TLSF_SZ_BITS - n - (TLSF_LG2_UNITSIZE + 1); 
	/* subtract off the most significant bit */
	j = len - (1 << (i + TLSF_LG2_UNITSIZE));
	if ( len < FULLBLLEN )
		j /= UNITSIZE;
	else
		j >>= TLSF_SZ_BITS - (n + TLSF_L2_LEN + 1);
	*l1 = i;
	*l2 = j;
}


static int round_next_tlsf_size(struct tlsf *tlsf, size_t *amt, int *l1, 
				int *l2)
{
	abort_unless(tlsf && amt && l1 && l2);
	*l2 += 1;
	if ( *l2 >= tlsf->tlsf_l1[*l1].tl2_nblists ) { 
		*l1 += 1;
		*l2 = 0;
	}
	if ( (unsigned)l1 >= TLSF_LG2_ALIM - TLSF_LG2_UNITSIZE )
		return -1;
	return 0;
}


static void tlsf_ins_blk(struct tlsf *tlsf, struct memblk *mb, int l1, int l2)
{
	struct tlsf_l2 *tl2;
	abort_unless(tlsf);
	tlsf->tlsf_l1bm |= (1 << l1);
	tl2 = &tlsf->tlsf_l1[l1];
	tl2->tl2_bm |= (1 << l2);
	l_ins(tl2->tl2_blists[l2].prev, &mb->mb_entry);
}

static void tlsf_ins_blk_c(struct tlsf *tlsf, struct memblk *mb)
{
	int l1, l2;
	calc_tlsf_indices(tlsf, MBSIZE(mb), &l1, &l2);
	tlsf_ins_blk(tlsf, mb, l1, l2);
}

static void tlsf_rem_blk(struct tlsf *tlsf, struct memblk *mb, int l1, int l2)
{
	struct tlsf_l2 *tl2;
	l_rem(&mb->mb_entry);
	tl2 = &tlsf->tlsf_l1[l1];
	if ( l_isempty(&tl2->tl2_blists[l2]) ) { 
		tl2->tl2_bm &= ~((tlsf_sz_t)1 << l2);
		if ( tl2->tl2_bm == 0 )
			tlsf->tlsf_l1bm &= !((tlsf_sz_t)1 << l1);
	}
}

static void tlsf_rem_blk_c(struct tlsf *tlsf, struct memblk *mb)
{
	int l1, l2;
	calc_tlsf_indices(tlsf, MBSIZE(mb), &l1, &l2);
	tlsf_rem_blk(tlsf, mb, l1, l2);
}


/* remove a block, split a block in 2 and reinset the second one, return */
/* the first block */
static struct memblk *tlsf_split_blk(struct tlsf *tlsf, struct memblk *mb, 
				     size_t amt)
{
	struct memblk *nmb;
	size_t flags;

	/* caller must assure that amt is a multiple of UNITSIZE */
	abort_unless(amt % UNITSIZE == 0);

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

	abort_unless(mb);
	/* mark the block free first */
	sz = MBSIZE(mb);	
	set_free_mb(mb, sz, mb->mb_len.sz & CTLBMASK);

	/* join with the previous block if it is free */
	if ( !(mb->mb_len.sz & PREV_ALLOC_BIT) ) {
		unitp = (union align_u *)mb - 1;
		mb = (struct memblk *)((char *)mb - unitp->sz);
		tlsf_rem_blk_c(tlsf, mb);
		sz += unitp->sz;
		/* note that we are using aggressive coalescing, so this */
		/* the prev alloc bit must always be set here: be safe   */
		/* in case we decide to do delayed coalescing some time later */
		set_free_mb(mb, sz, mb->mb_len.sz & CTLBMASK);
	}

	/* join with the next block if it is free */
	unitp = PTR2U(mb, sz);
	if ( !(unitp->sz & ALLOC_BIT) ) {
		nmb = (struct memblk *)unitp;
		tlsf_rem_blk_c(tlsf, mb);
		sz += MBSIZE(nmb);
		set_free_mb(mb, sz, mb->mb_len.sz & CTLBMASK);
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
	abort_unless(ulen >= MINASZ);
	abort_unless((unsigned long)mem % UNITSIZE == 0);

	pool_u = mem;
	unitp = (union align_u *)(pool_u + 1);
	ulen -= sizeof(*pool_u) + UNITSIZE;
	tlsf_init_pool(&pool_u->p, tlsf, len, ulen, unitp);
	
	/* add sentiel at the end */
	PTR2U(unitp, ulen)->sz = ALLOC_BIT;
	obj = (struct memblk *)unitp;
	obj->mb_len.sz = ulen | PREV_ALLOC_BIT;

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


static struct list *tlsf_find_blk(struct tlsf *tlsf, int *l1, int *l2)
{
	int n, rl1;
	struct tlsf_l2 *tl2;

	n = tlsf->tlsf_l1bm & ~(((tlsf_sz_t)1 << *l1) - 1);
	if ( n == 0 )
		return NULL;
	rl1 = ntz(n);
	/* if the best fit bin not available then set l2 to 0 and reset l1 */
	if (rl1 != *l1) { 
		*l2 = 0;
		*l1 = rl1;
	}
	tl2 = &tlsf->tlsf_l1[rl1];
	n = tl2->tl2_bm & ~(((tlsf_sz_t)1 << *l2) - 1);
	abort_unless(n != 0);
	*l2 = ntz(n);
	return &tl2->tl2_blists[*l2];
}


static void *tlsf_extract(struct tlsf *tlsf, struct list *head, size_t amt, 
			  int l1, int l2)
{
	struct memblk *mb = container(l_head(head), struct memblk, mb_entry);
	if ( MBSIZE(mb) + amt < MINSZ ) {
		mb = tlsf_split_blk(tlsf, mb, amt);
	} else {
		tlsf_rem_blk(tlsf, mb, l1, l2);
	}
	mb->mb_len.sz |= ALLOC_BIT;
	return mb2ptr(mb);
}


void *tlsf_malloc(struct tlsf *tlsf, size_t amt)
{
	int l1, l2;
	struct list *head;
	void *mm = NULL;

	abort_unless(tlsf);

	/* the first condition tests for overflow */
	amt += UNITSIZE;
	if ( amt < UNITSIZE )
		return NULL;
	if ( amt < MINSZ )
		amt = MINSZ;
	else {
		amt = round2u(amt); /* round up size */ 
		if ( amt >= TLSF_ALIM )
			return NULL;
	}
	calc_tlsf_indices(tlsf, amt, &l1, &l2);
	if ( round_next_tlsf_size(tlsf, &amt, &l1, &l2) < 0 )
		return NULL;

again:
	if ( (head = tlsf_find_blk(tlsf, &l1, &l2)) != NULL )
		return tlsf_extract(tlsf, head, amt, l1, l2);

	/* should not get here twice */
	abort_unless(mm == NULL);

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
	struct memblk *mb;

	abort_unless(tlsf);
	if ( mem == NULL )
		return;

	mb = ptr2mb(mem);
	mark_free(mb);
	tlsf_coalesce_and_insert(tlsf, mb);
}


static void tlsf_shrink_blk(struct tlsf *tlsf, struct memblk *mb, size_t sz)
{
	size_t delta, nbsz;
	struct memblk *nmb;
	union align_u *unitp;

	/* caller should assure that sz is a multiple of UNITSIZE */
	abort_unless(sz % UNITSIZE == 0);
	delta = nutosize((MBSIZE(mb) - sz) / UNITSIZE);
	if ( delta < MINSZ )
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

	abort_unless(tlsf);
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
		if ( delta > nextsz ) {
			struct memblk *nmb = (struct memblk *)lenp;
			if ( nextsz > delta + MINSZ )
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
	abort_unless(tlsf);
	abort_unless(f);
	l_apply(&tlsf->tlsf_pools, f, ctx);
}


void tlsf_each_block(struct tlsfpool *pool, apply_f f, void *ctx)
{
	union align_u *unitp, *endp;
	struct tlsf_block_fake blk;

	abort_unless(pool);
	abort_unless(f);

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

#endif /* CAT_HAS_FIXED_WIDTH */
