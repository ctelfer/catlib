/*
 * str.c -- String manipulation functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/grow.h>
#include <cat/str.h>
#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB
#include <string.h>
#else
#include <cat/catstdlib.h>
#endif


struct raw *str2raw(struct raw *r, char *s)
{
	r->data = s;
	r->len  = strlen(s)+1;
	return r;
}



void str_clear(struct cstring *s)
{
	Assert(s && s->buf.data && s->buf.len);
	s->slen = 0;
}


void str_init(struct cstring *s, char *buf, long blen, struct memctl *m, 
	      int bufisstring)
{
	long slen;
	Assert(s);
	Assert(buf);
	Assert(blen > 0);

	s->buf.data = buf;
	s->buf.len = blen;
	s->slen = 0;
	s->memctl = m;
	if ( bufisstring ) {
		slen = strlen(buf) + 1;
		Assert(slen <= blen);
		s->slen = slen;
	}
}


struct cstring *str_new(const char *src, long len, struct memctl *m)
{
	struct cstring *s;
	long alen;
	Assert(src || len >= 0);
	Assert(m);

	if ( len < 0 )
		len = strlen(src) + 1;
	alen = len;
	if ( len == 0 )
		alen = 1;

	s = m->mc_alloc(m, alen + sizeof(struct cstring));
	if ( !s )
		return NULL;

	s->buf.data = (char *)(s + 1);
	s->buf.len  = alen;
	s->memctl   = m;

	if ( src ) {
		memcpy(s->buf.data, src, len);
		s->slen = len;
	} else {
		s->buf.data[0] = '\0';
		s->slen = 0;
	}
	return s;
}


void str_free(struct cstring *s)
{
	Assert(s && s->buf.data && s->buf.len && s->memctl && 
	       s->memctl->mc_free && s->slen >= 0 && s->slen <= s->buf.len);
	s->memctl->mc_free(s->memctl, s);
}


void str_set(struct cstring *s, const void *d, long l)
{
	Assert(s && s->buf.data && s->buf.len && 
	       s->slen >= 0 && s->slen <= s->buf.len);
	Assert(l >= 0 && l <= s->buf.len);
	memcpy(s->buf.data, d, l);
	s->slen = l;
}


void str_scopy(struct cstring *s, const char *cp)
{
	long i;
	char *dp;
	Assert(s && s->buf.data && s->buf.len &&
	       s->slen >= 0 && s->slen <= s->buf.len);
	dp = s->buf.data;
	for ( i = 0 ; i < s->buf.len && *cp ; ++i )
		*dp++ = *cp++;
	if ( i >= s->buf.len ) {
		dp -= 1;
		i -= 1;
	}
	*dp = '\0';
	s->slen = i;
}


struct cstring *str_copy(struct cstring *s)
{
	struct cstring *news;
	struct memctl *m;
	long alen;

	Assert(s && s->buf.data && s->buf.len && s->memctl && 
	       s->memctl->mc_alloc && s->slen >= 0 && s->slen <= s->buf.len);
	m = s->memctl;
	alen = s->slen;
	if ( !alen )
		alen = 1;
	news = m->mc_alloc(m, alen + sizeof(struct cstring));
	if ( !news )
		return NULL;
	news->buf.data = (char *)(news + 1);
	news->buf.len  = alen;
	news->slen     = s->slen;
	news->memctl   = m;
	memcpy(news->buf.data, s->buf.data, s->slen);
	return news;
}


int str_grow(struct cstring **sp, long size)
{
	struct cstring *s;
	struct memctl *m;
	unsigned long curlen;

	Assert(sp);
	s = *sp;
	Assert(s && s->buf.data && s->buf.len && s->memctl && 
	       s->slen >= 0 && s->slen <= s->buf.len);
	m = s->memctl;
	if ( size <= s->buf.len )
		return 0;
	curlen = s->buf.len + sizeof(struct cstring);
	if ( mc_grow(m, (char **)&s, &curlen, size+sizeof(struct cstring)) < 0 )
		return -1;
	s->buf.data = (char *)(s + 1);
	s->buf.len = curlen - sizeof(struct cstring);
	*sp = s;
	return 0;
}


struct cstring *str_slice(struct cstring *s, long start, long end, int nulterm)
{
	struct cstring *ns;
	long len, addnull = 0;
	Assert(s && s->buf.data && s->buf.len &&
	       s->slen >= 0 && s->slen <= s->buf.len);

	if ( start < 0 )
		start = -start >= s->slen ? 0 : s->slen + start + 1;
	if ( end < 0 )
		end = -end >= s->slen ? 0 : s->slen + end + 1;
	if ( end > s->slen )
		end = s->slen;

	if ( end <= start ) {
		ns = str_new(NULL, 0, s->memctl); /* empty string */
		if ( ! ns ) 
			return NULL;
		if ( nulterm )
			ns->buf.data[0] = '\0';
	} else {
		len = end - start;
		if ( nulterm && s->buf.data[end - 1] != '\0' )
			addnull = 1;
		ns = str_new(NULL, (addnull ? len + 1 : len), s->memctl);
		if ( ! ns )
			return NULL;
		memcpy(ns->buf.data, s->buf.data + start, len);
		ns->slen = len;
		if ( addnull ) {
			ns->buf.data[len] = '\0';
			ns->slen++;
		}
	}

	return ns;
}


