/*
 * cat.c -- generic universal catlib functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/cat.h>

#include <stdio.h>
#include <stdlib.h>

void cat_abort(const char *fn, unsigned ln, const char *expr)
{
	fprintf(stderr, "%s:%u -- check failed: %s\n", fn, ln, expr);
	abort();
	exit(255);	/* in case the signal is caught */
}

