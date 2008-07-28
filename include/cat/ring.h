/*
 * cat/ring.h -- Ring Buffer
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004 See accompanying license
 *
 */

#ifndef __cat_ring_h
#define __cat_ring_h

#include <cat/cat.h>

/* N.B. First two fields match struct raw */
struct ring {
	size_t			alloc;
	unsigned char *		data;
	size_t			start;
	size_t			len;
} ;


#define ring_avail(r)	((r)->alloc - (r)->len)
void   ring_init(struct ring *r, void *data, size_t len);
void   ring_reset(struct ring *r);
size_t ring_put(struct ring *r, char *in, size_t len, int overwrite);
size_t ring_get(struct ring *r, char *out, size_t len);
size_t ring_last(struct ring *r);

#endif /* __cat_ring_h */
