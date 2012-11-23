/*
 * cat/time.h -- Functions for timing events
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
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


struct cat_time_s {
	long	sec;
	long	nsec;
};
typedef struct cat_time_s cat_time_t;


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
static cat_time_t tm_zero = { 0, 0 };
#else
extern cat_time_t tm_zero;
#endif

DECL cat_time_t tm_add(cat_time_t t1, cat_time_t t2);
DECL cat_time_t tm_sub(cat_time_t t1, cat_time_t t2);

DECL int        tm_cmp(cat_time_t t1, cat_time_t t2);
DECL int	tm_eqz(cat_time_t t);
DECL int	tm_ltz(cat_time_t t);
DECL int	tm_gtz(cat_time_t t);
DECL int	tm_lez(cat_time_t t);
DECL int	tm_gez(cat_time_t t);
DECL int        tm_cmpf(void *t1, void *t2);
DECL double     tm_2dbl(cat_time_t t);
DECL long	tm_sec(cat_time_t t);
DECL long	tm_nsec(cat_time_t t);
DECL cat_time_t tm_lset(long sec, long nsec);
DECL cat_time_t tm_dset(double d);

/* start = end ; returns  end - (original) start */
DECL cat_time_t tm_mark(cat_time_t *start, cat_time_t end);

#if CAT_USE_STDLIB
#include <time.h>
DECL cat_time_t tm_cget();

#if CAT_HAS_POSIX
#include <sys/time.h>
DECL cat_time_t tm_uget();
#endif /* CAT_HAS_POSIX */

#endif /* CAT_USE_STDLIB */


#if defined(CAT_TIME_DO_DECL) && CAT_TIME_DO_DECL

#if !defined(CAT_USE_INLINE) || CAT_USE_INLINE == 0
cat_time_t tm_zero = { 0, 0 };
#endif

LDECL void tm_normalize(cat_time_t *t)
{
	abort_unless(t);

	while ( t->nsec > 1000000000L || (t->sec < 0 && t->nsec > 0) ) {
		t->nsec -= 1000000000L;
		++t->sec;
	}

	while ( t->nsec < -1000000000L || (t->sec > 0 && t->nsec < 0) ) {
		t->nsec += 1000000000L;
		--t->sec;
	}
}


DECL cat_time_t tm_add(cat_time_t t1, cat_time_t t2)
{
	tm_normalize(&t1);
	tm_normalize(&t2);

	t1.sec  += t2.sec;
	t1.nsec += t2.nsec;
	tm_normalize(&t2);
	return t1;
}


DECL cat_time_t tm_sub(cat_time_t t1, cat_time_t t2)
{
	tm_normalize(&t1);
	tm_normalize(&t2);

	t1.sec  -= t2.sec;
	t1.nsec -= t2.nsec;
	tm_normalize(&t1);
	return t1;
}


DECL int tm_eqz(cat_time_t t)
{
	return t.sec == 0 && t.nsec == 0;
}


DECL int tm_ltz(cat_time_t t)
{
	tm_normalize(&t);
	return t.sec < 0 || t.nsec < 0;
}


DECL int tm_gtz(cat_time_t t)
{
	tm_normalize(&t);
	return t.sec > 0 || t.nsec > 0;
}


DECL int tm_lez(cat_time_t t)
{
	return !tm_gtz(t);
}


DECL int tm_gez(cat_time_t t)
{
	return !tm_ltz(t);
}


DECL int tm_cmp(cat_time_t t1, cat_time_t t2)
{
	long d;
	tm_normalize(&t1);
	tm_normalize(&t2);

	if ( (d = t1.sec - t2.sec) == 0 )
		d = t1.nsec - t2.nsec;

	return (d < 0) ? -1 : ((d > 0) ? 1 : 0) ;
}


DECL int tm_cmpf(void *t1p, void *t2p)
{
	abort_unless(t1p);
	abort_unless(t2p);

	return tm_cmp(*(cat_time_t *)t1p, *(cat_time_t *)t2p);
}


DECL double tm_2dbl(cat_time_t t)
{
	return t.sec + (double)t.nsec / (double)1000000000;
}


DECL long tm_sec(cat_time_t t)
{
	tm_normalize(&t);
	return t.sec;
}


DECL long tm_nsec(cat_time_t t)
{
	tm_normalize(&t);
	return t.nsec;
}


DECL cat_time_t tm_dset(double d)
{
	cat_time_t t;

	t.sec  = (long)d;
	t.nsec = (long)((d - t.sec) * 1000000000L);
	return t;
}


DECL cat_time_t tm_lset(long sec, long nsec)
{
	cat_time_t t;

	t.sec  = sec;
	t.nsec = nsec;
	tm_normalize(&t);
	return t;
}


DECL cat_time_t tm_mark(cat_time_t *old, cat_time_t tnew)
{
	cat_time_t hold;

	abort_unless(old);

	hold = tm_sub(tnew, *old);
	*old = tnew;

	return hold;
}


#if CAT_USE_STDLIB

DECL cat_time_t tm_cget()
{
	cat_time_t t;
	clock_t cur;

	cur = clock();
	t.sec  = cur / CLOCKS_PER_SEC;
	t.nsec = (long)((double)(cur - t.sec) / 
			(double)CLOCKS_PER_SEC * 1000000000L);
	return t;
}

#if CAT_HAS_POSIX

/* Get Posix system time */


DECL cat_time_t tm_uget()
{
	cat_time_t t;
	struct timeval cur;

	gettimeofday(&cur, NULL);
	t.sec  = cur.tv_sec;
	t.nsec = cur.tv_usec * 1000;

	return t;
}


#endif /* CAT_HAS_POSIX */

#endif /* CAT_USE_STDLIB */


#endif /* defined(CAT_TIME_DO_DECL) && CAT_TIME_DO_DECL */


#undef DECL
#undef LDECL
#undef CAT_TIME_DO_DECL

#endif /* __cat_time_h */
