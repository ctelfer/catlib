/*
 * cat/dynbuf.h -- A generic resizeable byte buffer;
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2012, See accompanying license
 *
 */

#ifndef __cat_buffer_h
#define __cat_buffer_h

#include <cat/cat.h>
#include <cat/mem.h>


struct dynbuf {
	ulong		size;
	byte_t *	data;
	ulong		off;
	ulong		len;
	struct memmgr *	mm;
};


#define dyb_last(b) ((b)->off + (b)->len)


/* all of the below functions will return a -1 if the offset/length */
/* parameters will cause an overflow of ulong */


/* initialize a fresh dynbuf */
void dyb_init(struct dynbuf *b, struct memmgr *mm);

/* ensure that the buffer has at least sz bytes */
/* if not, try to allocate it.  Return 0 on success and */
/* -1 if unable to allocate the space. */
int dyb_alloc(struct dynbuf *b, ulong sz);

/* release the memory associated with a dynbuf */
void dyb_free(struct dynbuf *b);

/* concatenate 'len' bytes from 'p' to 'b'.  Return 0 on success or */
/* -1 if not enough space.  */
int dyb_cat(struct dynbuf *b, void *p, ulong len);

/* concatenate 'len' bytes from 'p' to 'b'.  If 'b' lacks space, try */
/* to allocate it.  Return 0 on success and -1 on failure. */
int dyb_cat_a(struct dynbuf *b, void *p, ulong len);

/* Set the data in the buffer to 'len' bytes of data starting at 'off' */
/* bytes from the start of the dynbuf.  Copy the data from 'p'.  Returns 0 */
/* on success, and -1 if not enough space.  */
int dyb_set(struct dynbuf *b, ulong off, void *p, ulong len);

/* Set the data in the buffer to 'len' bytes of data starting at 'off' */
/* bytes from the start of the dynbuf.  Copy the data from 'p'.  If 'b' lacks */
/* the space, try to allocate it.  Return 0 on success and -1 on failure */
int dyb_set_a(struct dynbuf *b, ulong off, void *p, ulong len);

/* copy the 'sb' into 'db'.  Returns 0 on success, or -1 if unable to */
/* allocate the necessary space in 'db'.  Offsets are preserved and */
/* the 'db' is sized to at least 'sb->size' */
int dyb_copy(struct dynbuf *db, struct dynbuf *sb);

#endif /* __cat_dynbuf_h */
