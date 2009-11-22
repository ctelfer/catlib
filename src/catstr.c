/*
 * catstr.c -- application level strings
 *
 * by Christopher Adam Telfer
 *  
 * Copyright 2007, 2008, 2009 See accompanying license
 *  
 */

#include <cat/catstr.h>
#include <cat/str.h>
#include <cat/grow.h>
#include <cat/match.h>

/* for memmove and strlen */
#if CAT_USE_STDLIB
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdio.h>
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


static struct memmgr *cs_mmp = &stdmm;

#define CKCS(cs)							\
	if (!cs || !cs->cs_data ||					\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen > cs->cs_size))	\
		return CS_ERROR;

#define CKCSP(cs)							\
	if (!cs || !cs->cs_data || 					\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen > cs->cs_size))	\
		return NULL;

#define CKCSI(cs)							\
	if (!cs || !cs->cs_data || 					\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen > cs->cs_size))	\
		return -1;

#define CKCSN(cs)							\
	if (!cs || !cs->cs_data ||					\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen > cs->cs_size))	\
		return;


void cs_init(struct catstr *cs, char *data, size_t size, int data_is_str)
{
	size_t dlen = 0;

	if ( data_is_str && data )
		dlen = strlen(data);
	if ( size > CS_MAXLEN || size < 1 || dlen > size - 1) {
		cs->cs_size = 0;
		cs->cs_dynamic = 0;
		cs->cs_data = NULL;
		return;
	}

	cs->cs_size = size - 1;
	cs->cs_data = data;
	cs->cs_dlen = dlen;
	cs->cs_data[dlen] = '\0';
}


void cs_clear(struct catstr *cs)
{
	CKCSN(cs);
	cs->cs_dlen = 0;
	cs->cs_data[0] = '\0';
}


size_t cs_set_cstr(struct catstr *cs, const char *cstr)
{
	size_t cstrlen;

	abort_unless(cstr);
	CKCS(cs);

	cstrlen = strlen(cstr);
	if ( cstrlen > CS_MAXLEN || cstrlen > cs->cs_size )
		return CS_ERROR;

	cs->cs_dlen = cstrlen;
	memmove(cs->cs_data, cstr, cstrlen);
	cs->cs_data[cstrlen] = '\0';
	return cstrlen;
}


size_t cs_trunc_d(struct catstr *cs, size_t len)
{
	CKCS(cs);

	if ( len < cs->cs_dlen ) {
		cs->cs_dlen = len;
		cs->cs_data[len] = '\0';
		return len;
	} else {
		return cs->cs_dlen;
	}
}


size_t cs_concat_d(struct catstr *dst, const struct catstr *src)
{
	size_t tomove, tlen;

	CKCS(dst);
	CKCS(src);
	if ( CS_MAXLEN - dst->cs_dlen > src->cs_dlen )
		return CS_ERROR;

	tomove = dst->cs_size - dst->cs_dlen;
	if ( tomove > src->cs_dlen )
		tomove = src->cs_dlen;
	tlen = tomove + dst->cs_dlen;

	memmove(dst->cs_data + dst->cs_dlen, src->cs_data, tomove);
	dst->cs_dlen += tomove;
	dst->cs_data[dst->cs_dlen] = '\0';

	return tlen;
}


size_t cs_copy_d(struct catstr *dst, struct catstr *src)
{
	size_t tocopy;
	CKCS(dst);
	CKCS(src);

	tocopy = src->cs_dlen;
	if ( tocopy >= dst->cs_size )
		tocopy = dst->cs_size - 1;
	memmove(dst->cs_data, src->cs_data, tocopy);
	dst->cs_data[tocopy] = '\0';
	dst->cs_dlen = tocopy;
	return tocopy;
}


size_t cs_format_d(struct catstr *dst, const char *fmt, ...)
{
	va_list ap;
	int rv;
	abort_unless(fmt);
	va_start(ap, fmt);
	rv = str_vfmt(dst->cs_data, dst->cs_size, fmt, ap);
	va_end(ap);
	if ( rv < 0 ) {
		dst->cs_dlen = 0;
		dst->cs_data[0] = '\0';
		return CS_ERROR;
	}
	dst->cs_dlen = rv;
	if ( rv > dst->cs_size )
		dst->cs_dlen = dst->cs_size;
	return (size_t)rv;
}


