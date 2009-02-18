/*
 * pack.c -- Functions to pack values into byte arrays
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004, 2005 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/pack.h>
#if CAT_USE_STDLIB
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */

typedef unsigned char	byte;
typedef unsigned short	half;
typedef unsigned long	word;
typedef signed char	sbyte;
typedef signed short	shalf;
typedef signed long	sword;


#if PSIZ_MAX % PSIZ_BYTE == 0 && PSIZ_BYTE > 1
#define PSIZ_BYTE_OVERFLOW	((PSIZ_MAX / PSIZ_BYTE) + 1)
#else
#define PSIZ_BYTE_OVERFLOW	(PSIZ_MAX / PSIZ_BYTE)
#endif

#if PSIZ_MAX % PSIZ_HALF == 0
#define PSIZ_HALF_OVERFLOW	((PSIZ_MAX / PSIZ_HALF) + 1)
#else
#define PSIZ_HALF_OVERFLOW	(PSIZ_MAX / PSIZ_HALF)
#endif

#if PSIZ_MAX % PSIZ_WORD == 0
#define PSIZ_WORD_OVERFLOW	((PSIZ_MAX / PSIZ_WORD) + 1)
#else
#define PSIZ_WORD_OVERFLOW	(PSIZ_MAX / PSIZ_WORD)
#endif

#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG

typedef unsigned long long	jumbo;
typedef signed long long	sjumbo;

#if PSIZ_MAX % PSIZ_JUMBO == 0
#define PSIZ_JUMBO_OVERFLOW	((PSIZ_MAX / PSIZ_JUMBO) + 1)
#else
#define PSIZ_JUMBO_OVERFLOW	(PSIZ_MAX / PSIZ_JUMBO)
#endif

#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */


static byte *pack_half(byte *p, word val, int bigendian)
{
	if ( bigendian ) {
		*p++ = (val >> 8) & 0xFF;
		*p++ = val & 0xFF;
	} else {
		*p++ = val & 0xFF;
		*p++ = (val >> 8) & 0xFF;
	}
	return p;
}


static byte *pack_word(byte *p, word val, int bigendian)
{
	if ( bigendian ) {
		*p++ = (val >> 24) & 0xFF;
		*p++ = (val >> 16) & 0xFF;
		*p++ = (val >> 8) & 0xFF;
		*p++ = val & 0xFF;
	} else {
		*p++ = val & 0xFF;
		*p++ = (val >> 8) & 0xFF;
		*p++ = (val >> 16) & 0xFF;
		*p++ = (val >> 24) & 0xFF;
	}
	return p;
}


static byte *unpack_half(byte *p, void *hp, int bigendian, int issigned)
{
	half u = 0;
	shalf s = 0;

	if ( bigendian ) {
		if ( issigned ) {
			s = (*p++ & 0xFF) << 8;
			s |= *p++ & 0xFF;
			s |= -(s & 0x8000);
			*(shalf *)hp = s;
		} else {
			u = (*p++ & 0xFF) << 8;
			u |= *p++;
			*(half *)hp = u;
		}
	} else {
		if ( issigned ) {
			s = (*p++ & 0xFF);
			s |= (*p++ & 0xFF) << 8;
			s |= -(s & 0x8000);
			*(shalf *)hp = s;
		} else {
			u |= (*p++ & 0xFF);
			u = (*p++ & 0xFF) << 8;
			*(half *)hp = u;
		}
	}
	return p;
}


static byte *unpack_word(byte *p, void *wp, int bigendian, int issigned)
{
	word u = 0;
	sword s = 0;

	if ( bigendian ) {
		if ( issigned ) {
			s  = ((sword)*p++ & 0xFF) << 24;
			s |= ((sword)*p++ & 0xFF) << 16;
			s |= ((sword)*p++ & 0xFF) << 8;
			s |= ((sword)*p++ & 0xFF);
			s |= -(s & 0x80000000);
			*(sword *)wp = s;
		} else {
			u  = ((word)*p++ & 0xFF) << 24;
			u |= ((word)*p++ & 0xFF) << 16;
			u |= ((word)*p++ & 0xFF) << 8;
			u |= ((word)*p++ & 0xFF);
			*(word *)wp = u;
		}
	} else {
		if ( issigned ) {
			s  = ((sword)*p++ & 0xFF);
			s |= ((sword)*p++ & 0xFF) << 8;
			s |= ((sword)*p++  & 0xFF)<< 16;
			s |= ((sword)*p++  & 0xFF)<< 24;
			s |= -(s & 0x80000000);
			*(sword *)wp = s;
		} else {
			u  = ((word)*p++ & 0xFF);
			u |= ((word)*p++ & 0xFF) << 8;
			u |= ((word)*p++ & 0xFF) << 16;
			u |= ((word)*p++ & 0xFF) << 24;
			*(word *)wp = u;
		}
	}
	return p;
}


