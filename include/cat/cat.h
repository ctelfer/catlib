/*
 * cat/cat.h -- Info needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007-2015 -- See accompanying license
 *
 */

#ifndef __cat_cat_h
#define __cat_cat_h

#include <cat/config.h>

#ifndef CAT_USE_STDLIB
#define CAT_USE_STDLIB		1
#endif /* CAT_USE_STDLIB */

#ifndef CAT_HAS_POSIX
#define CAT_HAS_POSIX		1
#endif /* CAT_HAS_POSIX */

#ifndef CAT_ANSI89
#define CAT_ANSI89		0
#endif /* CAT_ANSI89 */

#ifndef CAT_USE_INLINE
#define CAT_USE_INLINE		1
#endif /* CAT_USE_INLINE */

#ifndef CAT_HAS_LONGLONG
#define CAT_HAS_LONGLONG	0
#endif /* CAT_HAS_LONGLONG */

#ifndef CAT_HAS_DIV
#define CAT_HAS_DIV		1
#endif /* CAT_HAS_DIV */

#ifndef CAT_HAS_FLOAT
#define CAT_HAS_FLOAT		1
#endif /* CAT_HAS_FLOAT */

#ifndef CAT_64BIT
#define CAT_64BIT		0
#endif /* CAT_64BIT */

#ifndef CAT_DIE_DUMP
#define CAT_DIE_DUMP		0
#endif /* CAT_DIE_DUMP */

#ifndef CAT_DEBUG_LEVEL
#define CAT_DEBUG_LEVEL		0
#endif /* CAT_DEBUG_LEVEL */

#if CAT_USE_STDLIB
#define CAT_USE_STDINT_TYPES	1
#endif /* CAT_USE_STDLIB */


/* Compile time assertions */
#define __STATIC_BUGNAME(name, line) __bug_on_##name##_##line
#define STATIC_BUGNAME(name, line) __STATIC_BUGNAME(name, line)
#define STATIC_BUG_ON(name, test) \
	enum { STATIC_BUGNAME(name, __LINE__) = 1 / !(test) };

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
#if CAT_HAS_LONGLONG
typedef long long llong;
#endif /* CAT_HAS_LONGLONG */

#ifndef CAT_NEED_UTYPEDEFS
#define CAT_NEED_UTYPEDEFS 1
#endif /* CAT_NEED_UTYPEDEFS */

#if CAT_NEED_UTYPEDEFS
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#if CAT_HAS_LONGLONG
typedef unsigned long long ullong;
#endif /* CAT_HAS_LONGLONG */
#endif /* CAT_NEED_UTYPEDEFS */

#include <stddef.h>

#ifdef CAT_USE_STDINT_TYPES
#include <stdint.h>
#elif CAT_USE_POSIX_TYPES
#include <sys/types.h>
#else /* no defined headers: take guess */

/* Reasonable guesses for a 8, 16, 32 or some 64 bit architectures. */
/* While these will always be at least wide enough, they may be too wide. */

/* Allow overrides during compilation */
#ifndef CAT_S8_T
#define CAT_S8_T signed char
#endif /* CAT_S8_T */
#ifndef CAT_U8_T
#define CAT_U8_T unsigned char
#endif /* CAT_U8_T */
#ifndef CAT_S16_T
#define CAT_S16_T short
#endif /* CAT_S16_T */
#ifndef CAT_U16_T
#define CAT_U16_T unsigned short
#endif /* CAT_U16_T */
#ifndef CAT_S32_T
#define CAT_S32_T long
#endif /* CAT_S32_T */
#ifndef CAT_U32_T
#define CAT_U32_T unsigned long
#endif /* CAT_U32_T */

/* 
 * Define 64-bit types --
 *  If CAT_64BIT is true, that means our compiler supports some 64-bit type.
 *  It _could_ be "long" or even "char".  If CAT_HAS_LONGLONG is set, then
 *  it is safe to set the 64-bit type to 'long long'.  But if this isn't
 *  set then set it to 'long'.  The problem being that ANSI 89 compilers
 *  don't need to support 'long long', but do need to support 'long'.
 */ 
#if CAT_64BIT
#if CAT_HAS_LONGLONG

#ifndef CAT_U64_T
#define CAT_U64_T unsigned long long
#endif /* CAT_U64_T */
#ifndef CAT_S64_T
#define CAT_S64_T long long
#endif /* CAT_S64_T */

#else /* CAT_HAS_LONGLONG */

