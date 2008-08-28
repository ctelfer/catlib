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
#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG
	long long	ll;
#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */
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
	if ( !(rv & MERGEBACK) )
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
		tsz = MBSIZE(mb) + MBSIZE(lenp);
		if ( tsz > newamt ) {
			set_free_mb(mb, tsz, mb->mb_len.sz & CTLBMASK);
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


#if 0
#include <cat/cattypes.h>
#if CAT_64BIT

#ifndef TLSF_LG2_MAX_ALLOC
#define TLSF_LG2_MAX_ALLOC 63
#define TLSF_MAX_ALLOC (1LL << TLSF_LG2_MAX_ALLOC)
#endif /* TLSF_MAX_ALLOC */

typedef uint64_t tlsf_sz_t;
#define TLSF_SZ_BITS 64
#define TLSF_LG2_MINSZ 5 /* XXX guess: verify by assert */

#ifndef TLSF_L2_LEN
#define TLSF_L2_LEN 6
#endif /* TLSF_L2_LEN */
#if TLSF_L2_LEN > 6
#error "Max supported L2 len is 6 in a 64-bit architecture"
#endif /* TLSF_L2_LEN */

#else /* CAT_64BIT */

#ifndef TLSF_LG2_MAX_ALLOC
#define TLSF_LG2_MAX_ALLOC 31
#define TLSF_MAX_ALLOC (1L << TLSF_LG2_MAX_ALLOC)
#endif /* TLSF_MAX_ALLOC */

typedef uint32_t tlsf_sz_t;
#define TLSF_SZ_BITS 32
#define TLSF_LG2_MINSZ 4 /* XXX guess: verify by assert */

#ifndef TLSF_L2_LEN
#define TLSF_L2_LEN 5
#endif /* TLSF_L2_LEN */
#if TLSF_L2_LEN > 5
#error "Max supported L2 len is 5 in a 32-bit architecture"
#endif /* TLSF_L2_LEN */

#endif /* CAT_64BIT */


#define NUML2 (TLSF_LG2_MAX_ALLOC - TLSF_LG2_MINSZ + 1)
#define LG2FULLBLLEN (TLSF_LG2_MINSZ + TLSF_L2_LEN)
#define FULLBLLEN (1 << (TLSF_LG2_MINSZ + TLSF_L2_LEN))
#define NUMFULL  (TLSF_LG2_MAX_ALLOC - LG2FULLBLLEN + 1)
#define NUMSMALL (NUML2 - NUMFULL)

/* TLSF_L2_LEN list heads for each list with a # of MINSZ slots >= to min size
   for the list head.  Then consider the smaller lists.  Number of slots there
   is 1 for slot MINSZ, 2 for slot MINSIZ * 2, 4 for slot MINSZ * 4 ... So
   there are 2^NUMSMALL-1 blocks in all.
*/
#define NUMHEADS ((NUMFULL * TLSF_L2_LEN) + ((1 << (NUMSMALL)) - 1))


struct tlsf { 
	struct list	tlsf_pools;
	tlsf_sz_t 	tlsf_l1bm;
	struct tlsf_l2  tlsf_l1[NUML2];
	struct list 	tlsf_lists[NUMHEADS];
};

struct tlsf_l2 {
	tlsf_sz_t	tl2_bm;
	struct list *	tl2_blists;
};


void tlsf_init(struct tlsf *tlsf, void *mem, size_t len);
void tlsf_add_pool(struct tlsf *tlsf, void *mem, size_t len);
static void tlsf_init_pool(struct tlsfpool *pool, struct tlsf *tlsf,
			     size_t tlen, size_t ulen, union align_u *start);
void *tlsf_malloc(struct tlsf *tlsf, size_t amt);
void tlsf_free(struct tlsf *tlsf, void *mem);
void *tlsf_realloc(struct tlsf *tlsf, void *omem, size_t newamt);
void tlsf_each_pool(struct tlsf *tlsf, apply_f f, void *ctx);
void tlsf_each_block(struct tlsfpool *pool, apply_f f, void *ctx);


void tlsf_init(struct tlsf *tlsf)
{
	int i;
	struct list *listp;
	abort_unless((1 << TLSF_LG2_MINSZ) == MINSZ);
	abort_unless(tlsf);

	memset(tlsf, 0, sizeof(tlsf));
	l_init(&tlsf->tlsf_pools);
	for (i = 0; i < NUMHEADS; i++)
		l_init(&tlsf->tlsf_lists[i]);
	listp = tlsf->tlsf_lists;
	for (i = 0; i < NUMSMALL; i++) {
		tlsf->tlsf_l2[i].tl2_blists = listp;
		listp += (1 << i);
	}
	for ( ; i < NUML2; i++ ) { 
		tlsf->tlsf_l2[i].tl2_blists = listp;
		listp += (1 << TLSF_L2_LEN);
	}
}


/* assumes CHAR_BIT == 8 */
static int nlz(tlsf_sz_t x)
{
	int i = 1, b, n = 0;
	do {
		b = nlz8(x >> (TLSF_SZ_BITS - (i << 3)));
		n += b;
		i++
	} while ( b < 8 && i <= sizeof(x) );
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
	return pop(~x & (x-1));
}


static void calc_tslf_indices(size_t len, int *l1, int *l2)
{
	int i, j;
	/* len must be a multiple of 16! */
	abort_unless((len & 0xF) == 0);
	abort_unless(l1);
	abort_unless(l2);

	i = TLSF_SZ_BITS - nlz(len) - TLSF_LG2_MINSZ;
	if (len < FULLBLLEN) {
		j = (len - (1 << i)) / MINSZ;
	else
		j = ((len - (1 << i)) >> (i - TSLF_L2_LEN)) & 
	            ((1 << TSLF_L2_LEN) - 1);
	*l1 = i;
	*l2 = j;
}


static round_next_tlsf_size(int *l1, int *l2)
{
	abort_unless(l1);
	abort_unless(l2);
	*l2 += 1;
	if ( *l1 + TLSF_LG2_MINSZ >= LG2FULLBLEN ) { 
		if ( *l2 == TSLF_L2_LEN ) {
			*l2 = 0;
			*l1 += 1;
		}
	} else {
		if ( (MINSZ * *l2) == (1 << (*l1 + MINSZ)) ) {
			*l2 = 0;
			*l1 += 1;
		}
	}
}


void tlsf_add_pool(struct tlsf *tlsf, void *mem, size_t len)
{
}


static void tlsf_init_pool(struct tlsfpool *pool, struct tlsf *tlsf,
			   size_t tlen, size_t ulen, union align_u *start)
{
}


void *tlsf_malloc(struct tlsf *tlsf, size_t amt)
{
	int l1, l2;
	struct list *head;

	abort_unless(tlsf);

	/* the first condition tests for overflow */
	amt += UNITSIZE;
	if ( amt < UNITSIZE)
		return NULL;
	if ( amt < MINSZ )
		amt = MINSZ;
	else {
		amt = round2u(amt); /* round up size */ 
		if ( amt > TLSF_MAX_ALLOC )
			return NULL;
	}

	calc_tlsf_indices(amt, &l1, &l2);
	round_next_tlsf_size(&l1, &l2);

again:
	if ( (head = find_bin(tlsf, l1, l2)) != NULL ) {
		return extract_blk(tlsf, head, amt);
	}

	/* should not get here twice */
	abort_unless(mm == NULL);

	if ( add_mem_func ) {
		void *mm = NULL;
		size_t moreamt;
		/* add 2 units for boundary sentinels */
		moreamt = amt + UNITSIZE + sizeof(union dynmempool_u);
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
}


void *tlsf_realloc(struct tlsf *tlsf, void *omem, size_t newamt)
{
}


void tlsf_each_pool(struct tlsf *tlsf, apply_f f, void *ctx)
{
}


void tlsf_each_block(struct tlsfpool *pool, apply_f f, void *ctx)
{
}


#endif
