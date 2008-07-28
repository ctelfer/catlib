#ifndef __bitops_h
#define __bitops_h

#include <cat/cattypes.h>

#if CAT_USE_INLINE
#define DECL static inline
#else
#define DECL static
#endif


DECL int pop32(uint32_t x)
{
	x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
	x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
	x = x + (x >> 16);
	return x & 0x3F;
}


#if CAT_HAS_LONGLONG
DECL int pop64(uint64_t x)
{
	x = (x & 0x5555555555555555LL) + ((x >> 1) & 0x5555555555555555LL);
	x = (x & 0x3333333333333333LL) + ((x >> 2) & 0x3333333333333333LL);
	x = (x & 0x0F0F0F0F0F0F0F0FLL) + ((x >> 4) & 0x0F0F0F0F0F0F0F0FLL);
	x = (x & 0x00FF00FF00FF00FFLL) + ((x >> 8) & 0x00FF00FF00FF00FFLL);
	x = (x & 0x0000FFFF0000FFFFLL) + ((x >> 16) & 0x0000FFFF0000FFFFLL);
	x = x + (x >> 32);
	return x & 0x7F;
}
#endif /* CAT_HAS_LONGLONG */

uint32_t compress_l32(uint32_t x, uint32_t mask);
uint32_t compress_r32(uint32_t x, uint32_t mask);
uint64_t compress_l64(uint64_t x, uint64_t mask);
uint64_t compress_r64(uint64_t x, uint64_t mask);

uint32_t SAG32(uint32_t x, uint32_t maskleft);
uint64_t SAG64(uint64_t x, uint64_t maskleft);

int arr_to_SAG_permvec32(uint8_t arr[32], uint32_t permvec[5]);
uint32_t permute32_SAG(uint32_t bits, uint32_t permvec[5]);
int arr_to_SAG_permvec64(uint8_t arr[64], uint64_t permvec[6]);
uint64_t permute64_SAG(uint64_t bits, uint64_t permvec[6]);

uint32_t bitgather32(uint32_t bits, uint8_t pos[32]);
uint64_t bitgather64(uint64_t bits, uint8_t pos[64]);

#undef DECL

#endif /* __bitops_h */
