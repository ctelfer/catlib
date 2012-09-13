/*
 * inport.c -- generic input API.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_inport_h
#define __cat_inport_h
#include <cat/cat.h>

struct inport;

/* len must be >= 0 or returns error */
/* returns: -1 on error, 0 on EOF, > 0 == # of bytes */
typedef int (*inp_read_f)(struct inport *in, void *buf, int len);
typedef int (*inp_close_f)(struct inport *in);

struct inport {
	inp_read_f		read;
	inp_close_f		close;
};


/* len must be less than INT_MAX or returns error */
/* returns: -1 on err, 0 on EOF, > 0 == # of bytes */
int inp_read(struct inport *in, void *buf, int len);

/* returns -1 on both end of file or error, otherwise char */
int inp_getc(struct inport *in);

/* no return */
int inp_close(struct inport *in);


struct cstr_inport {
	struct inport		in;
	const char *		str;
	size_t			slen;
	size_t			off;
};

void csinp_init(struct cstr_inport *csi, const char *s);
void csinp_reset(struct cstr_inport *csi);
void csinp_clear(struct cstr_inport *csi);


void ninp_init(struct inport *in);
extern struct inport null_inport;

#endif /* __cat_inport_h */
