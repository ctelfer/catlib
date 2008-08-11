#include <cat/dynmem.h>
#if CAT_USE_STDLIB
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */

/* core alignment type */
union align_u {
	double		d;
	long		l;
	void *		p;
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


/* global data structures including ~2 million align_u elements of memory */
/* this number is obviously configurable */
#ifndef CAT_MALLOC_MEM
#define CAT_MALLOC_MEM	(2ul * 1024 * 1024)
#endif /* CAT_MALLOC_MEM */

#ifndef CAT_MIN_MOREMEM	
#define CAT_MIN_MOREMEM	8192
#endif /* CAT_MIN_MOREMEM */

/* global data structures */
void *(*add_mem_func)(size_t len) = NULL;


void dynmem_init(struct dynmem *dm)
{
	l_init(&dm->dm_pools);
	l_init(&dm->dm_blocks);
	dm->dm_current = &dm->dm_blocks;
	dm->add_mem_func = NULL;
}


static void init_dynmem_pool(struct dynmempool *pool, struct dynmem *dm,
			     size_t tlen, size_t ulen, union align_u *start)
{
	l_ins(dm->dm_pools.prev, &pool->dmp_entry);
	pool->dmp_total_len = tlen;
	pool->dmp_useable_len = ulen;
	pool->dmp_start = start;
}


void add_dynmempool(struct dynmem *dm, void *mem, size_t len)
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
	init_dynmem_pool(&pool_u->p, dm, len, ulen, unitp);
	
	/* add sentiel at the end */
	PTR2U(unitp, ulen)->sz = ALLOC_BIT;
	obj = (struct memblk *)unitp;
	obj->mb_len.sz = ulen | PREV_ALLOC_BIT;

	dynmem_free(dm, mb2ptr(obj));
}


static void set_free_mb(struct memblk *mb, size_t size, size_t bits)
{
	mb->mb_len.sz = size | bits;
	PTR2U(mb, size - UNITSIZE)->sz = size;
}


static struct memblk *split_block(struct memblk *mb, size_t amt)
{
	struct memblk *nmb;

	/* caller must assure that amt is a multiple of UNITSIZE */
	abort_unless(amt % UNITSIZE == 0);

	nmb = (struct memblk *)PTR2U(mb, amt);
	set_free_mb(nmb, MBSIZE(mb) - amt, 0);
	l_ins(&mb->mb_entry, &nmb->mb_entry);
	set_free_mb(mb, amt, mb->mb_len.sz & PREV_ALLOC_BIT);
	return mb;
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
			l_rem(&mb->mb_entry);
			mb->mb_len.sz &= ~ALLOC_BIT;
			PTR2U(mb, amt)->sz |= PREV_ALLOC_BIT;
			dm->dm_current = mb->mb_entry.next;
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
			add_dynmempool(dm, mm, moreamt);
		goto again;
	}
	
	return NULL;
}


void dynmem_free(struct dynmem *dm, void *mem)
{
	int reinsert = 1;
	struct memblk *mb, *mb2;
	union align_u *unitp;
	size_t sz;

	abort_unless(dm);
	if ( mem == NULL )
		return;

	mb = ptr2mb(mem);
	sz = MBSIZE(mb);	
	/* note, we set the original PREV_ALLOC_BIT, but clear the ALLOC_BIT */
	set_free_mb(mb, sz, mb->mb_len.sz & PREV_ALLOC_BIT);

	if ( !(mb->mb_len.sz & PREV_ALLOC_BIT) ) {
		unitp = (union align_u *)mb - 1;
		mb = (struct memblk *)((char *)mb - unitp->sz);
		sz += unitp->sz;
		/* note that we are using aggressive coalescing, so this */
		/* the prev alloc bit must always be set here */
		set_free_mb(mb, sz, PREV_ALLOC_BIT);
		reinsert = 0;
	}

	unitp = PTR2U(mb, sz);
	if ( !(unitp->sz & ALLOC_BIT) ) {
		mb2 = (struct memblk *)unitp;
		l_rem(&mb2->mb_entry);
		sz += MBSIZE(mb2);
		/* See note at previous set_free_mb() */
		set_free_mb(mb, sz, PREV_ALLOC_BIT);
	} else {
		unitp->sz &= ~PREV_ALLOC_BIT;
	}

	if ( reinsert )
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
