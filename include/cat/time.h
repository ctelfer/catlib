/*
 * cat/time.h -- Functions for timing events
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004 See accompanying license
 *
 */
#ifndef __cat_time_h
#define __cat_time_h

#include <cat/cat.h>

#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define CAT_TIME_DO_DECL 1
#define LDECL static inline
#else /* CAT_USE_INLINE */
#define DECL
#define LDECL static
#endif /* CAT_USE_INLINE */


struct cat_time {
	long	sec;
	long	nsec;
};


typedef struct cat_time *(*gettime_f)(struct cat_time *t);

DECL struct cat_time *tm_add(struct cat_time *t1, struct cat_time *t2);
DECL struct cat_time *tm_sub(struct cat_time *t1, struct cat_time *t2);
DECL struct cat_time *tm_clr(struct cat_time *t);
DECL int              tm_isset(struct cat_time *t);

DECL int              tm_cmp(void *t1, void *t2);
DECL double           tm_2dbl(struct cat_time *t);
DECL struct cat_time *tm_dset(struct cat_time *t, double d);
DECL struct cat_time *tm_lset(struct cat_time *t, long sec, long nsec);
DECL struct cat_time *tm_mark(struct cat_time *old, struct cat_time *tnew);

#if CAT_USE_STDLIB
#include <time.h>
DECL struct cat_time *tm_cget(struct cat_time *t);
#endif /* CAT_USE_STDLIB */


#if defined(CAT_TIME_DO_DECL) && CAT_TIME_DO_DECL

LDECL void tm_fixup(struct cat_time *t)
{
	abort_unless(t);

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


DECL struct cat_time *tm_add(struct cat_time *t1, struct cat_time *t2)
{
	abort_unless(t1);
	abort_unless(t2);

	if ( !tm_isset(t1) || !tm_isset(t2) )
		return NULL;

	t1->sec  += t2->sec;
	t1->nsec += t2->nsec;
	tm_fixup(t1);
	return t1;
}


DECL struct cat_time *tm_sub(struct cat_time *t1, struct cat_time *t2)
{
	abort_unless(t1);
	abort_unless(t2);

	if ( !tm_isset(t1) || !tm_isset(t2) )
		return NULL;

	t1->sec  -= t2->sec;
	t1->nsec -= t2->nsec;
	tm_fixup(t1);
	return t1;
}


DECL struct cat_time *tm_clr(struct cat_time *t)
{
	abort_unless(t);

	t->sec = -1;
	t->nsec = -1;
	return t;
}


DECL int tm_isset(struct cat_time *t)
{
	abort_unless(t);

	return t->sec >= 0 && t->nsec >= 0;
}


DECL int tm_cmp(void *t1p, void *t2p)
{
	struct cat_time *t1 = t1p, *t2 = t2p;
	long rv;

	abort_unless(t1);
	abort_unless(t2);

	if ( !(rv = t1->sec - t2->sec) )
		rv = t1->nsec - t2->nsec;

	if ( rv < 0 )
		return -1;
	else if ( rv ) 
		return 1;
	else
		return 0;
}


DECL double tm_2dbl(struct cat_time *t)
{
	abort_unless(t);

	return t->sec + (double)t->nsec / (double)1000000000;
}


DECL struct cat_time *tm_dset(struct cat_time *t, double d)
{
	abort_unless(t);

	t->sec  = (long)d;
	t->nsec = (long)((d - t->sec) * 1000000000L);
	return t;
}


DECL struct cat_time *tm_lset(struct cat_time *t, long sec, long nsec)
{
	abort_unless(t);
	abort_unless(sec >= 0);
	abort_unless(nsec >= 0 && nsec < 1000000000l);

	t->sec  = sec;
	t->nsec = nsec;
	return t;
}


DECL struct cat_time *tm_mark(struct cat_time *old, struct cat_time *tnew)
{
	struct cat_time hold;

	abort_unless(tnew);
	abort_unless(old);

	hold = *tnew;
	if ( tm_sub(tnew, old) == NULL )
		return NULL;
	*old = hold;
	
	return tnew;
}


#if CAT_USE_STDLIB

DECL struct cat_time *tm_cget(struct cat_time *t)
{
	clock_t cur;

	abort_unless(t);

	cur = clock();
	t->sec  = cur / CLOCKS_PER_SEC;
	t->nsec = (long)((double)(cur - t->sec) / 
			(double)CLOCKS_PER_SEC * 1000000000L);
	return t;
}

#endif /* CAT_USE_STDLIB */


#endif /* defined(CAT_TIME_DO_DECL) && CAT_TIME_DO_DECL */


#undef DECL
#undef LDECL
#undef CAT_TIME_DO_DECL

#endif /* __cat_time_h */
