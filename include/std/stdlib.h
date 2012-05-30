/*
 * stdlib.h -- Standard C library routines.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_stdlib_h
#define __cat_stdlib_h

#include <cat/cat.h>
#include <stdarg.h>

/* We use the mem* functions even if we don't use the standard library */
#if !CAT_USE_STDLIB

long strtol(const char *start, char **cp, int base);
ulong strtoul(const char *start, char **cp, int base);
double strtod(const char *start, char **cp);

void *malloc(size_t len);
void *calloc(size_t nmem, size_t ilen);
void *realloc(void *omem, size_t len);
void free(void *mem);


/* TODO: in the near future
 * errno
 */

void exit(int status);
void abort(void);

#endif /* !CAT_USE_STDLIB */

#endif /* __cat_stdlib_h */