size_t cs_find_cc(const struct catstr *cs, char ch)
{
	const char *cp;
	const char *end;
	size_t off = 0;
	CKCS(cs);

	cp = cs->cs_data;
	end = cp + cs->cs_dlen;
	while ( cp < end && *cp != ch ) {
		++cp;
		++off;
	}

	if ( cp == end )
		return CS_NOTFOUND;
	else
		return off;
}


size_t cs_span_cc(const struct catstr *cs, const char *accept)
{
	const char *cp;
	const char *ap;
	const char *end;
	size_t off = 0;
	CKCS(cs);

	cp = cs->cs_data;
	end = cp + cs->cs_dlen;
	while ( cp < end ) {
		for ( ap = (const char *)accept ; *ap != '\0' ; ++ap )
			if ( *cp == *ap )
				break;
		if ( *ap == '\0' )
			break;
		++cp;
		++off;
	}

	return off;
}


size_t cs_cspan_cc(const struct catstr *cs, const char *reject)
{
	const char *cp;
	const char *rp;
	const char *end;
	size_t off = 0;
	CKCS(cs);

	cp = cs->cs_data;
	end = cp + cs->cs_dlen;
	while ( cp < end ) {
		for ( rp = (const char *)reject ; *rp != '\0' ; ++rp )
			if ( *cp == *rp )
				goto done;
		++cp;
		++off;
	}
done:
	return off;
}


size_t cs_find_uc(const struct catstr *cs, const char *utf8ch)
{
	size_t off = 0, dlen;
	int scs, fcs, i;
	const char *fcp = (const char *)utf8ch;
	const char *scp;

	CKCS(cs);
	abort_unless(utf8ch);
	scp = (const char *)cs->cs_data;
	dlen = cs->cs_dlen;

	if ( (fcs = utf8_nbytes(*fcp)) < 0 )
		return CS_ERROR;
	while ( off < dlen ) {
		if ( (scs = utf8_nbytes(scp[off])) < 0 )
			return CS_ERROR;
		if ( fcs != scs ) {
			off += scs;
			continue;
		}
		for ( i = 0 ; i < scs ; ++i )
			if ( scp[off + i] != fcp[i] )
				break;
		if ( i == scs )
			off += scs;
		else
			break;
	}

	if ( off == dlen )
		return CS_NOTFOUND;
	else
		return off;
}


static int utf8match(const char *c1, const char *c2)
{
	int c1b, c2b;
	abort_unless(c1 && c2);

	c1b = utf8_nbytes(*c1);
	c2b = utf8_nbytes(*c2);

	if ( c1b == CS_ERROR || c2b == CS_ERROR )
		return -1;

	while ( c1b > 0 ) {
		if ( *c1 != *c2 )
			return c2b;
		--c1b;
		++c1;
		++c2;
	}

	return 0;
}



size_t cs_span_uc(const struct catstr *cs, const char *utf8ch, int nc)
{
	size_t off = 0, dlen, u8off;
	int i, mr, uclen;
	const char *fcp = (const char *)utf8ch;
	const char *scp;

	CKCS(cs);
	abort_unless(utf8ch && nc > 0);
	scp = (const char *)cs->cs_data;
	dlen = cs->cs_dlen;

	while ( off < dlen ) {
		for ( u8off = 0, i = 0 ; i < nc ; ++i ) {
			mr = utf8match(&scp[off], &fcp[u8off]);
			if ( mr < 0 )
				return CS_ERROR;
			if ( mr == 0 )
				break;
			u8off += mr;
		}

		if ( i == nc )
			break;
		uclen = utf8_nbytes(scp[off]);
		if ( uclen < 0 )
			return CS_ERROR;
		off += (size_t)uclen;
	}

	return off;
}


size_t cs_cspan_uc(const struct catstr *cs, const char *utf8ch, int nc)
{
	size_t off = 0, dlen, u8off;
	int i, mr;
	const char *fcp = (const char *)utf8ch;
	const char *scp;

	CKCS(cs);
	abort_unless(utf8ch && nc > 0);
	scp = (const char *)cs->cs_data;
	dlen = cs->cs_dlen;

	while ( off < dlen ) {
		for ( u8off = 0, i = 0 ; i < nc ; ++i ) {
			mr = utf8match(&scp[off], &fcp[u8off]);
			if ( mr < 0 )
				return CS_ERROR;
			if ( mr == 0 )
				goto out;
			u8off += (size_t)mr;
		}
		off += utf8_nbytes(scp[off]);
	}
out:

	return off;
}



