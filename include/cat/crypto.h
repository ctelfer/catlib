/*
 * cat/crypto.h -- Cryptographic algorithms
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009, See accompanying license
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

void arc4_init(struct arc4ctx *arc4, struct raw *key);
void arc4_gen(struct arc4ctx *arc4, struct raw *out);
void arc4_encrypt(struct arc4ctx *arc4, struct raw *in, struct raw *out);


#endif /* __cat_crypto_h */
