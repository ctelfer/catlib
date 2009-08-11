#include <cat/cat.h>
#include <cat/bitops.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>


void print_bits32(uint32_t x) {
	int i;
	for ( i = 31; i >= 0; i-- ) {
		printf("%u", (uint)((x & ((uint32_t)1 << i)) >> i));
		if ( i == 0 )
			printf("\n");
		else if ( i % 4 == 0 )
			printf(" ");
	}

}


void print_bits64(uint64_t x) {
	int i;
	for ( i = 63; i >= 0; i-- ) {
		printf("%u", (uint)((x & ((uint64_t)1 << i)) >> i));
		if ( i == 0 )
			printf("\n");
		else if ( i % 4 == 0 )
			printf(" ");
	}
}


void test_compress()
{
	uint32_t word32 = (uint32_t)rand();
	uint32_t cl32, cr32;
	uint32_t sag32;
#if CAT_64BIT
	uint64_t word64 = (uint64_t)rand() | ((uint64_t)rand() << 32);
	uint64_t cl64, cr64;
#endif /* CAT_64BIT */

	printf("Testing compress\n");
	cl32 = compress_l32(word32, word32);
	cr32 = compress_r32(word32, word32);
	printf("Word 32 (%d bits):\n", pop_32(word32));
	print_bits32(word32);
	printf("compress left:\n");
	print_bits32(cl32);
	printf("compress right:\n");
	print_bits32(cr32);
	printf("\n");

	sag32 = SAG32(0xff00ff00, 0x00ffff00);
	printf("SAG32 of 0xff00ff00 mask = 0x00ffff00:\n");
	print_bits32(sag32);
	printf("\n");

#if CAT_64BIT
	cl64 = compress_l64(word64, word64);
	cr64 = compress_r64(word64, word64);
	printf("Word 64 (%d bits):\n", pop_64(word64));
	print_bits64(word64);
	printf("compress left:\n");
	print_bits64(cl64);
	printf("compress right:\n");
	print_bits64(cr64);
	printf("\n");
#endif /* CAT_64BIT */
}


void print_perm32(uint32_t x, uint8_t p[32]) {
	int i;
	for ( i = 31; i >= 0; i-- ) {
		printf("%u", (uint)((x & ((uint32_t)1 << p[i])) >> p[i]));
		if ( i == 0 )
			printf("\n");
		else if ( i % 4 == 0 )
			printf(" ");
	}

}


#if CAT_64BIT
void print_perm64(uint32_t x, uint8_t p[64]) {
	int i;
	for ( i = 63; i >= 0; i-- ) {
		printf("%u", (uint)((x & ((uint64_t)1 << p[i])) >> p[i]));
		if ( i == 0 )
			printf("\n");
		else if ( i % 4 == 0 )
			printf(" ");
	}

}
#endif /* CAT_64BIT */


void random_permutation(uint8_t arr[], size_t len)
{
	int i;
	uint8_t t, p;
	for ( i = 0; i < len; i++ )
		arr[i] = i;

	/* for each position randomly swap it with another position */
	for ( i = len - 1; i > 0; --i ) {
		p = (rand() & 0x7fff) % len;
		t = arr[p];
		arr[p] = arr[i];
		arr[i] = t;
	}

	printf("perm%d: ", (int)len);
	for ( i = 0; i < len; i++ ) {
		printf("%3u", arr[i]);
		if (i != len -1) {
			putchar(',');
			if (i % 16 == 15)
				printf("\n\t");
		}
	}
	printf("\n\n");
}


void reverse(uint8_t src[], uint8_t dst[], size_t len)
{
	int i;
	for ( i = 0; i < len; ++i )
		dst[src[i]] = i;
	printf("perm%dr:", (int)len);
	for ( i = 0; i < len; i++ ) {
		printf("%3u", dst[i]);
		if (i != len -1) {
			putchar(',');
			if (i % 16 == 15)
				printf("\n\t");
		}
	}
	printf("\n\n");
}


