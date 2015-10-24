/*
 * cat/pack.h -- functions to pack and unpack values into byte arrays
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2015 See accompanying license
 *
 */

#ifndef __cat_pack_h
#define __cat_pack_h

#include <cat/cat.h>

/* 
	 "eEbhwjBHWJr"

	 e - little endian
	 E - big endian
	 b - byte (8 bits)            -- pass in byte_t / uchar / uchar
	 h - half-word (16 bits)      -- pass in short / ushort
	 w - word (32 bits)           -- pass in long / ulong
	 j - jumbo word (64 bits)     -- pass in long long / ullong
	 B - signed 8 bit value       -- pass in signed char / schar
	 H - signed 16 bit value      -- pass in short
	 W - signed 32 bit value      -- pass in long
	 J - signed 64 bit value      -- pass in long long
	 r - (struct raw *) follows (pack only)

	 Prefix with a number to indicate a count.  In this case source or 
	 destination operand is an array of the type indicated.
*/

size_t pack(void * buf, size_t len, const char *fmt, ... ); 
size_t unpack(void * buf, size_t len, const char *fmt, ... );
size_t packlen(const char *fmt, ... );

#define PSIZ_BYTE	1
#define PSIZ_HALF	2
#define PSIZ_WORD	4
#define PSIZ_JUMBO	8

#ifdef CAT_PSIZ_MAX
#define PSIZ_MAX	CAT_PSIZ_MAX
#else  /* CAT_PSIZ_MAX */
#define PSIZ_MAX	0x7fffffff
#endif /* CAT_PSIZ_MAX */


#if CAT_USE_INLINE
#define DECL static inline
#else /* CAT_USE_INLINE */
#define DECL static
#endif /* CAT_USE_INLINE */

/* These functions all assume that CHAR_BIT == 8 */
DECL uint16_t ntoh16(uint16_t v) {
	byte_t *p = (byte_t*)&v;
	return (p[0] << 8) | p[1];
}

DECL uint16_t ntoh16x(void *pp) {
	byte_t *p = pp;
	return ((uint16_t)p[0] << 8) |
		(uint16_t)p[1];
}

DECL uint16_t hton16(uint16_t v) {
	uint16_t ov = 0;
	byte_t *p = (byte_t*)&ov;
	*p++ = v >> 8;
	*p = v;
	return ov;
}

DECL void hton16i(uint16_t v, void *pp) {
	byte_t *p = (byte_t*)pp;
	*p++ = v >> 8;
	*p = v;
}

DECL uint32_t ntoh32(uint32_t v) {
	byte_t *p = (byte_t*)&v;
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | 
		((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

DECL uint32_t ntoh32x(void *pp) {
	byte_t *p = pp;
	return  ((uint32_t)p[0] << 24) |
		((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8)  | 
		(uint32_t)p[3];
}

DECL uint32_t hton32(uint32_t v) {
	uint32_t ov = 0;
	byte_t *p = (byte_t*)&ov;
	*p++ = v >> 24;
	*p++ = v >> 16;
	*p++ = v >> 8;
	*p = v;
	return ov;
}

DECL void hton32i(uint32_t v, void *pp) {
	byte_t *p = (byte_t*)pp;
	*p++ = v >> 24;
	*p++ = v >> 16;
	*p++ = v >> 8;
	*p = v;
}

#if CAT_64BIT
DECL uint64_t ntoh64(uint64_t v) {
	byte_t *p = (byte_t*)&v;
	return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) | 
		((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) | 
		((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) | 
		((uint64_t)p[6] << 8) | (uint64_t)p[7];
}

DECL uint64_t ntoh64x(void *pp) {
	byte_t *p = pp;
	return  ((uint64_t)p[0] << 56) |
		((uint64_t)p[1] << 48) |
		((uint64_t)p[2] << 40) | 
		((uint64_t)p[3] << 32) |
		((uint64_t)p[4] << 24) |
		((uint64_t)p[5] << 16) |
		((uint64_t)p[6] << 8)  | 
		(uint64_t)p[7];
}

DECL uint64_t hton64(uint64_t v) {
	uint64_t ov = 0;
	byte_t *p = (byte_t*)&ov;
	*p++ = v >> 56;
	*p++ = v >> 48;
	*p++ = v >> 40;
	*p++ = v >> 32;
	*p++ = v >> 24;
	*p++ = v >> 16;
	*p++ = v >> 8;
	*p = v;
	return ov;
}

DECL void hton64i(uint64_t v, void *pp) {
	byte_t *p = (byte_t*)pp;
	*p++ = v >> 56;
	*p++ = v >> 48;
	*p++ = v >> 40;
	*p++ = v >> 32;
	*p++ = v >> 24;
	*p++ = v >> 16;
	*p++ = v >> 8;
	*p = v;
}

#endif /* CAT_64BIT */

#endif /* __cat_pack_h */
