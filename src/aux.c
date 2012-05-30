/*
 * aux.c -- funtions needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
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
