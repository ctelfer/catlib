/*
 * ctype.h -- C standard ctype.h library.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_ctype_h
#define __cat_ctype_h

#include <cat/cat.h>
#include <stdarg.h>

/* We use the mem* functions even if we don't use the standard library */
#if !CAT_USE_STDLIB

/* ctype.h */
int isalnum(int c);
int isdigit(int c);
int isxdigit(int c);
int isspace(int c);
int isprint(int c);
int isalpha(int c);
int islower(int c);
int isupper(int c);
int toupper(int c);
int tolower(int c);

#endif /* !CAT_USE_STDLIB */

#endif /* __cat_ctype_h */
