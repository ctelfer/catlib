#ifndef __catstddef_h
#define __catstddef_h

#if !CAT_USE_STDLIB
/* 
 * These are best guesses that you can substitute on your own machine if 
 * need be.
 */
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef unsigned short wchar_t;
#define offsetof(type, field) ((unsigned long)&((type *)0)->field)
#define NULL ((void *)0)

#endif /* !CAT_USE_STDLIB */

#endif /* __catstddef_h */