int main(int argc, char *argv[])
{
	int i;
	uint32_t word32, p32_sag, p32_bg;
	uint8_t perm32[32];
	uint8_t perm64[64];
	uint8_t perm32_SAG[32]; /* goes-to rather than comes-from */
	uint32_t sagpv32[5];
#if CAT_64BIT
	uint64_t sagpv64[6];
	uint64_t word64, p64_sag, p64_bg;
	uint8_t perm64_SAG[64]; /* goes-to rather than comes-from */
#endif /* CAT_64BIT */

	srand(time(NULL));
	test_compress();
	word32 = (uint32_t)rand();
	random_permutation(perm32, 32);
	reverse(perm32, perm32_SAG, 32);


	if (arr_to_SAG_permvec32(perm32_SAG, sagpv32) < 0) {
		printf("Error in SAG32 permutation vector\n");
		return -1;
	}
	p32_sag = permute32_SAG(word32, sagpv32);
	p32_bg  = bitgather32(word32, perm32);

#if CAT_64BIT
	word64 = (uint64_t)rand() | ((uint64_t)rand() << 32);
	random_permutation(perm64, 64);
	reverse(perm64, perm64_SAG, 64);
	if (arr_to_SAG_permvec64(perm64_SAG, sagpv64) < 0) {
		printf("Error in SAG64 permutation vector\n");
		return -1;
	}
	p64_sag = permute64_SAG(word64, sagpv64);
	p64_bg  = bitgather64(word64, perm64);
#endif /* CAT_64BIT */


	printf("Original 32 bits:\n");
	print_bits32(word32);
	printf("Local Perm 32 bits:\n");
	print_perm32(word32, perm32);
	printf("SAG32 result:\n");
	print_bits32(p32_sag);
	printf("BG-32 result:\n");
	print_bits32(p32_bg);
	if ( p32_sag == p32_bg ) {
		printf("32-bit SAG and 32-bit bit gather perms agree\n");
	} else {
		printf("32-bit SAG and 32-bit bit gather perms disagree\n");
	}
	for ( i = 0; i < 32; i++ ) {
		uint8_t b1 = !!(word32 & ((uint32_t)1 << perm32[i]));
		uint8_t b2 = !!(p32_sag & ((uint32_t)1 << i));
		if ( b1 != b2 ) {
			printf("32-bit SAG value is wrong ");
			printf("at position %d: %d vs %d.  Perm[%d] = %d\n",
				i, b1, b2, i, perm32[i]);
			break;
		}
	}
	for ( i = 0; i < 32; i++ ) {
		uint8_t b1 = !!(word32 & (1 << perm32[i]));
		uint8_t b2 = !!(p32_bg & (1 << i));
		if ( b1 != b2 ) {
			printf("32-bit bit gather value is wrong ");
			printf("at position %d: %d vs %d.  Perm[%d] = %d\n",
				i, b1, b2, i, perm32[i]);
			break;
		}
	}
	printf("\n");


#if CAT_64BIT
	printf("Original 64 bits:\n");
	print_bits64(word64);
	printf("Local Perm 64 bits:\n");
	print_perm64(word64, perm64);
	printf("SAG64 result:\n");
	print_bits64(p64_sag);
	printf("BG-64 result:\n");
	print_bits64(p64_bg);
	if ( p64_sag == p64_bg ) {
		printf("64-bit SAG and 64-bit bit gather perms agree\n");
	} else {
		printf("64-bit SAG and 64-bit bit gather perms disagree\n");
	}
	for ( i = 0; i < 64; i++ ) {
		uint8_t b1 = !!(word64 & ((uint64_t)1 << perm64[i]));
		uint8_t b2 = !!(p64_sag & ((uint64_t)1 << i));
		if ( b1 != b2 ) {
			printf("64-bit SAG value is wrong ");
			printf("at position %d: %d vs %d.  Perm[%d] = %d\n",
				i, b1, b2, i, perm64[i]);
			break;
		}
	}
	for ( i = 0; i < 64; i++ ) {
		uint8_t b1 = !!(word64 & ((uint64_t)1 << perm64[i]));
		uint8_t b2 = !!(p64_bg & ((uint64_t)1 << i));
		if ( b1 != b2 ) {
			printf("64-bit bit gather value is wrong ");
			printf("at position %d: %d vs %d.  Perm[%d] = %d\n",
				i, b1, b2, i, perm64[i]);
			break;
		}
	}
	printf("\n");
#endif /* CAT_64BIT */

	return 0;
}
