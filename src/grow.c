/*
 * grow.c -- funtions to grow dynamically allocated memory
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/aux.h>
#include <cat/mem.h>
#include <cat/grow.h>
#include <stdlib.h>

/* 
 * Rules: 
 *  - min must be less that CAT_MAXGROW
 *  - len and ptr must be nonnull
 *  - *ptr may be null if and only if len == 0
 *  - if min == 0 then it the buffer double in size or be set to CAT_MINGROW
 *    if len == 0
 *  - the new length will be a power of 2 multiple of the old length or 
 *    CAT_MAXGROW if doubling the length would put it above CAT_MAXGROW
 */


int grow(byte_t **ptr, size_t *lenp, size_t min)
{
	void *p2 = *ptr;
	int rv;
	rv = agrow(&p2, 1, lenp, min);
	*ptr = p2;
	return rv;
}


int agrow(void **ptr, size_t ilen, size_t *lenp, size_t min)
{
	size_t len, n, newlen;
	byte_t *p;
	size_t maxmem = (size_t)~0;

	abort_unless(ptr);
	abort_unless(ilen > 0);
	abort_unless(lenp);

#if CAT_HAS_DIV
	maxmem /= ilen;
#else
	maxmem = uldivmod(maxmem, ilen, 1);
#endif
	len = *lenp;
	abort_unless((len > 0) || (*ptr == NULL));
	abort_unless(len <= maxmem);

	if ( min > maxmem )
		return -1;

	/* When min == 0 && len == maxmem fall through to return 0 */
	if ( (min == 0) && (len < maxmem) )
		min = len + 1;

	if ( min <= len )
		return 0;

	if ( len > 0 ) {
		newlen = len;
	} else {
		newlen = CAT_MINGROW / ilen;
		if ( newlen == 0 )
			newlen = 1;
	}

	while ( newlen < min ) { 
		n = newlen << 1;
		if ( (n < newlen) || (n > maxmem) )
			newlen = maxmem;
		else
			newlen = n;
	} 

	p = realloc(*ptr, newlen * ilen);
	if ( !p )
		return -1;

	*ptr = p;
	*lenp = newlen;

	return 0;
}


int mm_grow(struct memmgr *mm, byte_t **ptr, size_t *lenp, size_t min)
{
	void *p2 = ptr;
	int rv;
	rv = mm_agrow(mm, &p2, 1, lenp, min);
	*ptr = p2;
	return rv;
}


int mm_agrow(struct memmgr *mm, void **ptr, size_t ilen, size_t *lenp, 
	     size_t min)
{
	size_t len, n, newlen;
	byte_t *p;
	size_t maxmem = (size_t)~0;

	abort_unless(ptr);
	abort_unless(ilen > 0);
	abort_unless(lenp);

#if CAT_HAS_DIV
	maxmem /= ilen;
#else
	maxmem = uldivmod(maxmem, ilen, 1);
#endif
	len = *lenp;
	abort_unless((len > 0) || (*ptr == NULL));
	abort_unless(len <= maxmem);

	if ( min > maxmem )
		return -1;

	/* When min == 0 && len == maxmem fall through to return 0 */
	if ( (min == 0) && (len < maxmem) )
		min = len + 1;

	if ( min <= len )
		return 0;

	if ( len > 0 ) {
		newlen = len;
	} else {
		newlen = CAT_MINGROW / ilen;
		if ( newlen == 0 )
			newlen = 1;
	}

	while ( newlen < min ) { 
		n = newlen << 1;
		if ( (n < newlen) || (n > maxmem) )
			newlen = maxmem;
		else
			newlen = n;
	} 

	p = mem_resize(mm, *ptr, newlen);
	if ( !p )
		return -1;

	*ptr = p;
	*lenp = newlen;

	return 0;
}


