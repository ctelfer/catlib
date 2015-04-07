/*
 * cat/crc.h -- Cyclic Redundancy Check headers
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2015 -- See accompanying license
 *
 */

#ifndef __cat_crc32_h
#define __cat_crc32_h

#include <cat/cat.h>

struct crc32tab {
	ulong resid[256];
};


#define CAT_CRC32_POLY		0x04C11DB7
#define CAT_CRC32_POLY_REV	0xEDB88320
#define CAT_CRC32C_POLY		0x1EDC6F41


/* Table accelerated CRC32 operations */

/*
 * Initialize a CRC-32 table for big-endian bits given a polynomial.
 *
 *  - poly should represent a 33-bit irreducible polynomial in GF(2) with
 *    the high order coefficient (x^32) == 1.  The high order '1' is assumed
 *    and the parameter should contain the coefficients of the remaining 32
 *    terms in the polynomial.  The least-significant bit represents the
 *    coefficient of term x^0.
 */
void crc32t_be_init(struct crc32tab *t, ulong poly);

/*
 * Calculate the CRC-32 of a stream of bytes given an initial residue and
 * an initialized a table initialized by crc32t_be_init().  Returns the
 * new residue which one can pass to subsequent crc32t_be() calls.  The
 * bits in the individual bytes are processed in big-endian order.  The
 * residue is in big-endian bit order.
 */
ulong crc32t_be(const struct crc32tab *t, const void *p, size_t size,
	        ulong crc);

/*
 * Initialize a CRC-32 table for little-endian bits given a polynomial.
 *
 *  - poly should represent a 33-bit irreducible polynomial in GF(2) with
 *    the high order coefficient (x^32) == 1.  The high order '1' is assumed
 *    and the parameter should contain the coefficients of the remaining 32
 *    terms in the polynomial.  The least-significant bit represents the
 *    coefficient of term x^0.
 */
void crc32t_le_init(struct crc32tab *t, ulong poly);

/*
 * Calculate the CRC-32 of a stream of bytes given an initial residue and
 * an initialized a table initialized by crc32t_le_init().  Returns the
 * new residue which one can pass to subsequent crc32t_le() calls.  The
 * bits in the individual bytes are processed in little-endian order.  The
 * residue is in little-endian bit order.
 */
ulong crc32t_le(const struct crc32tab *t, const void *p, size_t size,
	        ulong crc);

/*
 * Initialize global state for standard CRC-32 checksum calculations.
 * Used for for embedded environments where C global data structure
 * initialization isn't automatic or in multi-threaded environments.
 */
void crc32_init(void);

/*
 * Begin a standard CRC-32 checksum and return the initial residue.
 * This function is threadsafe if crc32_init() has been called at
 * some point in the past.  
 * 
 * The standard CRC-32 checksum:
 *  - Starts with an initial residue of 0xFFFFFFFF
 *  - Processes bytes in little-endian bit-order
 *  - Returns a checksum of the final residue in little-endian bit order
 *    XORed with 0xFFFFFFFF
 */
ulong crc32_start(void);

/*
 * Continue a standard CRC-32 checksum by adding more bytes.
 *  - 'crc' must be the last current CRC residue returned either from
 *    crc32_start() or the latest crc32_step() call.
 */
ulong crc32_step(const void *p, size_t size, ulong crc);

/*
 * Output the the final CRC-32 given the last residue of a computation
 * begun with crc32_start() and crc32_step() calls.
 */
ulong crc32_finish(ulong crc);

/* Convenience wrapper for a one-shot standard CRC-32 checksum calculation */
#define crc32(p, size) \
	crc32_finish(crc32_step((p), (size), crc32_start()))

#endif /* __cat_crc32_h */
