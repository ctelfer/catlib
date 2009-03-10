/*
 * ring.c -- Ring Buffer Implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004  See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/ring.h>
#if CAT_USE_STDLIB
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


#define CKRING(r)							       \
  do {	abort_unless(r); 					       \
    abort_unless(((r)->alloc > 0) && ((r)->start < (r)->alloc));   \
    abort_unless((r)->len <= (r)->alloc);			       \
  } while (0)


void ring_init(struct ring *r, void *d, size_t l)
{
  abort_unless(r);
  abort_unless(l);
  abort_unless(d);

  r->start = 0;
  r->len   = 0;
  r->alloc = l;
  r->data  = d;
}


void ring_reset(struct ring *r)
{
  abort_unless(r);
  r->start = 0;
  r->len   = 0;
}


size_t ring_put(struct ring *r, char *in, size_t len, int overwrite)
{
  size_t last, toend, avail;

  CKRING(r);

  avail = ring_avail(r);
  if ( !overwrite && (len > avail) )
    len = avail;

  if ( !in ) {
    abort_unless(len <= avail);
    r->len += len;
    return len;
  }
  
  if ( len > r->alloc ) {
    /* if the input is bigger than the buffer itself, just put in */
    /* the last r->alloc-sized chunk of the input.  Might as well */
    /* align it while we're at it.  */
    abort_unless(overwrite);
    r->start = 0;
    r->len = r->alloc;
    memcpy(r->data, in + (len - r->alloc), r->alloc);
    return len;
  } 
  
  last = ring_last(r); /* need to get last before possibly moving start */

  if ( len > avail ) {
    /* We've got more than we have space for, but less than */
    /* a buffer's worth.  Some old data stays in the buffer. */
    /* So adjust the start position accordingly */
    size_t ovfl = len - avail;
    abort_unless(ovfl < r->len);
    if ( r->start < r->alloc - ovfl )
      r->start += ovfl;
    else
      r->start = ovfl - (r->alloc - r->start);
  }

  toend = r->alloc - last;
  if ( (last >= r->start) && (toend < len) ) {
    memcpy(r->data + last, in, toend);
    memcpy(r->data, in + toend, len - toend);
  } else {
    memcpy(r->data + last, in, len);
  }

  if ( r->alloc - r->len < len )
    r->len = r->alloc;
  else
    r->len += len;

  return len;
}


size_t ring_get(struct ring *r, char *out, size_t len)
{
  size_t toend;

  CKRING(r);

  if ( len > r->len )
    len = r->len;

  toend = r->alloc - r->start;
  if ( toend < len ) {
    if ( out ) {
      memcpy(out, r->data + r->start, toend);
      memcpy(out + toend, r->data, len - toend);
    }
    r->start = len - toend;
    r->len -= len;
  } else {
    if ( out )
      memcpy(out, r->data + r->start, len);
    r->start += len;
    if ( r->start == r->alloc )
      r->start = 0;
    r->len -= len;
  }

  /* Optimization: if ring is empty, then we can reset the start */
  if ( r->len == 0 )
    r->start = 0;

  return len;
}


size_t ring_last(struct ring *r)
{
  size_t toend;

  CKRING(r);

  toend = r->alloc - r->start;
  if ( r->len < toend )
    return r->start + r->len;
  else
    return r->len - toend;
}


#undef CKRING
