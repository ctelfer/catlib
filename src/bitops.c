/*
 * bitops.c -- bit manipulation operations.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/bitops.h>


char num_bits_array[256] = { 
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};


char num_leading_zeros_array[256] = { 
	8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


uint32_t rup2_32(uint32_t x, uint lg2p)
{
	uint32_t p2 = ((uint32_t)1 << lg2p) - 1;
	return (lg2p > 31) ? 0 : (x + p2) & ~p2;
}


uint32_t rdp2_32(uint32_t x, uint lg2p)
{
	return (lg2p > 31) ? 0 : (x & ~(((uint32_t)1 << lg2p) - 1));
}


#if CAT_64BIT
uint64_t rup2_64(uint64_t x, uint lg2p)
{
	uint64_t p2 = ((uint64_t)1 << lg2p) - 1;
	return (lg2p > 63) ? 0 : (x + p2) & ~p2;
}


uint64_t rdp2_64(uint64_t x, uint lg2p)
{
	return (lg2p > 63) ? 0 : (x & ~(((uint64_t)1 << lg2p) - 1));
}
#endif /* CAT_64BIT */


uint32_t compress_l32(uint32_t x, uint32_t m)
{
	uint32_t t, mp, mv, mk;
	int i;

	x &= m;
	mk = ~m >> 1;

	for ( i = 0; i < 5; ++i ) {
		mp = mk ^ (mk >> 1);
		mp = mp ^ (mp >> 2);
		mp = mp ^ (mp >> 4);
		mp = mp ^ (mp >> 8);
		mp = mp ^ (mp >> 16);
		mv = m & mp;
		m = (m ^ mv) | (mv << ((uint32_t)1 << i));
		t = x & mv;
		x = (x ^ t) | (t << ((uint32_t)1 << i));
		mk = mk & ~mp;
	}

	return x;
}


/* HD p. 119 */
uint32_t compress_r32(uint32_t x, uint32_t m)
{
	uint32_t t, mp, mv, mk;
	int i;

	x &= m;
	mk = ~m << 1;

	for ( i = 0; i < 5; ++i ) {
		mp = mk ^ (mk << 1);
		mp = mp ^ (mp << 2);
		mp = mp ^ (mp << 4);
		mp = mp ^ (mp << 8);
		mp = mp ^ (mp << 16);
		mv = m & mp;
		m = (m ^ mv) | (mv >> ((uint32_t)1 << i));
		t = x & mv;
		x = (x ^ t) | (t >> ((uint32_t)1 << i));
		mk = mk & ~mp;
	}

	return x;
}


#if CAT_64BIT
uint64_t compress_l64(uint64_t x, uint64_t m)
{
	uint64_t t, mp, mv, mk;
	int i;

	x &= m;
	mk = ~m >> 1;

	for ( i = 0; i < 6; ++i ) {
		mp = mk ^ (mk >> 1);
		mp = mp ^ (mp >> 2);
		mp = mp ^ (mp >> 4);
		mp = mp ^ (mp >> 8);
		mp = mp ^ (mp >> 16);
		mp = mp ^ (mp >> 32);
		mv = m & mp;
		m = (m ^ mv) | (mv << ((uint64_t)1 << i));
		t = x & mv;
		x = (x ^ t) | (t << ((uint64_t)1 << i));
		mk = mk & ~mp;
	}

	return x;
}


uint64_t compress_r64(uint64_t x, uint64_t m)
{
	uint64_t t, mp, mv, mk;
	int i;

	x &= m;
	mk = ~m << 1;

	for ( i = 0; i < 6; ++i ) {
		mp = mk ^ (mk << 1);
		mp = mp ^ (mp << 2);
		mp = mp ^ (mp << 4);
		mp = mp ^ (mp << 8);
		mp = mp ^ (mp << 16);
		mp = mp ^ (mp << 32);
		mv = m & mp;
		m = (m ^ mv) | (mv >> ((uint64_t)1 << i));
		t = x & mv;
		x = (x ^ t) | (t >> ((uint64_t)1 << i));
		mk = mk & ~mp;
	}

	return x;
}
#endif /* CAT_64BIT */


uint32_t SAG32(uint32_t x, uint32_t mask)
{
	return compress_l32(x, mask) | compress_r32(x, ~mask);
}


#if CAT_64BIT
uint64_t SAG64(uint64_t x, uint64_t mask)
{
	return compress_l64(x, mask) | compress_r64(x, ~mask);
}
#endif /* CAT_64BIT */


