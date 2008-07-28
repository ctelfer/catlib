/*
 * catstr.c -- application level strings
 *
 * by Christopher Adam Telfer
 *  
 * Copyright 2007, 2008 See accompanying license
 *  
 */

#include <cat/catstr.h>
#include <cat/str.h>
#include <cat/emalloc.h>
#include <cat/stduse.h>
#include <cat/err.h>
#include <cat/grow.h>
#include <cat/match.h>
#include <cat/stduse.h>

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

#define CKCS(cs)							\
	if (!cs || (cs->cs_size == CS_UNINITIALIZED) || 		\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen < cs->cs_size))	\
		return CS_ERROR;

#define CKCSP(cs)							\
	if (!cs || (cs->cs_size == CS_UNINITIALIZED) || 		\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen < cs->cs_size))	\
		return NULL;

#define CKCSN(cs)							\
	if (!cs || (cs->cs_size == CS_UNINITIALIZED) || 		\
	    (cs->cs_size > CS_MAXLEN) || (cs->cs_dlen < cs->cs_size))	\
		return;

void cs_init(struct catstr *cs, size_t size)
{
	if ( size > CS_MAXLEN ) {
		cs->cs_size = CS_UNINITIALIZED;
		return;
	}

	cs->cs_size = size;
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


size_t cs_find_cc(const struct catstr *cs, char ch)
{
	const unsigned char *cp;
	const unsigned char *end;
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
	const unsigned char *cp;
	const unsigned char *ap;
	const unsigned char *end;
	size_t off = 0;
	CKCS(cs);

	cp = cs->cs_data;
	end = cp + cs->cs_dlen;
	while ( cp < end ) {
		for ( ap = (const unsigned char *)accept ; *ap != '\0' ; ++ap )
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
	const unsigned char *cp;
	const unsigned char *rp;
	const unsigned char *end;
	size_t off = 0;
	CKCS(cs);

	cp = cs->cs_data;
	end = cp + cs->cs_dlen;
	while ( cp < end ) {
		for ( rp = (const unsigned char *)reject ; *rp != '\0' ; ++rp )
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
	const unsigned char *fcp = (const unsigned char *)utf8ch;
	const unsigned char *scp;

	CKCS(cs);
	abort_unless(utf8ch);
	scp = (const unsigned char *)cs->cs_data;
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


static int utf8match(const unsigned char *c1, const unsigned char *c2)
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
	const unsigned char *fcp = (const unsigned char *)utf8ch;
	const unsigned char *scp;

	CKCS(cs);
	abort_unless(utf8ch && nc > 0);
	scp = (const unsigned char *)cs->cs_data;
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
	const unsigned char *fcp = (const unsigned char *)utf8ch;
	const unsigned char *scp;

	CKCS(cs);
	abort_unless(utf8ch && nc > 0);
	scp = (const unsigned char *)cs->cs_data;
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



#if CAT_USE_STDLIB
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
	struct bmpat *bmp;
	size_t off;
	int rv;
	struct raw rstr;

	CKCS(findin);

	/* TODO: fix bm_* so it uses size_t */
	abort_unless(r->len <= (unsigned long)~0);
	bmp = bm_pnew((struct raw *)r);

	rstr.len = findin->cs_dlen;
	rstr.data = (char *)findin->cs_data;
	rv = bm_match(&rstr, bmp, &off);
	free(bmp);

	if ( rv < 0 )
		return CS_NOTFOUND;
	else
		return off;
}
#endif /* CAT_USE_STDLIB */


struct catstr *cs_from_chars(const char *s)
{
	size_t slen;
	struct catstr *cs;

	slen = strlen(s);

	cs = cs_alloc(slen);
	abort_unless(cs && (cs->cs_size == cs_alloc_size(slen)));
	cs->cs_dlen = slen;
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

	if ( rv < 0 )
		err("cs_format: error in formatting");

	cs = cs_alloc(rv);
	cs->cs_dlen = rv;
	va_start(ap, fmt);
	rv2 = str_vfmt((char *)cs->cs_data, rv+1, fmt, ap);
	va_end(ap);
	abort_unless(rv == rv2);

	return cs;
}


struct catstr *cs_concat(struct catstr *first, struct catstr *second)
{
	struct catstr *cs;
	size_t newlen;

	CKCSP(first);
	CKCSP(second);

	if ( CS_MAXLEN - first->cs_dlen > second->cs_dlen )
		err("cs_concat:  size overflow");
	newlen = first->cs_dlen + second->cs_dlen;
	cs = cs_alloc(newlen);
	abort_unless(cs && (cs->cs_size == cs_alloc_size(newlen)));
	memmove(cs->cs_data, first->cs_data, first->cs_dlen);
	memmove(cs->cs_data + first->cs_dlen, second->cs_data, second->cs_dlen);
	cs->cs_dlen = newlen;
	cs->cs_data[newlen] = '\0';

	return cs;
}


struct catstr *cs_grow(struct catstr *cs, size_t minlen)
{
	char *csp;
	size_t tlen, olen;
	CKCSP(cs);
	abort_unless(minlen <= CS_MAXLEN);

	csp = (char *)cs;
	olen = cs->cs_size;
	if ( grow(&csp, &tlen, cs_alloc_size(minlen)) < 0 )
		err("cs_grow: could not increase allocation");
	abort_unless(tlen > olen);

	cs = (struct catstr *)csp;
	cs->cs_size = cs_data_size(tlen);

	return cs;
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

	cs = cs_alloc(len);
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
struct catstr *cs_fd_readline(int fd)
{
	struct catstr *cs;
	unsigned char *p;
	int eol = 0, rv;
	size_t sofar = 0;

	cs = cs_alloc(128);
	abort_unless(cs);
	while ( !eol ) {
		p = cs->cs_data + sofar;
		if ( (rv = read(fd, p, 1)) < 1 ) {
			if ( rv == 0 ) {
				eol = 1;
			} else {
				if ( errno == EINTR )
					continue;
				cs_free(cs);
				return NULL;
			}
		} else if ( *p == '\n' ) {
			++sofar;
			eol = 1;
		} else {
			++sofar;
			if ( sofar == cs_tlen(*cs) - 1 )
				cs = cs_grow(cs, cs_tlen(*cs) * 2);
			abort_unless(cs);
		}
	}

	cs->cs_dlen = sofar;
	cs->cs_data[sofar] = '\0';

	return cs;
}


struct catstr *cs_file_readline(FILE *file)
{
	struct catstr *cs;
	unsigned char *p, *trav;
	int eol = 0;
	size_t sofar = 0, toget;

	cs = cs_alloc(128);
	abort_unless(cs);
	while ( !eol ) {
		p = cs->cs_data + sofar;
		toget = cs_tlen(*cs) - sofar;
		abort_unless(toget > 2);
		if ( fgets((char *)p, toget, file) == NULL ) {
			cs_free(cs);
			return NULL;
		}
		for ( trav = p ; trav < p + toget ; ++trav )
			if ( *trav == '\n' )
				break;
		if ( trav != p + toget ) {
			eol = 1;
			abort_unless(trav - p + 1 < toget);
			cs->cs_dlen = sofar + (trav - p + 1);
			/* guaranteed by fgets() */
			abort_unless(cs->cs_data[cs->cs_dlen] == '\0');
		} else {
			/* guaranteed by fgets() */
			abort_unless(p[toget - 1] == '\0');
			sofar += toget - 1;
			cs = cs_grow(cs, cs_tlen(*cs) * 2);
			abort_unless(cs);
		}
	}
	return cs;
}
#endif /* CAT_USE_STDLIB */


struct catstr *cs_alloc(size_t len)
{
	struct catstr *cs;

	if ( len > CS_MAXLEN )
		err("cs_alloc: size overflow");

	cs = emalloc(cs_alloc_size(len));
	cs->cs_size = len;
	cs->cs_dlen = 0;
	cs->cs_data[len] = '\0';
	cs->cs_data[0] = '\0';

	return cs;
}


void cs_free(struct catstr *cs)
{
	CKCSN(cs);
	free(cs);
}

