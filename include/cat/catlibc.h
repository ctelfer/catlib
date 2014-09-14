/*
 * catlibc.h -- local implementation of standard libc functions.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2014 See accompanying license
 *
 */

#ifndef __cat_catlibc_h
#define __cat_catlibc_h

#include <cat/cat.h>


struct catlibc_cfg {
	ulong *heap_base;
	ulong heap_sz;
	char *stdin_buf;
	ulong stdin_bufsz;
	char *stdout_buf;
	ulong stdout_bufsz;
	char *stderr_buf;
	ulong stderr_bufsz;
};


void catlibc_reset(struct catlibc_cfg *cprm);




#endif /* __cat_catlibc_h */
