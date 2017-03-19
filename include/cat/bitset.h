/*
 * bitset.h -- Bitmap routines
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2017 -- See accompanying license
 *
 */
#ifndef __bitset_h
#define __bitset_h

#include <cat/cat.h>

#if defined(CAT_USE_INLINE) && CAT_USE_INLINE 
#define DECL static inline
#define CAT_BITSET_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#endif /* CAT_USE_INLINE */

#ifndef CAT_CHAR_BIT
#define CAT_CHAR_BIT 8
#else
#define CAT_CHAR_BIT CHAR_BIT
#endif /* CHAR_BIT */

typedef uint bitset_t;

#define CAT_UINT_BIT (sizeof(bitset_t) * CAT_CHAR_BIT)

/* Calculate the number of elements in the bitset array */
#define BITSET_LEN(nbits) (((nbits) + (CAT_UINT_BIT-1)) / CAT_UINT_BIT)

/* Declare a bit set named 'name' with 'nbits' bits */
#define DECLARE_BITSET(name, nbits) bitset_t name[BITSET_LEN(nbits)]

/* Clear all entries in a bitset of length 'nbits' */
DECL void bset_zero(bitset_t *set, unsigned nbits);

/* Set all entries in a bitset of length 'nbits' */
DECL void bset_fill(bitset_t *set, unsigned nbits);

/* Copy 'src' to 'dst'.  These two bitsets should have the same size: 'nbits' */
DECL void bset_copy(bitset_t *dst, bitset_t *src, unsigned nbits);

/* Return non-zero if bit 'index' is set in 'set' or zero otherwise */
DECL int  bset_test(bitset_t *set, unsigned index);

/* Set bit 'index' in 'set' */
DECL void bset_set(bitset_t *set, unsigned index);

/* Clear bit 'index' in 'set' */
DECL void bset_clr(bitset_t *set, unsigned index);

/* Invert bit 'index' in 'set' */
DECL void bset_flip(bitset_t *set, unsigned index);

/* Set bit 'index' in 'set' to 0 if 'val' is 0 or 1 otherwise */
DECL void bset_set_to(bitset_t *set, unsigned index, int val);


/* ----- Implementation ----- */
#if defined(CAT_BITSET_DO_DECL) && CAT_BITSET_DO_DECL


DECL void bset_zero(bitset_t *set, unsigned nbits)
{
	unsigned i, len = BITSET_LEN(nbits);
	for ( i = 0 ; i < len ; ++i )
		set[i] = 0;
}


DECL void bset_fill(bitset_t *set, unsigned nbits)
{
	unsigned i, len = BITSET_LEN(nbits);
	for ( i = 0 ; i < len ; ++i )
		set[i] = (uint)~0;
}


DECL void bset_copy(bitset_t *dst, bitset_t *src, unsigned nbits)
{
	unsigned i, len = BITSET_LEN(nbits);
	if ( !len )
		return;
	for ( i = 0 ; i < len - 1 ; ++i )
		dst[i] = src[i];
	i *= CAT_UINT_BIT;
	for ( ; i < nbits ; ++i )
		bset_set_to(dst, i, bset_test(src, i));
}


DECL int bset_test(bitset_t *set, unsigned index)
{
	return (set[index / CAT_UINT_BIT] & (1 << (index % CAT_UINT_BIT))) != 0;
}


DECL void bset_set(bitset_t *set, unsigned index)
{
	set[index / CAT_UINT_BIT] |=  (1 << (index % CAT_UINT_BIT));
}


DECL void bset_clr(bitset_t *set, unsigned index)
{
	set[index / CAT_UINT_BIT] &=  ~(1 << (index % CAT_UINT_BIT));
}


DECL void bset_flip(bitset_t *set, unsigned index)
{
	set[index / CAT_UINT_BIT] ^=  (1 << (index % CAT_UINT_BIT));
}


DECL void bset_set_to(bitset_t *set, unsigned index, int val)
{
	if ( val )
		bset_set(set, index);
	else
		bset_clr(set, index);
}

#endif /* defined(CAT_BITSET_DO_DECL) && CAT_BITSET_DO_DECL */

#undef DECL
#undef CAT_LIST_DO_DECL

#endif /* __bitset_h */