/* converts an array of bit positions to a permutation vector used to permute */
/* the bits of a 32-bit word usinv the permute32() function */
int arr_to_SAG_permvec32(uint8_t arr[32], uint32_t pv[5])
{
	int i, j;
	uint32_t t = 0, bit;

	/* ensures that arr[] represents a real permutation */
	for ( i = 0; i < 32; ++i ) {
		if ( arr[i] & ~0x1f )
			return -1;
		bit = (uint32_t)1 << arr[i];
		if ( t & bit )
			return -1;
		t |= bit;
	}

	/* Each bit k (low order bit is 0) in pv[i] contains the ith bit */
	/* of k's position in the final permutation. */
	for ( i = 0; i < 5; ++i ) {
		pv[i] = 0;
		for ( j = 0; j < 32; ++j )
			pv[i] |= ((arr[j] >> i) & 1) << j;
	}

	/* now permute each bit of p[x] for x > 1 so it lines up with */
	/* the permuted bit during shuffle x */
	pv[1] = SAG32(pv[1], pv[0]);
	pv[2] = SAG32(SAG32(pv[2], pv[0]), pv[1]);
	pv[3] = SAG32(SAG32(SAG32(pv[3], pv[0]), pv[1]), pv[2]);
	pv[4] = SAG32(SAG32(SAG32(SAG32(pv[4], pv[0]), pv[1]), pv[2]), pv[3]);

	return 0;
}


/* Performs a stable base-2 radix sort of the bits in "bits" ascribing a */
/* value to each bit position equal its final position (as a number). */
/* The SAG32() operation does the actual moving of bits */
uint32_t permute32_SAG(uint32_t bits, uint32_t pv[5]) 
{
	bits = SAG32(bits, pv[0]);
	bits = SAG32(bits, pv[1]);
	bits = SAG32(bits, pv[2]);
	bits = SAG32(bits, pv[3]);
	bits = SAG32(bits, pv[4]);
	return bits;
}


#if CAT_64BIT
/* converts an array of bit positions to a permutation vector used to permute */
/* the bits of a 64-bit word usinv the permute64() function */
int arr_to_SAG_permvec64(uint8_t arr[64], uint64_t pv[6])
{
	int i, j;
	uint64_t t = 0, bit;

	/* ensures that arr[] represents a real permutation */
	for ( i = 0; i < 64; ++i ) {
		if ( arr[i] & ~0x3f )
			return -1;
		bit = (uint64_t)1 << arr[i];
		if ( t & bit )
			return -1;
		t |= bit;
	}

	/* Each bit k (low order bit is 0) in pv[i] contains the ith bit */
	/* of k's position in the final permutation. */
	for ( i = 0; i < 6; ++i ) {
		pv[i] = 0;
		for ( j = 0; j < 64; ++j )
			pv[i] |= (uint64_t)((arr[j] >> i) & 1) << j;
	}

	/* now permute each bit of p[x] for x > 1 so it lines up with */
	/* the permuted bit during shuffle x */
	pv[1] = SAG64(pv[1], pv[0]);
	pv[2] = SAG64(SAG64(pv[2], pv[0]), pv[1]);
	pv[3] = SAG64(SAG64(SAG64(pv[3], pv[0]), pv[1]), pv[2]);
	pv[4] = SAG64(SAG64(SAG64(SAG64(pv[4], pv[0]), pv[1]), pv[2]), pv[3]);
	pv[5] = SAG64(SAG64(SAG64(SAG64(SAG64(pv[5], pv[0]), pv[1]), pv[2]), 
			pv[3]), pv[4]);

	return 0;
}


/* Performs a stable base-2 radix sort of the bits in "bits" ascribing a */
/* value to each bit position equal its final position (as a number). */
/* The SAG64() operation does the actual moving of bits. */
uint64_t permute64_SAG(uint64_t bits, uint64_t pv[6]) 
{
	bits = SAG64(bits, pv[0]);
	bits = SAG64(bits, pv[1]);
	bits = SAG64(bits, pv[2]);
	bits = SAG64(bits, pv[3]);
	bits = SAG64(bits, pv[4]);
	bits = SAG64(bits, pv[5]);
	return bits;
}
#endif /* CAT_64BIT */


uint32_t bitgather32(uint32_t bits, uint8_t pos[32])
{
	int i;
	uint32_t x = 0;
	for ( i = 0; i < 32; ++i )
		x |= ((bits >> (pos[i] & 0x1f)) & 1) << i;
	return x;
}


#if CAT_64BIT
uint64_t bitgather64(uint64_t bits, uint8_t pos[64])
{
	int i;
	uint64_t x = 0;
	for ( i = 0; i < 64; ++i )
		x |= ((bits >> (pos[i] & 0x3f)) & 1) << i;
	return x;
}
#endif /* CAT_64BIT */


