/*
 * cat/crypto.h -- Cryptographic algorithms
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009-2012 -- See accompanying license
 *
 */

#ifndef __cat_crypto_h
#define __cat_crypto_h

#include <cat/cat.h>

/* ARC4 Stream Cipher */
struct arc4ctx {
	uint		i;
	uint		j;
	byte_t		s[256];
};

void arc4_init(struct arc4ctx *arc4, void *key, ulong len);
void arc4_gen(struct arc4ctx *arc4, void *out, ulong len);
void arc4_encrypt(struct arc4ctx *arc4, void *in, void *out, ulong len);


/* SHA-2 256 hash function */
struct sha256ctx {
	ulong		llo;		/* length_lo: in bytes until fini */
	ulong		lhi;		/* length_hi: in bytes until fini */
	ulong		w[64];		/* 512-bits message + derived */
	ulong		h[8];		/* current hash */
};


/* only operates on byte-level granularity */
void sha256_init(struct sha256ctx *s);
void sha256_add(struct sha256ctx *s, void *p, ulong len);
void sha256_fini(struct sha256ctx *s, byte_t hash[32]);
void sha256(void *in, ulong len, byte_t hash[32]);

#endif /* __cat_crypto_h */