size_t cs_find(const struct catstr *findin, const struct catstr *find)
{
	struct raw praw;

	CKCS(findin);
	CKCS(find);

	praw.len = find->cs_dlen;
	praw.data = (char *)find->cs_data;
	return cs_find_raw(findin, &praw);
}


size_t cs_find_str(const struct catstr *findin, const char *s)
{
	struct raw praw;

	CKCS(findin);

	praw.len = strlen(s);
	praw.data = (char *)s;
	return cs_find_raw(findin, &praw);
}


size_t cs_find_raw(const struct catstr *findin, const struct raw *r)
{
	struct bmpat bmp;
	ulong off;
	int rv;
	struct raw rstr;

	CKCS(findin);

	/* TODO: fix bm_* so it uses size_t */
	abort_unless(r->len <= (ulong)~0);
	bm_pinit_lite(&bmp, (struct raw *)r);

	rstr.len = findin->cs_dlen;
	rstr.data = (char *)findin->cs_data;
	rv = bm_match(&rstr, &bmp, &off);

	if ( rv < 0 )
		return CS_NOTFOUND;
	else
		return off;
}


struct catstr *cs_copy_from_chars(const char *s)
{
	size_t slen;
	struct catstr *cs;

	slen = strlen(s);

	if ( (cs = cs_alloc(slen)) == NULL )
		return NULL;
	memmove(cs->cs_data, s, slen);
	cs->cs_dlen = slen;
	cs->cs_data[slen] = '\0';

	return cs;
}


struct catstr *cs_format(const char *fmt, ...)
{
	va_list ap;
	int rv, rv2;
	struct catstr *cs;
	char dummy[1];

	abort_unless(fmt);

	va_start(ap, fmt);
	rv = str_vfmt(dummy, sizeof(dummy), fmt, ap);
	va_end(ap);

	if ( (rv < 0) || (rv + 1 <= 0) )
		return NULL;
	if ( (cs = cs_alloc(rv)) == NULL )
		return NULL;
	cs->cs_dlen = rv;
	va_start(ap, fmt);
	rv2 = str_vfmt((char *)cs->cs_data, rv+1, fmt, ap);
	va_end(ap);
	abort_unless(rv == rv2);

	return cs;
}


int cs_concat(struct catstr *dst, struct catstr *src)
{
	size_t newlen;

	CKCSI(dst);
	CKCSI(src);

	if ( CS_MAXLEN - dst->cs_dlen > src->cs_dlen )
		return -1;
	newlen = dst->cs_dlen + src->cs_dlen;
	if ( cs_grow(dst, newlen) < 0 )
		return -1;
	memmove(dst->cs_data + dst->cs_dlen, src->cs_data, dst->cs_dlen);
	dst->cs_dlen = newlen;
	dst->cs_data[newlen] = '\0';

	return 0;
}


int cs_grow(struct catstr *cs, size_t minlen)
{
	byte_t *csp;
	size_t tlen, olen;
	CKCSI(cs);
	abort_unless(minlen <= CS_MAXLEN);
	abort_unless(cs->cs_dynamic);

	csp = (byte_t *)&cs->cs_data;
	tlen = olen = cs_alloc_size(cs->cs_size);
	if ( mm_grow(cs_mmp, &csp, &tlen, cs_alloc_size(minlen)) < 0 )
		return -1;
	abort_unless(tlen >= olen);

	cs->cs_data = csp;
	cs->cs_size = tlen - 1;

	return 0;
}


int cs_addch(struct catstr *cs, char ch)
{
	CKCSI(cs);
	if ( cs->cs_dlen >= cs->cs_size ) {
		if ( cs->cs_dynamic ) {
			if ( cs_grow(cs, cs->cs_dlen + 1) < 0 )
				return -1;
		} else {
			return -1;
		}
	}
	cs->cs_data[cs->cs_dlen++] = ch;
	cs->cs_data[cs->cs_dlen] = '\0';
	return 0;
}


