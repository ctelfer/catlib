/*
 * cat/cat.h -- Info needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007, See accompanying license
 *
 */

#ifndef __cat_cat_h
#define __cat_cat_h

#include <cat/config.h>

#ifndef CAT_USE_STDLIB
#define CAT_USE_STDLIB		1
#endif

#ifndef CAT_HAS_POSIX
#define CAT_HAS_POSIX		1
#endif

#ifndef CAT_ANSI89
#define CAT_ANSI89		0
#endif /* CAT_ANSI89 */

#ifndef CAT_USE_INLINE
#define CAT_USE_INLINE		1
#endif

#ifndef CAT_HAS_LONGLONG
#define CAT_HAS_LONGLONG	1
#endif

#ifndef CAT_64BIT
#define CAT_64BIT		0
#endif

#ifndef CAT_DIE_DUMP
#define CAT_DIE_DUMP		0
#endif 

#ifndef CAT_DEBUG_LEVEL
#define CAT_DEBUG_LEVEL		0
#endif

/* CAT_ALIGN is a union representing the most restrictive alignment type */
#ifndef CAT_ALIGN
typedef union { 
	long			l;
	void *			vp;
	char *			cp;
	double 			d;
#if CAT_HAS_LONGLONG
	long long		ll;
#endif /* CAT_HAS_LONGLONG */
} cat_align_t;
#define CAT_ALIGN	cat_align_t
#else /* CAT_ALIGN */
typedef CAT_ALIGN	cat_align_t;
#endif /* CAT_ALIGN */


/* ASSUMPTION: 
 * all types in cat_align_t are worst case alignments: (e.g. ll is on 8-byte
 * boundaries).  This does not generally hold but is safer than guessing
 * incorrectly.  If a programmer knows * specifically otherwise the coder can 
 * #define CAT_ALIGN.
 */
#define CAT_ALIGN_SIZE(x) \
	(((x)+(sizeof(cat_align_t)-1))/sizeof(cat_align_t)*sizeof(cat_align_t))
#define CAT_DECLARE_ALIGNED_DATA(name, len) \
	CAT_DECLARE_ALIGNED_DATA_Q(,name,len)
/* qual - (e.g. static volatile), name - variable name, len - in bytes */
#define CAT_DECLARE_ALIGNED_DATA_Q(qual, name, len) \
	qual CAT_ALIGN name[CAT_ALIGN_SIZE(len) / sizeof(CAT_ALIGN)]

typedef unsigned char byte_t;
typedef signed char schar;

#ifndef CAT_NEED_UTYPEDEFS
#define CAT_NEED_UTYPEDEFS 1
#endif /* CAT_NEED_UTYPEDEFS */

#if CAT_NEED_UTYPEDEFS
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#if CAT_HAS_LONGLONG
typedef unsigned long long ulonglong;
#endif /* CAT_HAS_LONGLONG */
#endif /* CAT_NEED_UTYPEDEFS */

#if CAT_USE_STDLIB

#define CAT_USE_SYS_STDDEF
#ifndef CAT_HAS_FIXED_WIDTH
#define CAT_HAS_FIXED_WIDTH 1
#define CAT_USE_STDINT_TYPES 1
#endif /* CAT_HAS_FIXED_WIDTH */

#else /* CAT_USE_STDLIB */

#ifndef CAT_HAS_FIXED_WIDTH
#define CAT_HAS_FIXED_WIDTH 1
#endif /* CAT_HAS_FIXED_WIDTH */

#endif /* CAT_USE_STDLIB */

#include <stddef.h>


/* We redefined this here because we might be including a c89 stddef.h */
#ifndef offsetof
#define offsetof(type, field) ((ulong)&((type *)0)->field)
#endif
#define container(ptr, type, field) \
	((type *)((char*)(ptr)-offsetof(type,field)))
#define array_length(arr) (sizeof(arr) / sizeof(arr[0]))

#define ptr2int(p)	((int)(ptrdiff_t)(p))
#define ptr2uint(p)	((uint)(ptrdiff_t)(p))
#define int2ptr(i)	((void *)(ptrdiff_t)(i))

/* generic binary data container */
struct raw {
	size_t		len;
	byte_t *	data;
} ;

union attrib_u {
	void *		au_pointer;
	long double	au_dblval;
	int		au_intval;
	uint		au_uintval;
	uchar		au_data[1];
	cat_align_t	au_align;
};

/* Generic scalar value union */
union scalar_u {
	int		int_val;
	unsigned int	uint_val;
	double		dbl_val;
	void *		ptr_val;
	char *		str_val;

#if CAT_HAS_LONG_LONG
	long long		llong_val;
	unsigned long long	ullong_val;
#endif /* CAT_HAS_LONG_LONG */
	long		long_val;
	unsigned long	ulong_val;
	short		short_val;
	unsigned short	ushort_val;
	signed char	sch_val;
	unsigned char	uch_val;
	long double	ldbl_val;
	float		float_val;
};

typedef union scalar_u scalar_t;

/* Special function types */
typedef int  (*cmp_f)(const void *v1, const void *v2);
typedef void (*apply_f)(void * data, void * ctx); 
typedef int (*copy_f)(void *dst, void *src, void *ctx);

extern void cat_abort(const char *fn, unsigned ln, const char *expr);
#define abort_unless(x) \
	do { if (!(x)) { cat_abort(__FILE__, __LINE__, #x); } } while (0)


#define CT_ERROR_IF(__name, __test) \
static char __ct_##__name[(__test) ? -1 : 1];

#endif /* __cat_cat_h */