#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG

static byte *pack_jumbo(byte *p, jumbo jval, int bigendian)
{
	if ( bigendian ) {
		*p++ = (jval >> 56) & 0xFF;
		*p++ = (jval >> 48) & 0xFF;
		*p++ = (jval >> 40) & 0xFF;
		*p++ = (jval >> 32) & 0xFF;
		*p++ = (jval >> 24) & 0xFF;
		*p++ = (jval >> 16) & 0xFF;
		*p++ = (jval >> 8) & 0xFF;
		*p++ = jval & 0xFF;
	} else {
		*p++ = jval & 0xFF;
		*p++ = (jval >> 8) & 0xFF;
		*p++ = (jval >> 16) & 0xFF;
		*p++ = (jval >> 24) & 0xFF;
		*p++ = (jval >> 32) & 0xFF;
		*p++ = (jval >> 40) & 0xFF;
		*p++ = (jval >> 48) & 0xFF;
		*p++ = (jval >> 56) & 0xFF;
	}
	return p;
}


static byte *unpack_jumbo(byte *p, void *jp, int bigendian, int issigned)
{
	jumbo u = 0;
	sjumbo s = 0;

	if ( bigendian ) {
		if ( issigned ) {
			s  = ((sjumbo)*p++ & 0xFF) << 56;
			s |= ((sjumbo)*p++ & 0xFF) << 48;
			s |= ((sjumbo)*p++ & 0xFF) << 40;
			s |= ((sjumbo)*p++ & 0xFF) << 32;
			s |= ((sjumbo)*p++ & 0xFF) << 24;
			s |= ((sjumbo)*p++ & 0xFF) << 16;
			s |= ((sjumbo)*p++ & 0xFF) << 8;
			s |= ((sjumbo)*p++ & 0xFF);
			s |= -(s & 0x8000000000000000ll);
			*(sjumbo *)jp = s;
		} else {
			u  = ((jumbo)*p++ & 0xFF) << 56;
			u |= ((jumbo)*p++ & 0xFF) << 48;
			u |= ((jumbo)*p++ & 0xFF) << 40;
			u |= ((jumbo)*p++ & 0xFF) << 32;
			u |= ((jumbo)*p++ & 0xFF) << 24;
			u |= ((jumbo)*p++ & 0xFF) << 16;
			u |= ((jumbo)*p++ & 0xFF) << 8;
			u |= ((jumbo)*p++ & 0xFF);
			*(jumbo *)jp = u;
		}
	} else {
		if ( issigned ) {
			s  = ((sjumbo)*p++ & 0xFF);
			s |= ((sjumbo)*p++ & 0xFF) << 8;
			s |= ((sjumbo)*p++ & 0xFF) << 16;
			s |= ((sjumbo)*p++ & 0xFF) << 24;
			s |= ((sjumbo)*p++ & 0xFF) << 32;
			s |= ((sjumbo)*p++ & 0xFF) << 40;
			s |= ((sjumbo)*p++ & 0xFF) << 48;
			s |= ((sjumbo)*p++ & 0xFF) << 56;
			s |= -(s & 0x8000000000000000ll);
			*(sjumbo *)jp = s;
		} else {
			u  = ((jumbo)*p++ & 0xFF);
			u |= ((jumbo)*p++ & 0xFF) << 8;
			u |= ((jumbo)*p++ & 0xFF) << 16;
			u |= ((jumbo)*p++ & 0xFF) << 24;
			u |= ((jumbo)*p++ & 0xFF) << 32;
			u |= ((jumbo)*p++ & 0xFF) << 40;
			u |= ((jumbo)*p++ & 0xFF) << 48;
			u |= ((jumbo)*p++ & 0xFF) << 56;
			*(jumbo *)jp = u;
		}
	}
	return p;
}

#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */


