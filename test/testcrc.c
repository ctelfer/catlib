#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cat/crc.h>
#include <cat/err.h>

const char check_vec[] = "123456789";

#define CRC32_CHECK_VAL		0xCBF43926ul
#define CRC32C_CHECK_VAL	0xE3069283ul


void crc32_file(const char *s)
{
	FILE *fp;
	char buf[1024];
	size_t n;
	ulong crc;

	crc = crc32_start();
	fp = fopen(s, "r");
	if ( !fp )
		errsys("Unable to open file %s", s);
	do {
		n = fread(buf, 1, sizeof(buf), fp);
		if ( n )
			crc = crc32_step(buf, n, crc);
	} while ( n == sizeof(buf) );

	if ( ferror(fp) )
		errsys("Error reading file %s", fp);

	printf("CRC32 for file %s is 0x%08lx\n", s, crc32_finish(crc));
}


static void check_crc32_1bit(void)
{
	ulong crc;
	uchar byte = 1;
	struct crc32tab ctab;

	crc32t_be_init(&ctab, CAT_CRC32_POLY);
	crc = crc32t_be(&ctab, &byte, 1, 0);
	fprintf(stderr, "CRC32 of 0x01 is 0x%08lx.  Should be 0x%08lx\n",
		crc, (ulong)CAT_CRC32_POLY);
	if ( crc == CAT_CRC32_POLY )
		fprintf(stderr, "PASSED\n");
	else
		fprintf(stderr, "FAILED\n");
}


static ulong reverse(ulong x)
{
        x = ((x >> 16) & 0xFFFF) | (x << 16);
        x = ((x >> 8) & 0x00FF00FF) | ((x << 8) & 0xFF00FF00);
        x = ((x >> 4) & 0x0F0F0F0F) | ((x << 4) & 0xF0F0F0F0);
        x = ((x >> 2) & 0x33333333) | ((x << 2) & 0xCCCCCCCC);
        x = ((x >> 1) & 0x55555555) | ((x << 1) & 0xAAAAAAAA);
        return x;
}


static ulong dumbcrc32(const void *voidp, size_t len)
{
	const byte_t *p;
	int i;
	ulong crc = 0xFFFFFFFF;

	for ( p = voidp; *p; p++ ) {
		for ( i = 0; i < 8; i++ ) {
			if ( ((*p >> i) ^ (crc >> 31)) & 1 )
				crc = (crc << 1) ^ CAT_CRC32_POLY;
			else
				crc <<= 1;
			crc &= 0xFFFFFFFF;
		}
	}

	return reverse(crc) ^ 0xFFFFFFFF;
}


static void check_crc32_dumb(void)
{
	ulong crc;

	crc = dumbcrc32(check_vec, strlen(check_vec));
	fprintf(stderr, "Dumb CRC32 of \"%s\" is 0x%08lx.  Should be 0x%08lx\n",
		check_vec, crc, CRC32_CHECK_VAL);
	if ( crc == CRC32_CHECK_VAL )
		fprintf(stderr, "PASSED\n");
	else
		fprintf(stderr, "FAILED\n");
}


static void check_crc32(void)
{
	ulong crc;
	crc = crc32(check_vec, strlen(check_vec));
	fprintf(stderr, "CRC32 of \"%s\" is 0x%08lx.  Should be 0x%08lx\n",
		check_vec, crc, CRC32_CHECK_VAL);
	if ( crc == CRC32_CHECK_VAL )
		fprintf(stderr, "PASSED\n");
	else
		fprintf(stderr, "FAILED\n");
}


static void check_crc32c(void)
{
	ulong crc = 0xFFFFFFFF;
	struct crc32tab ctab;

	crc32t_le_init(&ctab, CAT_CRC32C_POLY);

	crc = crc32t_le(&ctab, check_vec, strlen(check_vec), crc);
	crc ^= 0xFFFFFFFF;

	fprintf(stderr, "CRC32c of \"%s\" is 0x%08lx.  Should be 0x%08lx\n",
		check_vec, crc, CRC32C_CHECK_VAL);
	if ( crc == CRC32C_CHECK_VAL )
		fprintf(stderr, "PASSED\n");
	else
		fprintf(stderr, "FAILED\n");
}


int main(int argc, char *argv[])
{
	if ( argc == 2 && strcmp(argv[1], "-h") == 0 ) {
		fprintf(stderr, "usage: %s [file]\n", argv[0]);
		exit(1);
	}

	check_crc32_1bit();
	check_crc32_dumb();
	check_crc32();
	check_crc32c();

	if ( argc > 1 ) 
		crc32_file(argv[1]);
	return 0;
}
