/*
 * aux.c -- funtions needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2005 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/aux.h>

#if CAT_USE_STDLIB
#include <stdlib.h>
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


int cmp_ptr(void *k1, void *k2)
{
	return ((char *)k1 - (char *)k2);
}


int cmp_str(void *k1, void *k2)
{
	return strcmp(k1, k2);
}


int cmp_raw(void *k1p, void *k2p)
{
	struct raw *k1 = k1p, *k2 = k2p;

	if ( k1->len > k2->len )
		return -1;
	else if ( k1->len < k2->len )
		return 1;
	else 
		return memcmp(k1->data, k2->data, k1->len);
}
