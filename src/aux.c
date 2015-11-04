/*
 * aux.c -- funtions needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2014 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/aux.h>

#include <stdlib.h>
#include <string.h>


int cmp_ptr(const void *k1, const void *k2)
{
	return ((char *)k1 - (char *)k2);
}


int cmp_str(const void *k1, const void *k2)
{
	return strcmp(k1, k2);
}


int cmp_raw(const void *k1p, const void *k2p)
{
	const struct raw *k1 = k1p;
	const struct raw *k2 = k2p;

	if ( k1->len > k2->len )
		return -1;
	else if ( k1->len < k2->len )
		return 1;
	else 
		return memcmp(k1->data, k2->data, k1->len);
}


int cmp_intptr(const void *k1, const void *k2)
{
	return ptr2int(k1) - ptr2int(k2);
}


int cmp_uintptr(const void *k1, const void *k2)
{
	uintptr_t v1 = ptr2uint(k1);
	uintptr_t v2 = ptr2uint(k2);
	if ( v1 < v2 )
		return -1;
	else if ( v1 == v2 )
		return 0;
	else
		return 1;
}


ulong uldivmod(ulong dend, ulong dsor, int div)
{
	ulong r = 0;
	int i;

	if ( dsor == 0 )
		return 0xFFFFFFFFul;

	for ( i = 0; i < 32; ++i ) {
		r = (r << 1) | (dend >> 31);
		dend = dend << 1;
		if ( r >= dsor ) {
			r -= dsor;
			dend += 1;
		}
	}

	return div ? dend : r;
}


#if CAT_HAS_LONGLONG
ullong ulldivmod(ullong dend, ullong dsor, int div)
{
	ullong r = 0;
	int i;

	if ( dsor == 0 )
		return 0xFFFFFFFFFFFFFFFFull;

	for ( i = 0; i < 64; ++i ) {
		r = (r << 1) | (dend >> 63);
		dend = dend << 1;
		if ( r >= dsor ) {
			r -= dsor;
			dend += 1;
		}
	}

	return div ? dend : r;
}
#endif /* CAT_HAS_LONGLONG */
