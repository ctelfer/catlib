/*
 * pool.h -- fixed size element memory pools.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __pool_h
#define __pool_h

#include <cat/cat.h>
#include <cat/list.h>

#define IMAX ((int)(((unsigned)~0) >> 1))


typedef union {
	cat_align_t	align;
	struct list	list;
} cat_pitem_t;


struct pool {
	struct list	items;
	size_t		fill;
	size_t		max;
#if defined(CAT_DEBUG) && CAT_DEBUG
	char *		base;
	size_t		lim;
#endif
};


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define CAT_POOL_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#endif /* CAT_USE_INLINE */


DECL size_t pl_isiz(size_t siz, int align);
DECL void   pl_init(struct pool *p, size_t siz, int align, void *mem, 
		    size_t mlen);
DECL void * pl_alloc(struct pool *p);
DECL void   pl_free(struct pool *p, void *item);


#if defined(CAT_POOL_DO_DECL) && CAT_POOL_DO_DECL


DECL size_t pl_isiz(size_t siz, int align)
{
	size_t n, alen = 0;

	abort_unless(siz > 0);
	abort_unless(align < 0 || (align < sizeof(int) * 8 - 1));

	if ( align >= 0 && (align < sizeof(int) * 8 - 1) )
		alen = 1 << align;
	if ( alen < sizeof(cat_pitem_t) )
		alen = sizeof(cat_pitem_t);

	if ( siz < alen )
		siz = alen;

	n = siz % alen;
	if ( n > 0 ) {
		abort_unless(siz <= IMAX - (alen - n));
		siz += alen - n;
	}

	return siz;
}


DECL void pl_init(struct pool *p, size_t siz, int align, void *mem, size_t mlen)
{
	size_t i, n;
	char *cp;
	struct list *lp;

	abort_unless(p);
	abort_unless(mem);
	abort_unless(siz > 0);
	abort_unless(align < 0 || (align < sizeof(int) * 8 - 1));
	abort_unless(mlen > 0);

	siz = pl_isiz(siz, align);
	l_init(&p->items);
	n = mlen / siz;
	cp = mem;
#if defined(CAT_DEBUG) && CAT_DEBUG
	p->base = mem;
	p->lim = mlen;
#endif 
	
/*	XXX This is not the appropriate test:  what is? */
/*	abort_unless((unsigned)cp % sizeof(cat_align_t) == 0); */
	abort_unless((align < 0) || ((uint)(ptrdiff_t)cp % (1 << align) == 0));
	for ( i = 0 ; i < n ; ++i ) {
		lp = (struct list *)cp;
		l_ins(p->items.prev, lp);
		cp += siz;
	}

	p->fill = p->max = n;
}


DECL void * pl_alloc(struct pool *p)
{
	struct list *lp, *prev;

	abort_unless(p);

	if ( ! p->fill ) 
		return NULL;

	p->fill -= 1;

	prev = &p->items;
	lp = prev->next;

	abort_unless(lp && lp->next && lp->next->prev == lp);

	prev->next = lp->next;
	lp->next->prev = prev;

	return (char *)lp;
}


DECL void pl_free(struct pool *p, void *item)
{
	struct list *lp;

	abort_unless(p);
	abort_unless(item);

	lp = (struct list *)item;
#if defined(CAT_DEBUG) && CAT_DEBUG
	abort_unless((char *)item >= p->base && (char *)item < p->base+p->lim);
#endif /* CAT_DEBUG */

	l_ins(&p->items, lp);

	p->fill += 1;
}

#endif /* defined(CAT_POOL_DO_DECL) && CAT_POOL_DO_DECL */

#undef DECL
#undef CAT_POOL_DO_DECL

#endif /* __pool_h */