#ifndef CAT_U64_T
#define CAT_U64_T unsigned long
#endif /* CAT_U64_T */
#ifndef CAT_S64_T
#define CAT_S64_T long
#endif /* CAT_S64_T */

#endif /* CAT_HAS_LONGLONG */
#endif /* CAT_64BIT */


typedef CAT_S8_T	int8_t;
typedef CAT_U8_T	uint8_t;
typedef CAT_S16_T	int16_t;
typedef CAT_U16_T	uint16_t;
typedef CAT_S32_T	int32_t;
typedef CAT_U32_T	uint32_t;
#if CAT_64BIT
typedef CAT_S64_T	int64_t;
typedef CAT_U64_T	uint64_t;
#endif /* CAT_64BIT */

#endif /* no defined headers: take guess */

#if !CAT_HAS_INTPTR_T || (!CAT_USE_STDINT_TYPES && !CAT_USE_POSIX_TYPES)
#if CAT_64BIT
typedef CAT_S64_T intptr_t;
#else /* CAT_64BIT */
typedef CAT_S32_T intptr_t;
#endif /* CAT_64BIT */
#endif /* !CAT_HAS_INTPTR_T */

#if !CAT_HAS_UINTPTR_T || (!CAT_USE_STDINT_TYPES && !CAT_USE_POSIX_TYPES)
#if CAT_64BIT
typedef CAT_U64_T uintptr_t;
#else /* CAT_64BIT */
typedef CAT_U32_T uintptr_t;
#endif /* CAT_64BIT */
#endif /* !CAT_HAS_UINTPTR_T */

STATIC_BUG_ON(int8_t_size_not_equal_to_1, sizeof(int8_t) != 1)
STATIC_BUG_ON(uint8_t_size_not_equal_to_1, sizeof(uint8_t) != 1)
STATIC_BUG_ON(int16_t_size_not_equal_to_2, sizeof(int16_t) != 2)
STATIC_BUG_ON(uint16_t_size_not_equal_to_2, sizeof(uint16_t) != 2)
STATIC_BUG_ON(int32_t_size_not_equal_to_4, sizeof(int32_t) != 4)
STATIC_BUG_ON(uint32_t_size_not_equal_to_4, sizeof(uint32_t) != 4)
#if CAT_64BIT
STATIC_BUG_ON(int64_t_size_not_equal_to_8, sizeof(int64_t) != 8)
STATIC_BUG_ON(uint64_t_size_not_equal_to_8, sizeof(uint64_t) != 8)
#endif /* CAT_64BIT */
STATIC_BUG_ON(intptr_t_lt_sizeof_voidptr, sizeof(intptr_t) < sizeof(void *))
STATIC_BUG_ON(uintptr_t_lt_sizeof_voidptr, sizeof(uintptr_t) < sizeof(void *))


/* We redefined this here because we might be including a c89 stddef.h */
#ifndef offsetof
#define offsetof(type, field) ((ulong)&((type *)0)->field)
#endif
#define container(ptr, type, field) \
	((type *)((char*)(ptr)-offsetof(type,field)))
#define array_length(arr) (sizeof(arr) / sizeof(arr[0]))

#define ptr2int(p)	((intptr_t)(void *)(p))
#define ptr2uint(p)	((uintptr_t)(void *)(p))
#define int2ptr(i)	((void *)(uintptr_t)(i))

/* generic binary data container */
struct raw {
	size_t		len;
	byte_t *	data;
} ;

/* Generic scalar value union */
union attrib_u {
	int		int_val;
	unsigned int	uint_val;
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
	double		dbl_val;
	float		float_val;
	byte_t		bytes[1];
	cat_align_t	align[1];
};

typedef union attrib_u attrib_t;

#define attrib_csize(type, afld, asize) \
	(offsetof(type, afld) + \
	 ((((asize) <= 1) ? 1 : (asize)) + sizeof(attrib_t) - 1) \
	  / sizeof(attrib_t) * sizeof(attrib_t))

/* Special function types */
typedef int  (*cmp_f)(const void *v1, const void *v2);
typedef void (*apply_f)(void *data, void *ctx); 
typedef int (*copy_f)(void *dst, void *src, void *ctx);

extern void cat_abort(const char *fn, unsigned ln, const char *expr);
#define abort_unless(x) \
	do { if (!(x)) { cat_abort(__FILE__, __LINE__, #x); } } while (0)


#endif /* __cat_cat_h */
