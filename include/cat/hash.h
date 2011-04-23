/*
 * cat/hash.h -- Hash table implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2006 See accompanying license
 *
 */

#ifndef __cat_hash_h
#define __cat_hash_h

#include <cat/cat.h>
#include <cat/aux.h>
#include <cat/list.h>

typedef uint (*hash_f)(const void *key, void *ctx);

struct htab {
	struct list *	tab;
	uint		size;
	uint		po2mask;
	cmp_f		cmp;
	hash_f		hash;
	void *		hctx;
} ;


struct hnode {
	struct list	le;
	uint		hash;
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


DECL void           ht_init(struct htab *t, struct list *la, uint siz, 
		            cmp_f cmp, hash_f hash, void *hctx);
DECL void           ht_ninit(struct hnode *hn, void *k, void *d, uint h);
DECL uint	    ht_hash(struct htab *t, const void *key);
DECL struct hnode * ht_lkup(struct htab *t, const void *key, uint *hash);
DECL struct hnode * ht_ins(struct htab *t, struct hnode *node);
DECL void           ht_rem(struct hnode *node);

DECL void           ht_apply(struct htab *t, apply_f f, void * ctx);

PTRDECL uint        ht_shash(const void *k, void *unused);
PTRDECL uint        ht_phash(const void *k, void *unused);
PTRDECL uint        ht_rhash(const void *k, void *unused);


#if defined(CAT_HASH_DO_DECL) && CAT_HASH_DO_DECL


DECL void ht_init(struct htab *t, struct list *l, uint size, 
		  cmp_f cmp, hash_f hash, void *hctx)
{
	uint i;

	/* XXX useless code to silence the compiler about unused funcs */
	(void)ht_shash;
	(void)ht_phash;
	(void)ht_rhash;

	abort_unless(t != NULL);
	abort_unless(l != NULL);
	abort_unless(size > 0);

	t->size = size;
	t->tab = l;
	for ( i = size ; i > 0 ; --i, ++l ) 
		l_init(l);

	/* check if size is a power of 2 */
	if ( ((~size ^ (size - 1)) & (size - 1)) == 0 )
		t->po2mask = size - 1;
	else
		t->po2mask = 0;

	t->cmp = cmp;
	t->hash = hash;
	t->hctx = hctx;
}


DECL void ht_ninit(struct hnode *hn, void *key, void *data, uint hash)
{
	abort_unless(hn != NULL);
	abort_unless(key != NULL);

	l_init(&hn->le);
	hn->key  = key;
	hn->hash = hash;
	hn->data = data;
}


DECL struct hnode * ht_lkup(struct htab *t, const void *key, uint *hp)
{
	struct list *l, *list;
	uint h;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	h = (*t->hash)(key, t->hctx);
	if ( hp ) 
		*hp = h;
	if ( t->po2mask )
		list = t->tab + (h & t->po2mask);
	else
		list = t->tab + (h % t->size);

	for ( l = list->next ; l != list ; l = l->next )
		if ( !(*t->cmp)(((struct hnode *)l)->key, key) )
			return (struct hnode *)l;

	return NULL;
}


DECL struct hnode * ht_ins(struct htab *t, struct hnode *n)
{
	struct hnode *old = NULL;
	const void *key;
	uint h;
	struct list *list, *l;

	abort_unless(t != NULL);
	abort_unless(n != NULL);

	key = n->key;
	h = n->hash;

	if ( t->po2mask )
		list = t->tab + (h & t->po2mask);
	else
		list = t->tab + (h % t->size);

	for ( l = list->next ; l != list ; l = l->next )
		if ( ! (*t->cmp)(((struct hnode *)l)->key, key) ) {
			old = (struct hnode *)l;
			l_rem(&old->le);
			break;
		}

	l_ins(list, &n->le);

	return old;
}


DECL void ht_rem(struct hnode *node)
{
	abort_unless(node != NULL);
	l_rem(&node->le);
}


DECL uint ht_hash(struct htab *t, const void *key)
{
	abort_unless(t != NULL);
	return (*t->hash)(key, t->hctx);
}


DECL void ht_apply(struct htab *t, apply_f func, void * ctx)
{
	uint num;
	struct list *bucket;

	abort_unless(t != NULL);
	abort_unless(func != NULL);

	for ( num = t->size, bucket = t->tab ; num ; --num, ++bucket )
		l_apply(bucket, func, ctx);
}


PTRDECL uint ht_shash(const void *k, void *unused)  
{
	const uchar *p = k;
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


PTRDECL uint ht_rhash(const void *k, void *unused)  
{ 
	const uchar *p;
	uint h = 0, g, l;
	struct raw const *r = k;

	abort_unless(k != NULL);

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


PTRDECL uint ht_phash(const void *k, void *unused)  
{
	return (uint)((ulong)k >> 2);
}


#endif /* CAT_HASH_DO_DECL */


#undef PTRDECL
#undef DECL
#undef CAT_HASH_DO_DECL

#endif /* __cat_hash_h */
