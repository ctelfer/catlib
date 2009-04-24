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


void *mem_get(struct memsys *m, size_t len)
{
  if ( !m || !m->ms_alloc )
    return NULL;
  return m->ms_alloc(m, len);
}


void *mem_resize(struct memsys *m, void *mem, size_t len)
{
  if ( !m || !m->ms_resize )
    return NULL;
  return m->ms_resize(m, mem, len);
}


void mem_free(struct memsys *m, void *mem)
{
  if ( !m || !m->ms_free )
    return;
  m->ms_free(m, mem);
}


void applyfree(void *data, void *mp)
{
  struct memsys *m = mp;
  m->ms_free(m, data);
}


static void *amsys_get(struct memsys *m, size_t len)
{
  struct arraymsys *sys = (struct arraymsys *)m;
  size_t nu = (len + sys->alignp2 - 1) >> sys->alignp2;
  void *a;
  if ( nu > sys->mlen - sys->fill )
    return NULL;
  if ( sys->hi2lo ) {
    sys->fill += nu;
    a = sys->mem - (sys->fill << sys->alignp2);
  } else {
    a = sys->mem + (sys->fill << sys->alignp2);
    sys->fill += nu;
  }
  return a;
}


void amsys_init(struct arraymsys *sys, void *mem, size_t mlen, int align, 
                int hi2lo)
{
  int i = 0;
  abort_unless(sys && mem && mlen > 0 && align >= 0 && !(align & (align-1)));
  if ( align == 0 )
    align = sizeof(cat_align_t);
  sys->sys.ms_alloc = amsys_get;
  sys->sys.ms_resize = NULL;
  sys->sys.ms_free = NULL;
  sys->sys.ms_ctx = sys;
  sys->fill = 0;
  sys->mlen = mlen / align;
  if ( hi2lo )
    sys->mem = (byte_t *)mem + (sys->mlen * align);
  else
    sys->mem = mem;
  while ( align > 0 ) { /* dumb log base-2 */
    ++i;
    align >>= 1;
  }
  sys->alignp2 = i;
  sys->hi2lo = hi2lo;
}


void amsys_reset(struct arraymsys *sys)
{
  abort_unless(sys);
  sys->fill = 0;
}


size_t amsys_get_fill(struct arraymsys *sys)
{
  abort_unless(sys);
  return sys->fill << sys->alignp2;
}


size_t amsys_get_avail(struct arraymsys *sys)
{
  return (sys->mlen - sys->fill) << sys->alignp2;
}

