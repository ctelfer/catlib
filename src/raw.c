/*
 * raw.c -- raw data blob manipulation functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2004, See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/raw.h>


int raw_cmp(void *r1p, void *r2p)
{
	int rv = 0;
	size_t len;
	struct raw *r1 = r1p, *r2 = r2p;
	unsigned char *p1, *p2;

	abort_unless(r1 && r1->data);
	abort_unless(r2 && r2->data);

	if ( r1 == r2 )
		return 0;
	p1 = (unsigned char *)r1->data;
	p2 = (unsigned char *)r2->data;

	len = r1->len < r2->len ? r1->len : r2->len;
	while ( len > 0 && *p1 == *p2 ) {
		++p1;
		++p2;
		--len;
	}

	if ( len > 0 ) {
		rv = *p1 - *p2;
		if ( rv < 0 )
			rv = -1;
		else if ( rv > 0 )
			rv =1;
	}

	if ( (rv == 0) && (r1->len != r2->len) )
		return r1->len < r2->len ? -1 : 1;
	else
		return rv;
}


struct raw *str_to_raw(struct raw *r, char *s, int terminate)
{

	unsigned char *t = (unsigned char *)s;
	unsigned char *orig = t;
	abort_unless(r && s);
	r->data = s;
	while ( *t != '\0' )
		++t;
	r->len = t - orig;
	if ( terminate )
		r->len += 1;
	return r;
}
