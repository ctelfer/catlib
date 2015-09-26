/*
 * crypto.c -- Cryptographic algorithm implementations
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009-2015 See accompanying license
 *
 */

#include <cat/crypto.h>

#include <string.h>

void arc4_init(struct arc4ctx *arc4, void *key, ulong len)
{
	int i, j;
	byte_t b;
	byte_t *keyp = key;
	abort_unless(arc4 != NULL && (key != NULL || len == 0));

	 for ( i = 0; i < 256 ; ++i )
		 arc4->s[i] = i;

	 j = 0;
	 for ( i = 0; i < 256 ; ++i ) {
		 j = (j + arc4->s[i] + keyp[i % len]) & 0xFF;
		 b = arc4->s[i];
		 arc4->s[i] = arc4->s[j];
		 arc4->s[j] = b;
	 }
	 arc4->i = 0;
	 arc4->j = 0;
}


void arc4_gen(struct arc4ctx *arc4, void *out, ulong len)
{
	int i, j;
	byte_t b1, b2;
	byte_t *outp = out;

	abort_unless(arc4 != NULL && (out != NULL || len == 0));

	i = arc4->i;
	j = arc4->j;

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


void arc4_encrypt(struct arc4ctx *arc4, void *in, void *out, ulong len)
{
	int i, j;
	byte_t b1, b2;
	byte_t *inp = in, *outp = out;

	abort_unless(arc4 != NULL && 
		     ((in != NULL && out != NULL) || len == 0));

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



static ulong sha256_k[64] = { 
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};


static ulong ror(ulong x, uint amt) {
	return ((x >> amt) | (x << (32 - amt))) & 0xFFFFFFFF;
}


static ulong Ch(ulong x, ulong y, ulong z) {
	return (x & y) ^ (~x & z);
}


static ulong Maj(ulong x, ulong y, ulong z) {
	return (x & y) ^ (x & z) ^ (y & z);
}


static ulong Sigma0(ulong x) {
	return ror(x, 2) ^ ror(x, 13) ^ ror(x, 22);
}


static ulong Sigma1(ulong x) {
	return ror(x, 6) ^ ror(x, 11) ^ ror(x, 25);
}


static ulong sigma0(ulong x) {
	return ror(x, 7) ^ ror(x, 18) ^ ((x >> 3) & 0xFFFFFFFF);
}


static ulong sigma1(ulong x) {
	return ror(x, 17) ^ ror(x, 19) ^ ((x >> 10) & 0xFFFFFFFF);
}


static int sha256_add_bytes(struct sha256ctx *s, byte_t **pp, ulong *len)
{
	int idx = s->llo & 63;
	int toadd = 64 - idx;
	int i;
	byte_t *p = *pp;
	ulong olo;

	if ( toadd > *len )
		toadd = *len;

	for ( i = 0; i < toadd; ++i ) {
		switch ( idx & 3 ) {
		case 0: s->w[idx / 4] = ((ulong)*p++ << 24);	break;
		case 1: s->w[idx / 4] |= ((ulong)*p++ << 16);	break;
		case 2: s->w[idx / 4] |= ((ulong)*p++ << 8);	break;
		case 3: s->w[idx / 4] |= (ulong)*p++;	 	break;
		}
		++idx;
	}

	*pp = p;
	*len -= toadd;

	olo = s->llo;
	s->llo = (olo + toadd) & 0xFFFFFFFF;
	if ( s->llo < olo )
		s->lhi++;

	return (idx == 64);
}


void sha256_init(struct sha256ctx *s)
{
	memset(s, 0, sizeof(*s));
	s->llo = 0;
	s->lhi = 0;
	s->h[0] = 0x6a09e667;
	s->h[1] = 0xbb67ae85;
	s->h[2] = 0x3c6ef372;
	s->h[3] = 0xa54ff53a;
	s->h[4] = 0x510e527f;
	s->h[5] = 0x9b05688c;
	s->h[6] = 0x1f83d9ab;
	s->h[7] = 0x5be0cd19;
}


void sha256_hash_block(struct sha256ctx *s)
{
	int i;
	ulong a, b, c, d, e, f, g, h, t1, t2;

	/* compute the schedule */
	for ( i = 16; i < 64; ++i )
		s->w[i] = (sigma1(s->w[i-2]) + s->w[i-7] + 
			   sigma0(s->w[i-15]) + s->w[i-16]) & 0xFFFFFFFF;

	/* set the working variables */
	a = s->h[0];
	b = s->h[1];
	c = s->h[2];
	d = s->h[3];
	e = s->h[4];
	f = s->h[5];
	g = s->h[6];
	h = s->h[7];

	/* hash rounds */
	for ( i = 0; i < 64; ++i ) {
		t1 = (h + Sigma1(e) + Ch(e, f, g) + sha256_k[i] + s->w[i]) & 
		     0xFFFFFFFF;

		t2 = (Sigma0(a) + Maj(a, b, c)) & 0xFFFFFFFF;

		h = g;
		g = f;
		f = e;
		e = (d + t1) & 0xFFFFFFFF;
		d = c;
		c = b;
		b = a;
		a = (t1 + t2) & 0xFFFFFFFF;
	}

	s->h[0] = (s->h[0] + a) & 0xFFFFFFFF;
	s->h[1] = (s->h[1] + b) & 0xFFFFFFFF;
	s->h[2] = (s->h[2] + c) & 0xFFFFFFFF;
	s->h[3] = (s->h[3] + d) & 0xFFFFFFFF;
	s->h[4] = (s->h[4] + e) & 0xFFFFFFFF;
	s->h[5] = (s->h[5] + f) & 0xFFFFFFFF;
	s->h[6] = (s->h[6] + g) & 0xFFFFFFFF;
	s->h[7] = (s->h[7] + h) & 0xFFFFFFFF;
}


void sha256_add(struct sha256ctx *s, void *vp, ulong len)
{
	byte_t *p = vp;
	while ( len > 0 )
		if ( sha256_add_bytes(s, &p, &len) )
			sha256_hash_block(s);
}


void sha256_fini(struct sha256ctx *s, byte_t hash[32])
{
	byte_t pad[72];
	byte_t *p;
	ulong plen;
	ulong llo, lhi;
	int i;

	/* initialize the pad bytes to add */
	memset(pad, 0, sizeof(pad));
	pad[0] = 0x80;
	plen = 64 - (s->llo % 63);
	if ( plen < 9 )
		plen += 64;

	/* compute length into bits */
	lhi = (s->lhi << 3) + (s->llo >> 29);
	llo = (s->llo << 3) & 0xFFFFFFFF;

	/* encode as a big-endian bytes */
	p = &pad[plen - 8];
	*p++ = (lhi >> 24) & 0xFF;
	*p++ = (lhi >> 16) & 0xFF;
	*p++ = (lhi >>  8) & 0xFF;
	*p++ = (lhi >>  0) & 0xFF;
	*p++ = (llo >> 24) & 0xFF;
	*p++ = (llo >> 16) & 0xFF;
	*p++ = (llo >>  8) & 0xFF;
	*p++ = (llo >>  0) & 0xFF;

	/* hash the last block(s) */
	p = pad;
	while ( plen > 0 ) {
		sha256_add_bytes(s, &p, &plen);
		sha256_hash_block(s);
	}

	/* extract the hash value into hash bytes */
	for ( i = 0; i < 32; ++i )
		hash[i] = (s->h[i / 4] >> ((3 - (i % 4)) * 8)) & 0xFF;

	memset(s, 0, sizeof(*s));
}


void sha256(void *in, ulong len, byte_t hash[32])
{
	struct sha256ctx s;
	sha256_init(&s);
	sha256_add(&s, in, len);
	sha256_fini(&s, hash);
}


void siphash_init(struct siphashctx *shc, ulong key[4])
{
	shc->nbytes = 0;
	shc->state[0] = 0;
	shc->state[1] = 0;
	shc->v0[0] = 0x70736575ul ^ key[0];
	shc->v0[1] = 0x736f6d65ul ^ key[1];
	shc->v1[0] = 0x6e646f6dul ^ key[2];
	shc->v1[1] = 0x646f7261ul ^ key[3];
	shc->v2[0] = 0x6e657261ul ^ key[0];
	shc->v2[1] = 0x6c796765ul ^ key[1];
	shc->v3[0] = 0x79746573ul ^ key[2];
	shc->v3[1] = 0x74656462ul ^ key[3];
}


#define ADD64(_v0, _v1) 						\
	do {								\
		ulong _ov00 = (_v0)[0];					\
		(_v0)[0] += (_v1)[0];					\
		(_v0)[0] &= 0xFFFFFFFF;					\
		(_v0)[1] += (_v1)[1] + ((_v0)[0] < _ov00);		\
		(_v0)[1] &= 0xFFFFFFFF;					\
	} while (0)

#define ROTL64(_v, _amt)						\
	do {								\
		ulong out0 = (_v)[0] >> (32 - (_amt));			\
		ulong out1 = (_v)[1] >> (32 - (_amt));			\
		(_v)[0] = (((_v)[0] << (_amt)) | out1) & 0xFFFFFFFF;	\
		(_v)[1] = (((_v)[1] << (_amt)) | out0) & 0xFFFFFFFF;	\
	} while (0)

#define ROTL64_32(_v)							\
	do {								\
		ulong t = (_v)[0];					\
		(_v)[0] = (_v)[1];					\
		(_v)[1] = t;						\
	} while (0)

#define XOR64(_v0, _v1)							\
	do {								\
		(_v0)[0] ^= (_v1)[0];					\
		(_v0)[1] ^= (_v1)[1];					\
	} while (0)

static void siphash_round(struct siphashctx *shc)
{
	ADD64(shc->v0, shc->v1);
	ADD64(shc->v2, shc->v3);
	ROTL64(shc->v1, 13);
	ROTL64(shc->v3, 16);
	XOR64(shc->v1, shc->v0);
	XOR64(shc->v3, shc->v2);
	ROTL64_32(shc->v0);
	ADD64(shc->v2, shc->v1);
	ADD64(shc->v0, shc->v3);
	ROTL64(shc->v1, 17);
	ROTL64(shc->v3, 21);
	XOR64(shc->v1, shc->v2);
	XOR64(shc->v3, shc->v0);
	ROTL64_32(shc->v2);
}

#undef ADD64
#undef XOR64
#undef ROTL64
#undef ROTL64_32


static int siphash_add_bytes(struct siphashctx *shc, const byte_t **bp,
			     ulong *amt)
{
	uint off = shc->nbytes % 8;
	uint toadd;
	uint i;

	if ( off == 0 && *amt >= 8 ) {
		/* fast path */
		shc->state[0] = (*bp)[0] |
				(*bp)[1] << 8 |
				(*bp)[2] << 16 |
			        (*bp)[3] << 24;
		shc->state[1] = (*bp)[4] |
				(*bp)[5] << 8 |
				(*bp)[6] << 16 |
			        (*bp)[7] << 24;
		shc->nbytes += 8;
		*amt -= 8;
		*bp += 8;
		return 1;
	} else {
		/* slow path */
		if ( off == 0 ) {
			shc->state[0] = 0;
			shc->state[1] = 0;
		}
		toadd = 8 - off;
		if ( toadd > *amt )
			toadd = *amt;
		for ( i = 0; i < toadd; ++i, ++*bp ) {
			if ( off < 4 ) {
				shc->state[0] |= **bp << (8 * off);
			} else {
				shc->state[1] |= **bp << (8 * (off - 4));
			}
		}
		shc->nbytes += toadd;
		*amt -= toadd;
		return shc->nbytes % 8 == 0;
	}
}


void siphash24_add(struct siphashctx *shc, const void *p, ulong len)
{
	const byte_t *bp = p;
	while ( len > 0 ) {
		if ( siphash_add_bytes(shc, &bp, &len) ) {
			shc->v3[0] ^= shc->state[0];
			shc->v3[1] ^= shc->state[1];
			siphash_round(shc);
			siphash_round(shc);
			shc->v0[0] ^= shc->state[0];
			shc->v0[1] ^= shc->state[1];
		}
	}
}


void siphash24_fini(struct siphashctx *shc, byte_t hash[8])
{
	if ( shc->nbytes % 8 == 0 ) {
		shc->state[0] = 0;
		shc->state[1] = 0;
	}
	shc->state[1] |= (shc->nbytes % 256) << 24;
	shc->v3[0] ^= shc->state[0];
	shc->v3[1] ^= shc->state[1];
	siphash_round(shc);
	siphash_round(shc);
	shc->v0[0] ^= shc->state[0];
	shc->v0[1] ^= shc->state[1];

	/* finalize hash */
	shc->v2[0] ^= 0xff;
	siphash_round(shc);
	siphash_round(shc);
	siphash_round(shc);
	siphash_round(shc);
	shc->state[0] = shc->v0[0] ^ shc->v1[0] ^ shc->v2[0] ^ shc->v3[0];
	shc->state[1] = shc->v0[1] ^ shc->v1[1] ^ shc->v2[1] ^ shc->v3[1];

	/* copy out result */
	hash[0] = shc->state[0] & 0xff;
	hash[1] = (shc->state[0] >> 8) & 0xff;
	hash[2] = (shc->state[0] >> 16) & 0xff;
	hash[3] = (shc->state[0] >> 24) & 0xff;
	hash[4] = shc->state[1] & 0xff;
	hash[5] = (shc->state[1] >> 8) & 0xff;
	hash[6] = (shc->state[1] >> 16) & 0xff;
	hash[7] = (shc->state[1] >> 24) & 0xff;

	/* clean up state for safety */
	memset(shc, 0, sizeof(*shc));
}


void siphash24(ulong key[4], const void *p, ulong len, byte_t hash[8])
{
	struct siphashctx shc;
	siphash_init(&shc, key);
	siphash24_add(&shc, p, len);
	siphash24_fini(&shc, hash);
}


void ht_sh24_init(struct ht_sh24_ctx *hsc, const void *k, ulong len)
{
	const byte_t *p = k;
	ulong i;
	memset(hsc, 0, sizeof(*hsc));
	for ( i = 0; i < len; ++i, ++p )
		hsc->key[(i / 4) % 4] ^= *p << (8 * (i % 4));
}


uint ht_sh24_shash(const void *sp, void *hscp)
{
	const char *s = sp;
	struct ht_sh24_ctx *hsc = hscp;
	byte_t hash[8];
	ulong len = 0;

	while ( *s++ != '\0' ) ++len;
	siphash24(hsc->key, sp, len, hash);
	return hash[0] | hash[1] << 8 | hash[2] << 16 | hash[3] << 24;
}


uint ht_sh24_rhash(const void *rp, void *hscp)
{
	const struct raw *r = rp;
	struct ht_sh24_ctx *hsc = hscp;
	byte_t hash[8];
	siphash24(hsc->key, r->data, r->len, hash);
	return hash[0] | hash[1] << 8 | hash[2] << 16 | hash[3] << 24;
}