struct catstr *cs_substr(const struct catstr *orig, size_t off, size_t len)
{
	struct catstr *cs;
	size_t remaining;
	CKCSP(orig);

	/* allocate empty string */
	if ( off >= orig->cs_dlen )
		return cs_alloc(0);

	remaining = orig->cs_dlen - off;
	if ( remaining > len )
		len = remaining;

	if ( (cs = cs_alloc(len)) == NULL )
		return NULL;
	cs->cs_dlen = len;
	memmove(cs->cs_data, orig->cs_data + off, len);
	cs->cs_data[len] = '\0';

	return cs;
}


size_t cs_rev_off(const struct catstr *cs, size_t roff)
{
	CKCS(cs);

	if ( roff > cs->cs_dlen )
		roff = cs->cs_dlen;

	return cs->cs_dlen - roff;
}


#if CAT_USE_STDLIB
int cs_fd_readline(int fd, struct catstr **csp)
{
	struct catstr *cs;
	char *p;
	int eol = 0, rv;
	size_t sofar = 0;

	abort_unless(csp);

	if ( (cs = cs_alloc(128)) == NULL )
		return -1;

	while ( !eol ) {
		p = cs->cs_data + sofar;
		if ( (rv = read(fd, p, 1)) < 1 ) {
			if ( rv == 0 ) {
				eol = 1;
			} else {
				if ( errno == EINTR )
					continue;
				cs_free(cs);
				*csp = NULL;
				return -1;
			}
		} else if ( *p == '\n' ) {
			++sofar;
			eol = 1;
		} else {
			++sofar;
			if ( cs_isfull(cs) ) {
				if ( cs_grow(cs, cs->cs_size * 2) < 0 ) {
					cs_free(cs);
					*csp = NULL;
					return -1;
				}
			}
			abort_unless(cs);
		}
	}

	cs->cs_dlen = sofar;
	cs->cs_data[sofar] = '\0';
	*csp = cs;

	return 0;
}


int cs_file_readline(FILE *file, struct catstr **csp)
{
	struct catstr *cs;
	char *p, *trav;
	int eol = 0;
	size_t sofar = 0, avail;

	abort_unless(csp);

	if ( (cs = cs_alloc(128)) == NULL )
		return -1;

	while ( !eol ) {
		p = cs->cs_data + sofar;
		avail = cs->cs_size - sofar;
		abort_unless(avail > 2);
		if ( fgets((char *)p, avail+1, file) == NULL ) {
			cs_free(cs);
			*csp = NULL;
			return -1;
		}
		for ( trav = p ; trav < p + avail; ++trav )
			if ( *trav == '\n' )
				break;
		if ( trav != p + avail ) {
			eol = 1;
			abort_unless(trav - p + 1 <= avail);
			cs->cs_dlen = sofar + (trav - p + 1);
			/* guaranteed by fgets() */
			abort_unless(cs->cs_data[cs->cs_dlen] == '\0');
		} else {
			/* guaranteed by fgets() */
			abort_unless(p[avail] == '\0');
			sofar += avail;
			if ( cs_grow(cs, cs->cs_size * 2) < 0 ) {
				cs_free(cs);
				*csp = NULL;
				return -1;
			}
		}
	}

	*csp = cs;
	return 0;
}
#endif /* CAT_USE_STDLIB */


struct catstr *cs_alloc(size_t len)
{
	struct catstr *cs;

	if ( len > CS_MAXLEN )
		return NULL;
	if ( (cs = mem_get(cs_mmp, sizeof(*cs))) == NULL )
		return NULL;
	if ( (cs->cs_data = mem_get(cs_mmp, cs_alloc_size(len))) == NULL ) {
		mem_free(cs_mmp, cs);
		return NULL;
	}

	cs->cs_size = len;
	cs->cs_dlen = 0;
	cs->cs_data[0] = '\0';
	cs->cs_dynamic = 1;

	return cs;
}


void cs_free(struct catstr *cs)
{
	CKCSN(cs);
	abort_unless(cs->cs_dynamic);
	mem_free(cs_mmp, cs->cs_data);
	mem_free(cs_mmp, cs);
}


void cs_setmm(struct memmgr *mm)
{
	abort_unless(mm);
	cs_mmp = mm;
}

