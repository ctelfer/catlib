#include <cat/cat.h>
#include <cat/str.h>
#include <cat/emit_format.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>


#ifndef va_copy
#ifndef __va_copy
#error "No va_copy() or __va_copy() macros!"
#else /* __va_copy */
#define va_copy(dst, src) __va_copy(dst, src)
#endif /* __va_copy */
#endif /* va_copy */


const signed char utf8_lentab[256] = { 
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6,-1,-1,
};


size_t str_copy(char *dst, const char *src, size_t dlen)
{
	const char *osrc = src;

	while ( dlen > 0 && *src != '\0' ) {
		*dst++ = *src++;
		--dlen;
	}

	if ( dlen == 0 ) {
		if ( src != osrc )
			*(dst - 1) = '\0';
	} else {
		*dst = '\0';
	}

	while ( *src++ != '\0' )
		;

	return src - osrc;
}


size_t str_cat(char *dst, const char *src, size_t dlen)
{
	size_t copylen;
	size_t dstoff;
	char *odst = dst;

	while ( dlen > 0 && *dst != '\0' ) {
		--dlen;
		++dst;
	}

	copylen = str_copy(dst, src, dlen);
	dstoff = dst - odst;

	abort_unless(copylen <= (size_t)~0 - dstoff);

	return copylen + dstoff;
}


int str_vfmt(char *buf, size_t len, const char *fmt, va_list ap)
{
	struct string_emitter se;
	int rlen;
	char tbuf[1] = { '\0' };

	abort_unless(fmt);
	if ( len <= 1 )
		buf = NULL;

	if ( buf )
		string_emitter_init(&se, buf, len);
	else
		string_emitter_init(&se, tbuf, 1);
	rlen = emit_vformat(&se.se_emitter, fmt, ap);
	string_emitter_terminate(&se);

	return rlen;
}


int str_fmt(char *buf, size_t len, const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = str_vfmt(buf, len, fmt, ap);
	va_end(ap);

	return rv;
}


char * str_findc(const char *sprm, const char *cprm)
{
	int clen, slen;
	int i;
	const uchar *s = (const uchar *)sprm;
	const uchar *c = (const uchar *)cprm;

	/* could do boyer-moore, simple is faster for 1 char */
	clen = utf8_validate_char((const char *)c, 0);
	abort_unless(clen > 0);

	while ( *s != '\0' ) {

		/* common case */
		if ( (clen == 1) && ((*s & 0x80) == 0) ) { 
			if ( *s == *c )
				return (char *)s;
			++s;
			continue;
		}

		/* multi-length utf8-char */
		slen = utf8_nbytes(*s);
		abort_unless(slen > 0);

		if ( slen == clen ) {
			for ( i = 0; i < slen ; ++i )
				if ( s[i] != c[i] )
					break;
			if ( i == slen )
				return (char *)s;
		}
		s += slen;
	}
	return NULL;
}


static int char_is_in(const uchar *c, int cnb, const uchar *set)
{
	int snb, i;

	while ( *set != '\0' ) {
		snb = utf8_nbytes(*set);
		abort_unless(snb > 0);
		if ( cnb == snb ) {
			for ( i = 0; i < snb ; ++i )
				if ( set[i] != c[i] )
					break;
			if ( i == snb )
				return 1;
		}
		set += snb;
	}

	return 0;
}


size_t str_span(const char *s, const char *accept)
{
	size_t off = 0;
	int len;

	while ( *s != '\0' ) {
		len = utf8_nbytes(*s);
		abort_unless(len > 0);
		if ( !char_is_in((uchar *)s, len, 
				 (uchar *)accept) )
			return off;
		s += len;
		off += len;
	}
	return off;
}


size_t str_cspan(const char *s, const char *reject)
{
	size_t off = 0;
	int len;
	while ( *s != '\0' ) {
		len = utf8_nbytes(*s);
		abort_unless(len > 0);
		if ( char_is_in((uchar *)s, len, 
				(uchar *)reject) )
			return off;
		s += len;
		off += len;
	}
	return off;
}


int utf8_find_nbytes(const uchar c)
{
	int hibit = 0x80;
	int firstz = 0;

	while ( c & hibit ) {
		++firstz;
		hibit >>= 1;
	}

	if ( firstz == 0 )
		return 1;
	else if ( firstz == 1 )
		return -1;
	else if ( firstz >= 7 )
		return -1;
	return firstz;
}


