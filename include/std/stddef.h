#ifndef __catstddef_h
#define __catstddef_h

/* 
 * These are best guesses that you can substitute on your own machine if 
 * need be.
 */
typedef ulong size_t;
typedef long ptrdiff_t;
typedef ushort wchar_t;
#define offsetof(type, field) ((ulong)&((type *)0)->field)
#define NULL ((void *)0)

#endif /* __catstddef_h */
