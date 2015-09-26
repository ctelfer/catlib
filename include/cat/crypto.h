/*
 * cat/crypto.h -- Cryptographic algorithms
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009-2015 -- See accompanying license
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


/* SipHash hash function */

/* Each ulong[2] array is a little-endian 64-bit word */
struct siphashctx {
	uint nbytes;
	ulong state[2];
	ulong v0[2];
	ulong v1[2];
	ulong v2[2];
	ulong v3[2];
};

void siphash_init(struct siphashctx *shc, ulong key[4]);
void siphash24_add(struct siphashctx *shc, const void *p, ulong len);
void siphash24_fini(struct siphashctx *shc, byte_t hash[8]);
void siphash24(ulong key[4], const void *p, ulong len, byte_t hash[8]);

struct ht_sh24_ctx {
	ulong key[4];
};

void ht_sh24_init(struct ht_sh24_ctx *hsc, const void *k, ulong len);
uint ht_sh24_shash(const void *p, void *hsc);
uint ht_sh24_rhash(const void *p, void *hsc);

#endif /* __cat_crypto_h */
