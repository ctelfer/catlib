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

typedef unsigned (*hash_f)(void *key, void *ctx);

struct hashsys {
  cmp_f			cmp;
  hash_f		hash;
  void *		hctx;
};


struct htab {
  struct list *		tab;
  unsigned int		size;
  unsigned int		po2mask;
  struct hashsys	sys;
} ;


struct hnode {
  struct list		le;
  unsigned int		hash;
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


DECL void           ht_init(struct htab *t, struct list *la, unsigned siz, 
                            struct hashsys *hs);
DECL void           ht_ninit(struct hnode *hn, void *k, void *d, unsigned h);
DECL unsigned       ht_hash(struct htab *t, void *key);
DECL struct hnode * ht_lkup(struct htab *t, void *key, unsigned *hash);
DECL struct hnode * ht_ins(struct htab *t, struct hnode *node);
DECL void           ht_rem(struct hnode *node);

DECL void           ht_apply(struct htab *t, apply_f f, void * ctx);

PTRDECL unsigned    ht_shash(void *k, void *unused);
PTRDECL unsigned    ht_phash(void *k, void *unused);
PTRDECL unsigned    ht_rhash(void *k, void *unused);


#if defined(CAT_HASH_DO_DECL) && CAT_HASH_DO_DECL


DECL void ht_init(struct htab *t, struct list *l, unsigned size, 
                  struct hashsys *hsys)
{
  unsigned i;

  abort_unless(t != NULL);
  abort_unless(l != NULL);
  abort_unless(hsys != NULL);
  abort_unless(size > 0);

  t->size = size;
  t->tab = l;
  for ( i = size ; i > 0 ; --i, ++l ) 
    l_init(l);

  t->sys = *hsys;

  /* check if size is a power of 2 */
  if ( ((~size ^ (size - 1)) & (size - 1)) == 0 )
    t->po2mask = size - 1;
  else
    t->po2mask = 0;
}


DECL void ht_ninit(struct hnode *hn, void *key, void *data, unsigned hash)
{
  abort_unless(hn != NULL);
  abort_unless(key != NULL);

  l_init(&hn->le);
  hn->key  = key;
  hn->hash = hash;
  hn->data = data;
}


DECL struct hnode * ht_lkup(struct htab *t, void *key, unsigned *hp)
{
  struct list *l, *list;
  unsigned h;
  struct hashsys *hs;

  abort_unless(t != NULL);
  abort_unless(key != NULL);

  hs = &t->sys;
  h = (*hs->hash)(key, hs->hctx);
  if ( hp ) 
    *hp = h;
  if ( t->po2mask )
    list = t->tab + (h & t->po2mask);
  else
    list = t->tab + (h % t->size);

  for ( l = list->next ; l != list ; l = l->next )
    if ( ! (*hs->cmp)(((struct hnode *)l)->key, key) )
      return (struct hnode *)l;

  return NULL;
}


DECL struct hnode * ht_ins(struct htab *t, struct hnode *n)
{
  struct hnode *old = NULL;
  struct hashsys *hs;
  void *key;
  unsigned h;
  struct list *list, *l;

  abort_unless(t != NULL);
  abort_unless(n != NULL);

  hs = &t->sys;
  key = n->key;
  h = n->hash;

  if ( t->po2mask )
    list = t->tab + (h & t->po2mask);
  else
    list = t->tab + (h % t->size);

  for ( l = list->next ; l != list ; l = l->next )
    if ( ! (*hs->cmp)(((struct hnode *)l)->key, key) ) {
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


DECL unsigned ht_hash(struct htab *t, void *key)
{
  abort_unless(t != NULL);
  return (*t->sys.hash)(key, t->sys.hctx);
}


DECL void ht_apply(struct htab *t, apply_f func, void * ctx)
{
  unsigned int num;
  struct list *bucket;

  abort_unless(t != NULL);
  abort_unless(func != NULL);

  for ( num = t->size, bucket = t->tab ; num ; --num, ++bucket )
    l_apply(bucket, func, ctx);
}


PTRDECL unsigned ht_shash(void *k, void *unused)  
{
  unsigned char *p = k;
  unsigned h = 0, g;

  abort_unless(p != NULL);

  while ( *p ) {
    h = (h << 4) + (unsigned)(*p++);
    if ( (g = h & 0xf000000) ) {
      h ^= g >> 24;
      h ^= g;
    }
  }

  return h;
}


PTRDECL unsigned ht_rhash(void *k, void *unused)  
{ 
  unsigned char *p;
  unsigned h = 0, g, l;
  struct raw *r = k;

  abort_unless(k != NULL);

  l = r->len;
  p = (unsigned char *)r->data;

  abort_unless(l != 0);
  abort_unless(l <= (unsigned)-1);
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


PTRDECL unsigned ht_phash(void *k, void *unused)  
{
  /* XXX useless code to silence the compiler about unused funcs */
  (void)ht_shash;
  (void)ht_phash;
  (void)ht_rhash;
  return (unsigned)((unsigned long)k >> 2);
}


#endif /* CAT_HASH_DO_DECL */


#undef PTRDECL
#undef DECL
#undef CAT_HASH_DO_DECL

#endif /* __cat_hash_h */
