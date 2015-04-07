/*
 * crc.c -- Cyclic Redundancy Check implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2015 See accompanying license
 *
 */

#include <cat/crc.h>


static byte_t rev8(byte_t x)
{
	x = ((x >> 4) & 0x0F) | (x << 4);
	x = ((x >> 2) & 0x33) | ((x << 2) & 0xCC);
	x = ((x >> 1) & 0x55) | ((x << 1) & 0xAA);
	return x;
}


static ulong rev32(ulong x)
{
	x = ((x >> 16) & 0xFFFF) | (x << 16);
	x = ((x >> 8) & 0x00FF00FF) | ((x << 8) & 0xFF00FF00);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x << 4) & 0xF0F0F0F0);
	x = ((x >> 2) & 0x33333333) | ((x << 2) & 0xCCCCCCCC);
	x = ((x >> 1) & 0x55555555) | ((x << 1) & 0xAAAAAAAA);
	return x;
}


void crc32t_be_init(struct crc32tab *tab, ulong poly)
{
	int i, j;
	ulong r;
	for ( i = 0; i < 256; ++i ) {
		r = i << 24;
		for ( j = 0; j < 8; ++j ) {
			if ( (r >> 31) & 1 )
				r = (r << 1) ^ poly;
			else
				r = (r << 1);
		}
		tab->resid[i] = r & 0xFFFFFFFF;
	}
}


ulong crc32t_be(const struct crc32tab *tab, const void *voidp, size_t size,
	        ulong crc)
{
	const byte_t *p;
	for ( p = voidp ; size > 0 ; --size, ++p )
		crc = tab->resid[(*p ^ (crc >> 24)) & 0xFF] ^ (crc << 8);
	return crc & 0xFFFFFFFF;
}


void crc32t_le_init(struct crc32tab *tab, ulong poly)
{
	int i, j;
	ulong r;
	for ( i = 0; i < 256; ++i ) {
		r = rev8(i) << 24;
		for ( j = 0; j < 8; ++j ) {
			if ( (r >> 31) & 1 )
				r = (r << 1) ^ poly;
			else
				r = (r << 1);
		}
		tab->resid[i] = rev32(r & 0xFFFFFFFF);
	}
}


ulong crc32t_le(const struct crc32tab *tab, const void *voidp, size_t size,
	        ulong crc)
{
	const byte_t *p;
	for ( p = voidp ; size > 0 ; --size, ++p )
		crc = tab->resid[(*p ^ crc) & 0xFF] ^ (crc >> 8);
	return crc;
}


static struct crc32tab _crc32_table;
static int _crc32_table_initialized = 0;


void crc32_init(void)
{
	crc32t_le_init(&_crc32_table, CAT_CRC32_POLY);
	_crc32_table_initialized = 1;
}


ulong crc32_start(void)
{
	if (!_crc32_table_initialized)
		crc32_init();
	return 0xFFFFFFFF;
}

ulong crc32_step(const void *p, size_t size, ulong crc)
{
	return crc32t_le(&_crc32_table, p, size, crc);
}

ulong crc32_finish(ulong crc)
{
	return crc ^ 0xFFFFFFFF;
}
