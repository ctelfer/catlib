/*
 * cat/heap.h -- Array-based heap implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#ifndef __cat_heap_h
#define __cat_heap_h

#include <cat/cat.h>
#include <cat/mem.h>

#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define PTRDECL static
#define LOCAL static inline
#define CAT_HEAP_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#define PTRDECL
#define LOCAL static
#endif /* CAT_USE_INLINE */


struct heap {
  int			size;
  int			fill;
  void **		elem;
  cmp_f			cmp;
  struct memmgr *	mm;
};


DECL void   hp_init(struct heap *hp, void **elem, int size, int fill, 
                    cmp_f cmp, struct memmgr *mm);
DECL int    hp_add(struct heap *hp, void *elem, int *pos);
DECL int    hp_find(struct heap *hp, void *data);
DECL void * hp_extract(struct heap *hp);
DECL void * hp_rem(struct heap *hp, int elem);


#if defined(CAT_HEAP_DO_DECL) && CAT_HEAP_DO_DECL

LOCAL int reheapup(struct heap *hp, int pos)
{
  int ppos = (pos-1) >> 1;
  void *hold;

  abort_unless(hp);

  if ( pos >= hp->fill ) 
    return 0;

  while ( (pos > 0) && (hp->cmp(hp->elem[ppos], hp->elem[pos]) > 0) ) {
    hold = hp->elem[pos];
    hp->elem[pos] = hp->elem[ppos];
    hp->elem[ppos] = hold;
    pos = ppos;
    ppos = (pos-1) >> 1;
  }

  return pos;
}


LOCAL void reheapdown(struct heap *hp, int pos)
{
  int didswap;
  int cld;
  void *hold;

  abort_unless(hp);

  if ( pos >= hp->fill ) 
    return;

  do {
    didswap = 0;
    if ( (cld = (pos << 1) + 1) >= hp->fill )
      break;

    if ( ( cld + 1 < hp->fill ) && 
         ( hp->cmp(hp->elem[cld], hp->elem[cld+1]) > 0 ) )
      cld += 1;

    if ( hp->cmp(hp->elem[pos], hp->elem[cld]) > 0 ) {
      didswap = 1;
      hold = hp->elem[cld];
      hp->elem[cld] = hp->elem[pos];
      hp->elem[pos] = hold;
      pos = cld;
    }
  } while (didswap);
}


DECL void hp_init(struct heap *hp, void **elem, int size, int fill,
                  cmp_f cmp, struct memmgr *mm)
{
  int i;

  abort_unless(hp);
  abort_unless(size >= 0);
  abort_unless(fill >= 0);
  abort_unless(cmp);
  hp->size = size; 
  hp->elem = elem;
  hp->cmp  = cmp;
  hp->mm = mm;

  if ( (hp->fill = fill) ) {
    if ( fill > size ) 		/* sanity check */
      hp->fill = size;

    for ( i = hp->fill >> 1 ; i >= 0 ; i-- ) 
      reheapdown(hp, i);
  }
}


DECL int hp_add(struct heap *hp, void *elem, int *pos)
{
  void * p;
  int n;

  abort_unless(hp);

  if ( hp->fill == hp->size ) {
    if ( ! hp->mm ) 
      return -1;

    if ( ! hp->size )
      n = 32;
    else
      n = hp->size << 1;

    if ( n < hp->size ) /* XXX check for overflow */
      return -1;
    
    p = mem_resize(hp->mm, hp->elem, n * sizeof(void *));
    if ( p == NULL )
      return -1;

    hp->elem = p;
    hp->size = n;
  }

  hp->elem[n = hp->fill++] = elem;
  n = reheapup(hp, n);

  if ( pos ) 
    *pos = n;
  return 0;
}


DECL int hp_find(struct heap *hp, void *data)
{
  int i;

  abort_unless(hp);
  
  for ( i = 0 ; i < hp->fill ; ++i ) 
    if ( ! hp->cmp(hp->elem[i], data) )
      return i;

  return -1;
}


DECL void * hp_extract(struct heap *hp)
{
  void *hold;
  int last;

  abort_unless(hp);

  last = hp->fill - 1;

  if ( hp->fill == 0 )
    return NULL;

  hold           = hp->elem[0];
  hp->elem[0]    = hp->elem[last];
  hp->elem[last] = hold;

  hp->fill -= 1;

  reheapdown(hp, 0);

  return hold;
}


DECL void * hp_rem(struct heap *hp, int elem)
{
  void *hold;
  int last = hp->fill-1;

  abort_unless(hp);
  abort_unless(elem >= 0);

  if ( elem >= hp->fill ) 
    return NULL;

  hold           = hp->elem[elem];
  hp->elem[elem] = hp->elem[last];
  hp->elem[last] = hold;

  hp->fill = last;

  if ( reheapup(hp, elem) == elem )
    reheapdown(hp, elem);

  return hp->elem[last];
}

#endif /* if defined(CAT_HEAP_DO_DECL) && CAT_HEAP_DO_DECL */


#undef PTRDECL
#undef DECL
#undef LOCAL

#endif /* __cat_heap_h */
