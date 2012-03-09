/*
 * cat/bbuf.h -- A generic resizeable byte buffer;
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


struct bbuf {
	size_t		size;
	byte_t *	data;
	size_t		off;
	size_t		len;
	struct memmgr *	mm;
};


/* all of the below functions will return a -1 if the offset/length */
/* parameters will cause an overflow of size_t */


/* initialize a fresh bbuf */
void bb_init(struct bbuf *b, struct memmgr *mm);

/* ensure that the buffer has at least sz bytes */
/* if not, try to allocate it.  Return 0 on success and */
/* -1 if unable to allocate the space. */
int  bb_alloc(struct bbuf *b, size_t sz);

/* release the memory associated with a bbuf */
void bb_free(struct bbuf *b);

/* concatenate 'len' bytes from 'p' to 'b'.  Return 0 on success or */
/* -1 if not enough space.  */
int  bb_cat(struct bbuf *b, void *p, size_t len);

/* concatenate 'len' bytes from 'p' to 'b'.  If 'b' lacks space, try */
/* to allocate it.  Return 0 on success and -1 on failure. */
int  bb_cat_a(struct bbuf *b, void *p, size_t len);

/* Set the data in the buffer to 'len' bytes of data starting at 'off' */
/* bytes from the start of the bbuf.  Copy the data from 'p'.  Returns 0 */
/* on success, and -1 if not enough space.  */
int  bb_set(struct bbuf *b, size_t off, void *p, size_t len);

/* Set the data in the buffer to 'len' bytes of data starting at 'off' */
/* bytes from the start of the bbuf.  Copy the data from 'p'.  If 'b' lacks */
/* the space, try to allocate it.  Return 0 on success and -1 on failure */
int  bb_set_a(struct bbuf *b, size_t off, void *p, size_t len);

/* copy the 'sb' into 'db'.  Returns 0 on success, or -1 if unable to */
/* allocate the necessary space in 'db'.  Offsets are preserved and */
/* the 'db' is sized to at least 'sb->size' */
int  bb_copy(struct bbuf *db, struct bbuf *sb);

#endif /* __cat_bbuf_h */