size_t pack(void *buf, size_t blen, const char *fmt, ...)
{
	va_list ap;
	int bigendian = 1;
	size_t nreps, i, left;
	size_t len;
	byte *p, *bytep;
	half *halfp;
	word *wordp;
	struct raw *raw;
	char *cp;
#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG
	jumbo *jumbop;
#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */

	abort_unless(buf);
	abort_unless(fmt);

	if ( blen == 0 )
		return 0;
	if ( blen > PSIZ_MAX )
		blen = PSIZ_MAX;
	left = blen;

	va_start(ap, fmt);

	p = buf;

	while ( *fmt ) {
		if ( toupper(*fmt) == 'E' ) {
			bigendian = *fmt == 'E';
			continue;
		}

		nreps = 0;
		if ( isdigit(*fmt) ) {
			nreps = strtoul(fmt, &cp, 10);
			if ( nreps <= 0 )
				goto error;
			fmt = cp;
		}

		switch ( *fmt ) {
		case 'b':
		case 'B':
			if ( nreps == 0 ) {
				if ( PSIZ_BYTE > left )
					goto error;
				*p++ = va_arg(ap, int) & 0xFF;
				left -= PSIZ_BYTE;
			} else {
				if ( (nreps > PSIZ_BYTE_OVERFLOW) ||
				     (PSIZ_BYTE * nreps > left) )
					goto error;
				bytep = va_arg(ap, byte *);
				abort_unless(bytep);
				for ( i = 0 ; i < nreps ; ++i )
					*p++ = (bytep[i] & 0xFF);
				left -= PSIZ_BYTE * nreps;
			}
			break;

		case 'h':
		case 'H':
			if ( ! nreps ) {
				if ( PSIZ_HALF > left )
					goto error;
				p = pack_half(p, (half)va_arg(ap, int),
					      bigendian);
				left -= PSIZ_HALF;
			} else {
				if ( (nreps > PSIZ_HALF_OVERFLOW) ||
				     (PSIZ_HALF * nreps > left) )
					goto error;
				halfp = va_arg(ap, half *);
				abort_unless(halfp);
				for ( i = 0 ; i < nreps ; ++i )
					p = pack_half(p, halfp[i], bigendian);
				left -= PSIZ_HALF * nreps;
			}
			break;

		case 'w':
		case 'W':
			if ( ! nreps ) {
				if ( PSIZ_WORD > left )
					goto error;
				p = pack_word(p, va_arg(ap, word), bigendian);
				left -= PSIZ_WORD;
			} else {
				if ( (nreps > PSIZ_WORD_OVERFLOW) ||
				     (PSIZ_WORD * nreps > left) )
					goto error;
				wordp = va_arg(ap, word *);
				abort_unless(wordp);
				for ( i = 0 ; i < nreps ; ++i )
					p = pack_word(p, wordp[i], bigendian);
				left -= PSIZ_WORD * nreps;
			}
			break;

#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG
		case 'j':
		case 'J':
			if ( ! nreps ) {
				if ( PSIZ_JUMBO > left )
					goto error;
				p = pack_jumbo(p, va_arg(ap,jumbo), bigendian);
				left -= PSIZ_JUMBO;
			} else {
				if ( (nreps > PSIZ_JUMBO_OVERFLOW) ||
				     (PSIZ_JUMBO * nreps > left) )
					goto error;
				jumbop = va_arg(ap, jumbo *);
				abort_unless(jumbop);
				for ( i = 0 ; i < nreps ; ++i )
					p = pack_jumbo(p, jumbop[i], bigendian);
				left -= PSIZ_JUMBO * nreps;
			}
			break;
#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */

		case 'r':
			if ( ! nreps ) 
				nreps = 1;
			raw = va_arg(ap, struct raw *);
			abort_unless(raw);
			for ( i = 0 ; i < nreps ; ++i ) {
				len = raw[i].len;
				if ( (len > PSIZ_MAX) || (len > left) )
					goto error;
				memmove(p, raw[i].data, len);
				p += len;
				left -= len;
			}
			break;

		default:
			goto error;
		}

		++fmt;
	}

	va_end(ap);
	return blen - left;

error:
	va_end(ap);
	return 0;
}


