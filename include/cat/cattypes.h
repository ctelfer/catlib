/*
 * cattypes.h -- type definitions that work even when stdint.h isn't present.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_cattypes_h
#define __cat_cattypes_h

#include <cat/cat.h>

#ifdef CAT_HAS_FIXED_WIDTH 

#ifdef CAT_USE_STDINT_TYPES
#include <stdint.h>
#elif defined(CAT_USE_POSIX_TYPES)
#include <sys/types.h>
#else /* no defined headers: take guess */

/* reasonable guesses for a 8, 16, 32 or some 64 bit architectures */
/* While these will always be at least wide enough, they may be too wide. */

/* Allow overrides during compilation */
#ifndef CAT_U8_T
#define CAT_U8_T unsigned char
#endif /* CAT_U8_T */
#ifndef CAT_S8_T
#define CAT_S8_T char
#endif /* CAT_S8_T */
#ifndef CAT_U16_T
#define CAT_U16_T unsigned short
#endif /* CAT_U16_T */
#ifndef CAT_S16_T
#define CAT_S16_T short
#endif /* CAT_S16_T */
#ifndef CAT_U32_T
#define CAT_U32_T unsigned long
#endif /* CAT_U32_T */
#ifndef CAT_S32_T
#define CAT_S32_T long
#endif /* CAT_S32_T */


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


typedef CAT_U8_T	uint8_t;
typedef CAT_S8_T	int8_t;
typedef CAT_U16_T	uint16_t;
typedef CAT_S16_T	int16_t;
typedef CAT_U32_T	uint32_t;
typedef CAT_S32_T	int32_t;
#if CAT_64BIT
typedef CAT_U64_T	uint64_t;
typedef CAT_S64_T	int64_t;
#endif /* CAT_64BIT */

#endif /* no defined headers: take guess*/

#endif /* CAT_HAS_FIXED_WIDTH */

#endif /* __cat_cattypes_h */
