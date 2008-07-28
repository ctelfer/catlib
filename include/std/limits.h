#ifndef __catlimits_h
#define __catlimits_h

#if !CAT_USE_STDLIB
/* 
 * These are best guesses that you can substitute on your own machine if 
 * need be.
 */

#define CHAR_BIT	8
#define UCHAR_MAX	((unsigned char)~0)
#define UINT_MAX	((unsigned int)~0)
#define USHRT_MAX	((unsigned short)~0)
#define ULONG_MAX	((unsigned long)~0)
#define SCHAR_MAX	((signed char)(UCHAR_MAX >> 1))
#define SCHAR_MIN	((signed char)-SCHAR_MAX)
#define SHRT_MAX	((short)(USHRT_MAX >> 1))
#define SHRT_MIN	((short)-SHRT_MAX)
#define INT_MAX		((int)(UINT_MAX >> 1))
#define INT_MIN		((int)-INT_MAX)
#define LONG_MAX	((long)(ULONG_MAX >> 1))
#define LONG_MIN	((long)-LONG_MAX)

#endif /* !CAT_USE_STDLIB */

#endif /* __catlimits_h */