int utf8_validate_char(const char *strprm, size_t max)
{
	int i, nb;
	const uchar *str = (const uchar *)strprm;
	const uchar *ostr;

	if ( ((nb = utf8_nbytes(*str)) < 0) || ((max > 0) && (nb > max)) )
		return -1;
	ostr = str++;
	for ( i = 1 ; i < nb ; ++i, ++str )
		if ( (*str & 0xC0) != 0x80 )
			return -1;
	switch ( nb ) {
	case 2:
		if ( (ostr[0] & 0xFE) == 0xC0 )
			return -1;
		break;
	case 3:
		if ( (ostr[0] == 0xE0) && ((ostr[1] & 0xE0) == 0x80) )
			return -1;
		break;
	case 4:
		if ( (ostr[0] == 0xF0) && ((ostr[1] & 0xF0) == 0x80) )
			return -1;
		break;
	case 5:
		if ( (ostr[0] == 0xF8) && ((ostr[1] & 0xF8) == 0x80) )
			return -1;
		break;
	case 6:
		if ( (ostr[0] == 0xFC) && ((ostr[1] & 0xFC) == 0x80) )
			return -1;
		break;
	}

	abort_unless(nb > 0);
	return nb;
}


int utf8_validate(const char *str, size_t slen, const char **epos, int term)
{
	int len;

	while ( slen > 0 ) {
		if ( term && *str == '\0' )
			return 0;
		if ( (len = utf8_validate_char(str, slen)) < 0 ) {
			if ( epos )
				*epos = str;
			return -1;
		}
		str += len;
		slen -= len;
	}

	if ( epos )
		*epos = NULL;
	return 0;
}


size_t utf8_nchars(const char *str, size_t slen, int *maxlen)
{
	size_t nc = 0;
	int len;
	int max = 0;

	while ( slen > 0 ) {
		nc++;
		len = utf8_nbytes(*str);
		abort_unless(len > 0);
		if ( len > max )
			max = len;
		slen -= len;
		str += len;
	}

	if ( maxlen != NULL )
		*maxlen = max;
	return nc;
}


static struct utf8_chardesc { 
	int		hishift;
	ulong	mask;
} utf8_cdtab[7] = { 
	{ 0, 0 },	/* dummy */
	{ 0, 0x7F },
	{ 6, 0x1F },
	{ 12, 0x0F },
	{ 18, 0x07 },
	{ 24, 0x03 },
	{ 30, 0x01 },
};


int utf8_to_utf32(ulong *dst, size_t dlen, const char *sprm, size_t slen)
{
	int clen, i;
	ulong v;
	struct utf8_chardesc *cd;
	size_t nconv = 0;
	const uchar *src = (const uchar *)sprm;

	while ( slen > 0 ) {
		if ( (clen = utf8_validate_char((const char *)src, slen)) < 0 )
			return -1;
		if ( nconv < dlen ) {
			cd = &utf8_cdtab[clen];
			v = ((ulong)src[0] & cd->mask) << cd->hishift;
			for ( i = 1 ; i < clen ; ++i )
				v |= ((ulong)src[i] & 0x3F) << 
						 (6 * (clen - 1 - i));
			*dst++ = v;
		}
		++nconv;
		src += clen;
		slen -= clen;
	}

	return nconv;
}


int utf8_to_utf16(ushort *dst, size_t dlen, const char *sprm, size_t slen)
{
	int clen, i;
	ulong v;
	struct utf8_chardesc *cd;
	size_t nconv = 0;
	const uchar *src = (const uchar *)sprm;

	while ( slen > 0 ) {
		clen = utf8_validate_char((const char *)src, slen);
		if ( clen < 0 || clen > 3 )
			return -1;
		if ( nconv < dlen ) {
			cd = &utf8_cdtab[clen];
			v = ((ushort)src[0] & cd->mask) << cd->hishift;
			for ( i = 1 ; i < clen ; ++i )
				v |= ((ushort)src[i] & 0x3F) << 
						 (6 * (clen - 1 - i));

			/* Check whether we need to encode this as a */
			/* surrogate pair */
			if ( v > 0xFFFF ) {
				if ( v > 0x10FFFF )
					return -1;
				++nconv;
				if ( nconv < dlen ) {
					v -= 0x10000;
					*dst++ = 0xD800 | (v & 0xFFC00) >> 10;
					*dst++ = 0xDC00 | (v & 0x3FF);
				}
			} else {
				*dst++ = v;
			}
		}
		++nconv;
		src += clen;
		slen -= clen;
	}

	return nconv;
}


static int utf8_enc_len(ulong val)
{
	if ( val & 0x80000000 )
		return -1;
	if ( (val & 0xFFFFFF80) == 0 )	/* 7 bit ascii */
		return 1;
	if ( (val & 0xFFFFF800) == 0 )	/* 11 bits */
		return 2;
	if ( (val & 0xFFFF0000) == 0 )	/* 16 bits */
		return 3;
	if ( (val & 0xFFE00000) == 0 )	/* 21 bits */
		return 4;
	if ( (val & 0xFC000000) == 0 )	/* 26 bits */
		return 5;
	return 6;			/* 31 bits */
}


