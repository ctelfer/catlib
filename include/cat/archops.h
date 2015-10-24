/*
 * include/cat/archops.h -- Operations accelerated on native platform
 *
 * Include bitops.h to get these functions.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2008-2015 -- See accompanying license
 *
 */
#ifndef __cat_archops_h
#define __cat_archops_h
#include <cat/cat.h>

extern char num_bits_array[];
extern char num_leading_zeros_array[];

#define nbits8(x) (num_bits_array[(uchar)(x) & 0xff])
#define nlz8(x) (num_leading_zeros_array[(uchar)(x) & 0xff])

#if CAT_USE_INLINE
#define INLINE inline
#else /* CAT_USE_INLINE */
#define INLINE
#endif /* CAT_USE_INLINE */

#define CAT_HAS_NLZ_32 0
#define CAT_HAS_NTZ_32 0
#define CAT_HAS_POP_32 0
#define CAT_HAS_NLZ_64 0
#define CAT_HAS_NTZ_64 0
#define CAT_HAS_POP_64 0

#if __i386__ && !CAT_ANSI89

#undef CAT_HAS_NLZ_32
#undef CAT_HAS_NTZ_32
#define CAT_HAS_NLZ_32 1
#define CAT_HAS_NTZ_32 1

static INLINE uint32_t ilog2_32(uint32_t x) { 
	if ( x == 0 ) {
		return -1;
	} else {
		int r;
		asm("bsr %1, %%eax\n"
		    "mov %%eax, %0\n"
		    : "=r" (r)
		    : "r" (x)
		    : "%eax");
		return r;
	}
}


static INLINE uint32_t nlz_32(uint32_t x) { 
	if ( x == 0 ) {
		return 32;
	} else {
		int r;
		asm("bsr %1, %%eax\n"
		    "mov %%eax, %0\n"
		    : "=r" (r)
		    : "r" (x)
		    : "%eax");
		return 31 - r;
	}
} 

static INLINE uint32_t ntz_32(uint32_t x) {  
	if ( x == 0 ) {
		return 32;
	} else {
		int r;
		asm("bsf %1, %%eax\n"
		    "mov %%eax, %0\n"
		    : "=r" (r)
		    : "r" (x)
		    : "%eax");
		return r;
	}
}

#else
/* more architectures here as needed */
#endif


/* ---------------------  Default Definitions ---------------------- */

#if !CAT_HAS_NLZ_32
static INLINE int nlz_32(uint32_t x) {
	int i = 1, b, n = 0;
	do {
		b = nlz8(x >> (32 - (i << 3)));
		n += b;
		i++;
	} while ( b == 8 && i <= sizeof(x) );
	return n;
}

static INLINE int ilog2_32(uint32_t x) { 
	if ( x == 0 )
		return -1;
	else
		return 31 - nlz_32(x);
}
#endif /* CAT_HAS_NLZ_32 */


#if !CAT_HAS_POP_32
static INLINE int pop_32(uint32_t x)
{
	x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
	x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
	x = x + (x >> 16);
	return x & 0x3F;
}
#endif /* !CAT_HAS_POP_32 */


#if !CAT_HAS_NTZ_32
static INLINE int ntz_32(uint32_t x) { 
	return pop_32(~x & (x-1));
}
#if CAT_HAS_POP_32
#undef CAT_HAS_NTZ_32
#define CAT_HAS_NTZ_32 1
#endif /* CAT_HAS_POP_32 */
#endif /* !CAT_HAS_NTZ_32 */


/* ------ 64 Bit ------ */

#if CAT_64BIT

#if !CAT_HAS_NLZ_64
#if CAT_HAS_NLZ_32
static INLINE int nlz_64(uint64_t x) {
	int v;
	if ( x == 0 )
		return 64;
	v = nlz_32(x >> 32);
	if ( v == 32 )
		v += nlz_32(x & 0xFFFFFFFF);
	return v;
}
#else /* CAT_HAS_NLZ_32 */
static INLINE int nlz_64(uint64_t x) {
	int i = 1, b, n = 0;
	do {
		b = nlz8(x >> (64 - (i << 3)));
		n += b;
		i++;
	} while ( b == 8 && i <= sizeof(x) );
	return n;
}
#endif /* CAT_HAS_NLZ_32 */

static INLINE int ilog2_64(uint64_t x) { 
	if ( x == 0 )
		return -1;
	else
		return 63 - nlz_64(x);
}
#endif /* CAT_HAS_NLZ_64 */


#if !CAT_HAS_POP_64
#if CAT_HAS_POP_32
static INLINE int pop_64(uint64_t x) {
	return pop_32(x >> 32) + pop_32(x & 0xFFFFFFFF);
}
#else /* CAT_HAS_POP_32 */
static int INLINE pop_64(uint64_t x)
{
#if CAT_HAS_LONGLONG
	x = (x & 0x5555555555555555LL) + ((x >> 1) & 0x5555555555555555LL);
	x = (x & 0x3333333333333333LL) + ((x >> 2) & 0x3333333333333333LL);
	x = (x & 0x0F0F0F0F0F0F0F0FLL) + ((x >> 4) & 0x0F0F0F0F0F0F0F0FLL);
	x = (x & 0x00FF00FF00FF00FFLL) + ((x >> 8) & 0x00FF00FF00FF00FFLL);
	x = (x & 0x0000FFFF0000FFFFLL) + ((x >> 16) & 0x0000FFFF0000FFFFLL);
#else /* CAT_HAS_LONGLONG */
	x = (x & 0x5555555555555555L) + ((x >> 1) & 0x5555555555555555L);
	x = (x & 0x3333333333333333L) + ((x >> 2) & 0x3333333333333333L);
	x = (x & 0x0F0F0F0F0F0F0F0FL) + ((x >> 4) & 0x0F0F0F0F0F0F0F0FL);
	x = (x & 0x00FF00FF00FF00FFL) + ((x >> 8) & 0x00FF00FF00FF00FFL);
	x = (x & 0x0000FFFF0000FFFFL) + ((x >> 16) & 0x0000FFFF0000FFFFL);
#endif /* CAT_HAS_LONGLONG */
	x = x + (x >> 32);
	return x & 0x7F;
}
#endif /* CAT_HAS_POP_32 */
#endif /* CAT_HAS_POP_64 */


#if !CAT_HAS_NTZ_64
#if CAT_HAS_NTZ_32
static INLINE int ntz_64(uint64_t x) {
	int v;
	if ( x == 0 )
		return 64;
	v = ntz_32(x & 0xFFFFFFFF);
	if ( v == 32 )
		v += ntz_32(x >> 32);
	return v;
}
#else /* CAT_HAS_NLZ_32 */
static INLINE int ntz_64(uint64_t x) { 
	return pop_64(~x & (x-1));
}

#if CAT_HAS_POP_64 || CAT_HAS_NTZ_32
#undef CAT_HAS_NTZ_64
#define CAT_HAS_NTZ_64 1
#endif /* CAT_HAS_POP_64 */
#endif /* CAT_HAS_NTZ_32 */

#endif /* !CAT_HAS_NLZ_64 */


#endif /* CAT_HAS_LONGLONG */

#undef INLINE

#endif /* __cat_archops_h */
