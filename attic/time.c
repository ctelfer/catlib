/*
 * time.c -- timing functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#include <cat/time.h>
#include <time.h>



static void tm_fixup(struct cat_time *t)
{
  Assert(t);

  if ( t->sec > 0 ) {
    if ( t->nsec < 0 ) {
      t->nsec += 1000000000L;
      t->sec--;
    }
  } else if ( t->sec < 0 ) {
    if ( t->nsec > 0 ) {
      t->nsec -= 1000000000L;
      t->sec++;
    }
  }
}




struct cat_time *tm_add(struct cat_time *t1, struct cat_time *t2)
{
  Assert(t1);
  Assert(t2);

  t1->sec  += t2->sec;
  t1->nsec += t2->nsec;
  tm_fixup(t1);
  return t1;
}




struct cat_time *tm_sub(struct cat_time *t1, struct cat_time *t2)
{
  Assert(t1);
  Assert(t2);

  t1->sec  -= t2->sec;
  t1->nsec -= t2->nsec;
  tm_fixup(t1);
  return t1;
}




struct cat_time *tm_clr(struct cat_time *t)
{
  Assert(t);

  t->sec = -1;
  t->nsec = -1;
  return t;
}




int tm_isset(struct cat_time *t)
{
  Assert(t);
  return t->sec >= 0 && t->nsec >= 0;
}




int tm_cmp(void *t1p, void *t2p)
{
  struct cat_time *t1 = t1p, *t2 = t2p;
  long rv;

  Assert(t1);
  Assert(t2);

  if ( !(rv = t1->sec - t2->sec) )
    rv = t1->nsec - t2->nsec;

  if ( rv < 0 )
    return -1;
  else if ( rv ) 
    return 1;
  else
    return 0;
}




double tm_2dbl(struct cat_time *t)
{
  Assert(t);
  return t->sec + (double)t->nsec / (double)1000000000;
}




struct cat_time *tm_setd(struct cat_time *t, double d)
{
  Assert(t);
  t->sec  = (long)d;
  t->nsec = (long)((d - t->sec) * 1000000000L);
  return t;
}




struct cat_time *tm_setl(struct cat_time *t, long sec, long nsec)
{
  Assert(t);
  Assert(sec >= 0);
  Assert(nsec >= 0 && nsec < 1000000000l);
  t->sec  = sec;
  t->nsec = nsec;
  return t;
}




struct cat_time *tm_mark(struct cat_time *old, struct cat_time *new)
{
  struct cat_time hold;

  Assert(new);
  Assert(old);

  hold = *new;
  tm_sub(new, old);
  *old = hold;
  
  return new;
}





struct cat_time *tm_cget(struct cat_time *t)
{
  clock_t cur;

  Assert(t);

  cur = clock();
  t->sec  = cur / CLOCKS_PER_SEC;
  t->nsec = (long)((double)(cur - t->sec)/(double)CLOCKS_PER_SEC * 1000000000L);
  return t;
}





#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB


#include <sys/time.h>

struct cat_time *tm_uget(struct cat_time *t)
{
  struct timeval cur;

  Assert(t);

  gettimeofday(&cur, NULL);
  t->sec  = cur.tv_sec;
  t->nsec = cur.tv_usec * 1000;

  return t;
}


#endif /* CAT_USE_STDLIB */