struct cstring *str_splice(struct cstring *s, long start, long end,
			   struct cstring *is, int nullstrings)
{
	int addnull;
	long len, islen;
	struct cstring *ns;

	Assert(s && s->buf.data && s->buf.len &&
	       s->slen >= 0 && s->slen <= s->buf.len);
	Assert(!is || (is->buf.data && is->buf.len &&
	       is->slen >= 0 && is->slen <= is->buf.len));

	if ( start < 0 )
		start = -start >= s->slen ? 0 : s->slen + start + 1;
	else if ( start > s->slen )
		start = s->slen;
	if ( end < 0 )
		end = -end >= s->slen ? 0 : s->slen + end + 1;
	else if ( end > s->slen )
		end = s->slen;

	if ( is )
		islen = is->slen;
	else
		islen = 0;
	if ( nullstrings ) {
		if ( is && islen && is->buf.data[islen - 1] == '\0' )
			islen -= 1;
		if ( end == s->slen || s->buf.data[s->slen - 1] != '\0' )
			addnull = 1;
	}
	len = s->slen - (end - start) + islen + (addnull ? 1 : 0);

	ns = str_new(NULL, len, s->memctl);
	if ( ! ns )
		return NULL;

	memcpy(ns->buf.data, s->buf.data, start);
	if ( is && islen )
		memcpy(ns->buf.data + start, is->buf.data, islen);
	memcpy(ns->buf.data + start + islen, s->buf.data + end, s->slen - end);
	if ( addnull )
		ns->buf.data[len-1] = '\0';
	ns->slen = len;
	return ns;
}


int str_cmp(void *s1p, void *s2p)
{
	long len, rv;
	struct cstring *s1 = s1p, *s2 = s2p;
	Assert(s1 && s1->buf.data && s1->buf.len && 
	       s1->slen >= 0 && s1->slen <= s1->buf.len);
	Assert(s2 && s2->buf.data && s2->buf.len && 
	       s2->slen >= 0 && s2->slen <= s2->buf.len);
	len = s1->slen < s2->slen ? s1->slen : s2->slen;
	rv = memcmp(s1->buf.data, s2->buf.data, len);
	if ( rv == 0 && s1->slen != s2->slen )
		return s1->slen < s2->slen ? -1 : 1;
	else
		return rv;
}


long str_find(struct cstring *s, const char *pat, long len)
{
	long i, j;

	if ( len == 0 )
		return 0;
	if ( len < 0 )
		len = strlen(pat);

	for ( i = 0 ; i <= s->slen - len ; ++i ) {
		for ( j = 0 ; j < len ; ++j )
			if ( s->buf.data[i + j] != pat[j] )
				break;
		if ( j == len )
			return i;
	}

	return -1;
}


#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <cat/mem.h>
#include <cat/err.h>

char *str_fmt(const char *fmt, ...)
{
	char *s = NULL;
	va_list ap; 
	unsigned long len = 0;
	int r = 0;

	Assert(fmt);

	while ( 1 ) {
		if ( grow(&s, &len, r) < 0 )
			errsys("str_fmt: ");

		va_start(ap, fmt);
		r = vsnprintf(s, len, fmt, ap);
		va_end(ap);

		if ( r < 0 )	/* SUSv4 return value */
			r = 0;
		else if ( r >= len ) /* C99 return value */
			r += 1;
		else		/* Success */
			break;
	}

	return s;
}


char *str_dup(const char *s)
{
	int len;
	char *ns;

	Assert(s);

	if ( ! s ) 
		return NULL;
	len = strlen(s) + 1;
	ns = xmalloc(len);
	memcpy(ns, s, len-1);
	ns[len-1] = '\0';
	return ns;
}



char *str_tok(char *s, const char *d, unsigned *len)
{
	char *start;

	Assert(s);
	Assert(d);
	Assert(len);

	start = s + strspn(s, d);
	if ( !*start ) 
		return NULL;
	if ( len ) 
		*len = strcspn(start, d);

	return start;
}


struct cstring *str_fnew(struct memctl *m, const char *fmt, ...)
{
	struct cstring *s;
	char tstr[4];
	va_list ap;
	int r = 0;

	va_start(ap, fmt);
	r = vsnprintf(tstr, sizeof(tstr), fmt, ap);
	va_end(ap);

	if ( r < 0 )
		return NULL;
	s = str_new(NULL, r + 1, m);
	va_start(ap, fmt);
	vsnprintf(s->buf.data, s->buf.len, fmt, ap);
	va_end(ap);
	s->slen = r + 1;
	return s;
}

#endif /* CAT_USE_STDLIB */