size_t unpack(void *buf, size_t blen, const char *fmt, ...)
{
	va_list ap;
	byte *p, *bytep;
	sbyte *sbytep;
	int bigendian = 1, issigned;
	size_t pulled, nreps, i;
	size_t osiz, isiz;
	char *cp, *inp;
	byte *(*unpackf)(byte *, void *, int, int);

	abort_unless(buf);
	abort_unless(fmt);
	abort_unless(blen <= PSIZ_MAX);

	if ( blen == 0 )
		return 0;

	p = (byte *)buf;
	pulled = 0;

	va_start(ap, fmt);

	while ( *fmt ) {
		if ( toupper(*fmt) == 'E' ) {
			bigendian = *fmt == 'E';
			continue;
		}

		nreps = 1;
		if ( isdigit(*fmt) ) {
			nreps = strtoul(fmt, &cp, 10);
			if ( nreps <= 0 )
				goto error;
			fmt = cp;
		}

		switch ( *fmt ) {
		case 'b':
			if ( (nreps > PSIZ_BYTE_OVERFLOW) || 
			     (blen < PSIZ_BYTE * nreps) || 
			     (PSIZ_BYTE * nreps > blen - pulled) )
				goto error;
			bytep = va_arg(ap, byte *);
			abort_unless(bytep);
			for ( i = 0 ; i < nreps ; ++i )
				*bytep++ = *p++;
			pulled += PSIZ_BYTE * nreps;
			nreps = 0;
			break;

		case 'B':
			if ( (nreps > PSIZ_BYTE_OVERFLOW) || 
			     (blen < PSIZ_BYTE * nreps) || 
			     (PSIZ_BYTE * nreps > blen - pulled) )
				goto error;
			sbytep = va_arg(ap, sbyte *);
			abort_unless(sbytep);
			for ( i = 0 ; i < nreps ; ++i ) {
				sbyte sb = *p++ & 0xFF;
				sb |= -(sb & 0x80);
				*sbytep++ = sb;
			}
			pulled += PSIZ_BYTE * nreps;
			nreps = 0;
			break;

		case 'h':
		case 'H':
			isiz = PSIZ_HALF;
			osiz = PSIZ_HALF_OVERFLOW;
			unpackf = &unpack_half;
			issigned = *fmt == 'H';
			inp = (char *)va_arg(ap, half *);
			break;

		case 'w':
		case 'W':
			isiz = PSIZ_WORD;
			osiz = PSIZ_WORD_OVERFLOW;
			unpackf = &unpack_word;
			issigned = *fmt == 'W';
			inp = (char *)va_arg(ap, word *);
			break;

#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG
		case 'j':
		case 'J':
			isiz = PSIZ_JUMBO;
			osiz = PSIZ_JUMBO_OVERFLOW;
			unpackf = &unpack_jumbo;
			issigned = *fmt == 'W';
			inp = (char *)va_arg(ap, jumbo *);
			break;
#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */

		default:
			goto error;
		}

		if ( nreps > 0 ) {
			if ( (nreps > osiz) || (blen < isiz * nreps) || 
			     (isiz * nreps > blen - pulled) )
				goto error;
			abort_unless(inp);
			for ( i = 0 ; i < nreps ; ++i ) {
				p = (*unpackf)(p, inp, bigendian, issigned);
				inp += isiz;
			}
			pulled += isiz * nreps;
		}

		++fmt;
	}

	va_end(ap);
	return pulled;
error:
	va_end(ap);
	return 0;
}


size_t packlen(const char *fmt, ...)
{
	va_list ap;
	size_t max, i, nreps;
	struct raw *raw;
	char *cp;

	abort_unless(fmt);

	va_start(ap, fmt);

	max = PSIZ_MAX;

	while ( *fmt ) {
		nreps = 1;

		if ( isdigit(*fmt) ) {
			nreps = strtoul(fmt, &cp, 10);
			if (nreps <= 0)
				goto error;
			fmt = cp;
		}

		switch ( *fmt ) {

		case 'e':
		case 'E':
			break;

		case 'b':
		case 'B':
			if ( (nreps > PSIZ_BYTE_OVERFLOW) ||
			     (nreps * PSIZ_BYTE > max) )
				goto error;
			max -= PSIZ_BYTE * nreps;
			break;

		case 'h':
		case 'H':
			if ( (nreps > PSIZ_HALF_OVERFLOW) ||
			     (nreps * PSIZ_HALF > max) )
				goto error;
			max -= PSIZ_HALF * nreps;
			break;

		case 'w':
		case 'W':
			if ( (nreps > PSIZ_WORD_OVERFLOW) ||
			     (nreps * PSIZ_WORD > max) )
				goto error;
			max -= PSIZ_WORD * nreps;
			break;

#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG
		case 'j':
		case 'J':
			if ( (nreps > PSIZ_JUMBO_OVERFLOW) ||
			     (nreps * PSIZ_JUMBO > max) )
				goto error;
			max -= PSIZ_JUMBO * nreps;
			break;
#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */

		case 'r':
			raw = va_arg(ap, struct raw *);
			abort_unless(raw);
			for ( i = 0 ; i < nreps ; ++i ) {
				if ( raw[i].len > max )
					goto error;
				max -= raw[i].len;
			}
			break;

		default:
			goto error;
		}

		++fmt;
	}

	va_end(ap);
	return PSIZ_MAX - max;
error:
	va_end(ap);
	return 0;
}
