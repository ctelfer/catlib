/*
 * cat/hash.h -- Hash table implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */

#ifndef __cat_hash_h
#define __cat_hash_h

#include <cat/cat.h>
#include <cat/aux.h>

typedef uint (*hash_f)(const void *key, void *ctx);

struct htab {
	struct hnode **	bkts;
	uint		nbkts;
	uint		po2mask;
	cmp_f		cmp;
	hash_f		hash;
	void *		hctx;
} ;


struct hnode {
	struct hnode *	next;
	struct hnode **	prevp;
	void *		key;
	void *		data;
} ;


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define PTRDECL static
#define CAT_HASH_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#define PTRDECL
#endif /* CAT_USE_INLINE */


DECL void           ht_init(struct htab *t, struct hnode **bkts, uint nbkts,
		            cmp_f cmpf, hash_f hashf, void *hctx);
DECL void           ht_ninit(struct hnode *node, void *key, void *data);
DECL uint	    ht_hash(struct htab *t, const void *key);
DECL struct hnode * ht_lkup(struct htab *t, const void *key, uint *hash);
DECL struct hnode * ht_ins(struct htab *t, struct hnode *node, uint hash);
DECL struct hnode * ht_ins_h(struct htab *t, struct hnode *node);
DECL void           ht_rem(struct hnode *node);

DECL void           ht_apply(struct htab *t, apply_f func, void * ctx);

PTRDECL uint        ht_shash(const void *key, void *unused);
PTRDECL uint        ht_phash(const void *key, void *unused);
PTRDECL uint        ht_rhash(const void *key, void *unused);


#if defined(CAT_HASH_DO_DECL) && CAT_HASH_DO_DECL


DECL void ht_init(struct htab *t, struct hnode **bkts, uint nbkts,
		  cmp_f cmp, hash_f hash, void *hctx)
{
	uint i;

	/* XXX useless code to silence the compiler about unused funcs */
	(void)ht_shash;
	(void)ht_phash;
	(void)ht_rhash;

	abort_unless(t != NULL);
	abort_unless(bkts != NULL);
	abort_unless(nbkts > 0);

	t->nbkts = nbkts;
	t->bkts = bkts;
	for ( i = 0 ; i < nbkts ; ++i )
		bkts[i] = NULL;

	/* check if size is a power of 2 */
	if ( (nbkts & (nbkts - 1)) == 0 )
		t->po2mask = nbkts - 1;
	else
		t->po2mask = 0;

	t->cmp = cmp;
	t->hash = hash;
	t->hctx = hctx;
}


DECL void ht_ninit(struct hnode *node, void *key, void *data)
{
	abort_unless(node != NULL);
	abort_unless(key != NULL);

	node->next = NULL;
	node->prevp = NULL;
	node->key  = key;
	node->data = data;
}


DECL struct hnode * ht_lkup(struct htab *t, const void *key, uint *hp)
{
	struct hnode *node;
	uint h;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	h = (*t->hash)(key, t->hctx);
	if ( hp != NULL ) 
		*hp = h;
	if ( t->po2mask )
		node = t->bkts[h & t->po2mask];
	else
		node = t->bkts[h % t->nbkts];

	while ( node != NULL ) {
		if ( !(*t->cmp)(node->key, key) )
			return node;
		node = node->next;
	}

	return NULL;
}


DECL struct hnode *ht_ins(struct htab *t, struct hnode *node, uint hash)
{
	const void *key;
	struct hnode **trav, *old;

	abort_unless(t != NULL);
	abort_unless(node != NULL);

	key = node->key;

	if ( t->po2mask )
		trav = t->bkts + (hash & t->po2mask);
	else
		trav = t->bkts + (hash % t->nbkts);

	while ( *trav != NULL ) {
		if ( ! (*t->cmp)((*trav)->key, key) ) {
			old = *trav;
			node->prevp = trav;
			node->next = old->next;
			*trav = node;
			old->next = NULL;
			old->prevp = NULL;
			return old;
		}
		trav = &(*trav)->next;
	}

	node->prevp = trav;
	node->next = *trav;
	*trav = node;

	return NULL;
}


DECL struct hnode *ht_ins_h(struct htab *t, struct hnode *node)
{
	abort_unless(node != NULL);
	return ht_ins(t, node, ht_hash(t, node->key));
}


DECL void ht_rem(struct hnode *node)
{
	struct hnode **prevp;
	abort_unless(node != NULL);
	prevp = node->prevp;
	if ( prevp != NULL ) {
		if ( (*prevp = node->next) != NULL )
			(*prevp)->prevp = prevp;
		node->prevp = NULL;
	}
	node->next = NULL;
}


DECL uint ht_hash(struct htab *t, const void *key)
{
	abort_unless(t != NULL);
	return (*t->hash)(key, t->hctx);
}


DECL void ht_apply(struct htab *t, apply_f func, void *ctx)
{
	uint num;
	struct hnode **bkt, *node, *next;

	abort_unless(t != NULL);
	abort_unless(func != NULL);

	for ( num = t->nbkts, bkt = t->bkts ; num ; --num, ++bkt ) {
		for ( node = *bkt ; node ; node = next ) {
			next = node->next;
			(*func)(node, ctx);
		}
	}
}


PTRDECL uint ht_shash(const void *key, void *unused)  
{
	const uchar *p = key;
	uint h = 0, g;

	abort_unless(p != NULL);

	while ( *p ) {
		h = (h << 4) + (uint)(*p++);
		if ( (g = h & 0xf000000) ) {
			h ^= g >> 24;
			h ^= g;
		}
	}

	return h;
}


PTRDECL uint ht_rhash(const void *key, void *unused)  
{ 
	const uchar *p;
	uint h = 0, g, l;
	struct raw const *r = key;

	abort_unless(key != NULL);

	l = r->len;
	p = (const uchar *)r->data;

	abort_unless(l != 0);
	abort_unless(l <= (uint)-1);
	abort_unless(p != NULL);

	while ( l-- ) {
		h = (h << 4) + (*p++);
		if ( (g = h & 0xf000000) ) {
			h ^= g >> 24;
			h ^= g;
		}
	}

	return h;
}


PTRDECL uint ht_phash(const void *key, void *unused)  
{
	return (uint)((ulong)key >> 2);
}


struct hash_iter {
	struct hnode **bucket;
	struct hnode *node;
};


#define ht_for_each(n, i, t)					    \
	for ( (i).bucket = (t).bkts; 				    \
	      (i).bucket < (t).bkts + (t).nbkts;		    \
	      ++(i).bucket )					    \
		for ( (n) = *(i).bucket ;			    \
		      (i).node = (((n) != NULL) ? (n)->next : NULL),\
			(n) != NULL ;				    \
		      (n) = (i).node )


#endif /* CAT_HASH_DO_DECL */


#undef PTRDECL
#undef DECL
#undef CAT_HASH_DO_DECL

#endif /* __cat_hash_h */
