/*
 * crypto.c -- Cryptographic algorithm implementations
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009, See accompanying license
 *
 */

#include <cat/crypto.h>

void arc4_init(struct arc4ctx *arc4, struct raw *key)
{
	int i, j;
	byte_t b;
	abort_unless(arc4 != NULL && key != NULL && key->len > 0 && 
		     key->data != NULL);

	 for ( i = 0; i < 256 ; ++i )
		 arc4->s[i] = i;

	 j = 0;
	 for ( i = 0; i < 256 ; ++i ) {
		 j = (j + arc4->s[i] + key->data[i % key->len]) & 0xFF;
		 b = arc4->s[i];
		 arc4->s[i] = arc4->s[j];
		 arc4->s[j] = b;
	 }
	 arc4->i = 0;
	 arc4->j = 0;
}


void arc4_gen(struct arc4ctx *arc4, struct raw *out)
{
	size_t len;
	int i, j;
	byte_t b1, b2;
	byte_t *outp;

	abort_unless(arc4 != NULL && out != NULL && 
		     (out->len == 0 || out->data != NULL));

	i = arc4->i;
	j = arc4->j;
	len = out->len;
	outp = out->data;

	while ( len-- > 0 ) {
		i = (i + 1) & 0xff;
		j = (j + arc4->s[i]) & 0xff;
		b1 = arc4->s[i];
		b2 = arc4->s[i] = arc4->s[j];
		arc4->s[j] = b1;
		*outp++ = arc4->s[(b1 + b2) & 0xff];
	}

	arc4->i = i;
	arc4->j = j;
}


void arc4_encrypt(struct arc4ctx *arc4, struct raw *in, struct raw *out)
{
	size_t len;
	int i, j;
	byte_t b1, b2;
	byte_t *inp, *outp;

	abort_unless(arc4 != NULL && in != NULL && out != NULL && 
		     (in->len == 0 || in->data != NULL) &&
		     (out->len == 0 || out->data != NULL));

	len = (in->len > out->len) ? out->len : in->len;
	inp = in->data;
	outp = out->data;
	i = arc4->i;
	j = arc4->j;

	while ( len-- > 0 ) {
		i = (i + 1) & 0xff;
		j = (j + arc4->s[i]) & 0xff;
		b1 = arc4->s[i];
		b2 = arc4->s[i] = arc4->s[j];
		arc4->s[j] = b1;
		*outp++ = *inp++ ^ arc4->s[(b1 + b2) & 0xff];
	}

	arc4->i = i;
	arc4->j = j;
}