static void utf8_encode(uchar *dp, ulong v, int enclen)
{
	switch(enclen) {
		case 1: /* 7 bit ascii */
			*dp = v;
			break;
		case 2: /* lower 11 bits */
			*dp++ = 0xC0 | ((v & 0x000007C0) >> 6);
			*dp++ = 0x80 |  (v & 0x0000003F);
			break;
		case 3: /* lower 16 bits */
			*dp++ = 0xE0 | ((v & 0x0000F000) >> 12);
			*dp++ = 0x80 | ((v & 0x00000FC0) >> 6);
			*dp++ = 0x80 |  (v & 0x0000003F);
			break;
		case 4: /* lower 21 bits */
			*dp++ = 0xF0 | ((v & 0x001C0000) >> 18);
			*dp++ = 0x80 | ((v & 0x0003F000) >> 12);
			*dp++ = 0x80 | ((v & 0x00000FC0) >> 6);
			*dp++ = 0x80 |  (v & 0x0000003F);
			break;
		case 5: /* lower 26 bits */
			*dp++ = 0xF8 | ((v & 0x03000000) >> 24);
			*dp++ = 0x80 | ((v & 0x00FC0000) >> 18);
			*dp++ = 0x80 | ((v & 0x0003F000) >> 12);
			*dp++ = 0x80 | ((v & 0x00000FC0) >> 6);
			*dp++ = 0x80 |  (v & 0x0000003F);
			break;
		case 6: /* lower 31 bits */
			*dp++ = 0xFC | ((v & 0x40000000) >> 30);
			*dp++ = 0x80 | ((v & 0x3F000000) >> 24);
			*dp++ = 0x80 | ((v & 0x00FC0000) >> 18);
			*dp++ = 0x80 | ((v & 0x0003F000) >> 12);
			*dp++ = 0x80 | ((v & 0x00000FC0) >> 6);
			*dp++ = 0x80 |  (v & 0x0000003F);
			break;
		default:
			abort_unless(0);
	}
}


int utf32_to_utf8(char *dprm, size_t dlen, const ulong *src, size_t slen)
{
	size_t tlen = 0;
	int enclen;
	ulong v;
	uchar *dst = (uchar *)dprm;
	abort_unless(src);

	if ( slen >= INT_MAX )
		return -1;

	while ( slen > 0 ) { 
		if ( (enclen = utf8_enc_len(v = *src)) < 0 )
			return -1;
		abort_unless(enclen >= 1 && enclen <= 6);
		if ( dst != NULL && dlen >= enclen ) {
			utf8_encode(dst, v, enclen);
			dst += enclen;
			dlen -= enclen;
		}
		if ( INT_MAX - tlen < enclen )
			return -1;
		tlen += enclen;
		src++;
		slen--;
	}

	return tlen;
}


int utf16_to_utf8(char *dprm, size_t dlen, const ushort *src, size_t slen)
{
	size_t tlen = 0;
	int enclen;
	ulong v;
	uchar *dst = (uchar *)dprm;
	abort_unless(src);

	if ( slen >= INT_MAX )
		return -1;

	while ( slen > 0 ) { 

		/* Check for surrogate pairs */
		v = *src;
		if ( (v >= 0xDC00) && (v <= 0xDFFF) )
			return -1;
		if ( (v & 0xFC00) == 0xD800 ) {
			if ( (slen < 2) || ((src[1] & 0xFC00) != 0xDC00) )
				return -1;
			v = ((src[0] & 0x3FF) << 10) | (src[1] & 0x3FF);
			src += 2;
			slen -= 2;;
		} else {
			++src;
			--slen;
		}

		if ( (enclen = utf8_enc_len(v)) < 0 )
			return -1;
		abort_unless(enclen >= 1 && enclen <= 3);
		if ( dst != NULL && dlen >= enclen ) {
			utf8_encode(dst, v, enclen);
			dst += enclen;
			dlen -= enclen;
		}
		if ( INT_MAX - tlen < enclen )
			return -1;
		tlen += enclen;
	}

	return tlen;
}


static char * utf8_skip_help(char *str, size_t nchar, int tc)
{
	int clen;

	abort_unless(str);
	while ( nchar > 0 ) {
		clen = utf8_nbytes(*str);
		if ( (clen < 0) || (tc && (clen == 1) && (*str == '\0')) )
			return NULL;
		str += clen;
		--nchar;
	}

	return str;
}


char * utf8_skip(char *str, size_t nchar)
{
	return utf8_skip_help(str, nchar, 0);
}


char * utf8_skip_tck(char *str, size_t nchar)
{
	return utf8_skip_help(str, nchar, 1);
}
