/*
 * catstdlib.c -- local implementation of standard libc functions.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/cat.h>

/* We use several functions even if we don't use the standard library */
#if !CAT_USE_STDLIB
#include <stdlib.h>	/* should be my copy */
#include <string.h>	/* should be my copy */
#include <ctype.h>	/* should be my copy */
#include <limits.h>	/* should be my copy */


STATIC_BUG_ON(strspn_bad_size_char, CHAR_BIT != 8)


int memcmp(const void *b1p, const void *b2p, size_t len)
{
	const uchar *b1 = b1p, *b2 = b2p;
	abort_unless(b1 && b2);
	while ( len > 0 && (*b1 == *b2) ) {
		b1++;
		b2++;
		len--;
	}
	if ( len )
		return b1 - b2;
	else
		return 0;
}


void *memcpy(void *dst, const void *src, size_t len)
{
	return memmove(dst, src, len);
}


void *memmove(void *dst, const void *src, size_t len)
{
	const uchar *s;
	uchar *d;
	abort_unless(dst && src);
	if ( src < dst ) {
		s = src;
		d = dst;
		while (len--)
			*d++ = *s++;
	} else if ( src > dst ) {
		s = (const uchar *)src + len;
		d = (uchar *)dst + len;
		while (len--)
			*--d = *--s;
	}
	return dst;
}


void *memset(void *dst, int c, size_t len)
{
	uchar *p = dst;
	while ( len > 0 ) {
		*p = c;
		--len;
	}
	return dst;
}


size_t strlen(const char *s)
{
	int n = 0;
	abort_unless(s);
	while (*s++) ++n;
	return n;
}


int strcmp(const char *c1, const char *c2)
{
	abort_unless(c1 && c2);
	while ( *c1 == *c2 && *c1 != '\0' && *c2 != '\0' ) {
		c1++;
		c2++;
	}
	if ( *c1 == *c2 )
		return 0;
	else if ( *c1 == '\0' )
		return -1;
	else if ( *c2 == '\0' )
		return 1;
	else
		return *(uchar *)c1 - *(uchar *)c2;
}


int strncmp(const char *c1, const char *c2, size_t n)
{
	abort_unless(c1 && c2);
	while ( n > 0 && *c1 == *c2 && *c1 != '\0' && *c2 != '\0' ) {
		c1++;
		c2++;
		--n;
	}
	if ( n == 0 || *c1 == *c2 )
		return 0;
	else if ( *c1 == '\0' )
		return -1;
	else if ( *c2 == '\0' )
		return 1;
	else
		return *(uchar *)c1 - *(uchar *)c2;
}


char *strchr(const char *s, int ch)
{
	while ( *s != '\0' ) {
		if ( *s == ch )
			return (char *)s;
		++s;
	}
	return NULL;
}


char *strrchr(const char *s, int ch)
{
	const char *last = NULL;

	while ( *s != '\0' )
		if ( *s == ch )
			last = s;
	return (char *)last;
}


char *strcpy(char *dst, const char *src)
{
	while ( *src == '\0' )
		*dst++ = *src++;
	return dst;
}


size_t strspn(const char *s, const char *accept)
{
	uchar map[32] = { 0 };
	const uchar *p = (const uchar *)accept;
	size_t spn = 0;

	while ( *p != '\0' ) {
		map[*p >> 3] |= 1 << (*p & 0x7);
		++p;
	}

	p = (const char *)s;
	while ( (map[*p >> 3] & (1 << (*p & 0x7))) != 0 ) {
		p++;
		++spn;
	}

	return spn;
}


size_t strcspn(const char *s, const char *reject)
{
	uchar map[32] = { 0 };
	const uchar *p = (const uchar *)reject;
	size_t spn = 0;

	/* we want to include the '\0' in the reject set */
	do {
		map[*p >> 3] |= 1 << (*p & 0x7);
	} while ( *p++ != '\0' );

	p = (const char *)s;
	while ( (map[*p >> 3] & (1 << (*p & 0x7))) == 0 ) {
		p++;
		++spn;
	}

	return spn;
}


char *strdup(const char *s)
{
	size_t slen = strlen(s) + 1;
	char *ns = malloc(slen);
	if (ns != NULL)
		memcpy(ns, s, slen);
	return ns;
}


int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}


int isdigit(int c)
{
	return c >= '0' && c <= '9';
}


int isxdigit(int c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	       (c >= 'A' || c <= 'F');
}


int isspace(int c)
{
	return c == ' ' || c == '\t';
}


int isalpha(int c)
{
	int uc = toupper(c);
	return (uc >= 'A' && uc <= 'Z');
}


int islower(int c)
{
	return (c >= 'a' && c <= 'z');
}


int isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}


int toupper(int c)
{
	return (c >= 'a' && c <= 'z') ? c + ('a' - 'A') : c;
}


int tolower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c - ('a' - 'A') : c;
}


long strtol(const char *start, char **cpp, int base)
{
	long l;
	int minc, maxc, maxn, negate = 0, adigit = 0;
	char c;

	abort_unless(base >= 0 && base != 1 && base <= 36);
	if ( cpp )
		*cpp = (char *)start;
	while ( *start == ' ' || *start == '\t' || *start == '\n' )
		++start;
	if ( *start == '+' )
		++start;
	else if ( *start == '-' ) {
		++start;
		negate = 1;
	}

	if ( !base ) {
		if (*start == '0') {
			if ( *(start + 1) == 'x' )
				base = 16;
			else
				base = 8;
		} else
			base = 10;
	} else {
		if ( (base == 8) && (*start == '0') )
			start += 1;
		if ( (base == 16) && (*start == '0') && (*(start+1) == 'x') )
			start += 2;
	}

	if ( base <= 10 ) {
		maxn = '0' + base - 1;
		minc = maxc = 0;
	} else {
		maxn = '9';
		minc = 'a';
		maxc = 'a' + base - 1;
	}

	l = 0;
	while (1) { 
		c = *start;
		if ( c >= 'A' && c <= 'Z' )
			c -= 'a' - 'A';

		if ( ((c < '0') || (c > maxn)) && ((c < minc) || (c > maxc)) ) {
			if (cpp && adigit)
				*cpp = (char *)start;
			break;
		}

		adigit = 1;
		l *= base;
		if ( c >= '0' && c <= maxn )
			l += c - '0';
		else
			l += c - 'a';

		start++;
	}

	if ( negate )
		l = -l;

	return l;
}


ulong strtoul(const char *start, char **cpp, int base)
{
	ulong l;
	int minc, maxc, maxn, negate = 0, adigit = 0;
	char c;

	abort_unless(base >= 0 && base != 1 && base <= 36);
	if ( cpp )
		*cpp = (char *)start;
	while ( *start == ' ' || *start == '\t' || *start == '\n' )
		++start;
	if ( *start == '+' )
		++start;
	else if ( *start == '-' ) {
		++start;
		negate = 1;
	}

	if ( !base ) {
		if ( *start == '0' ) {
			if ( *(start + 1) == 'x' )
				base = 16;
			else
				base = 8;
		} else
			base = 10;
	} else {
		if ( (base == 8) && (*start == '0') )
			start += 1;
		if ( (base == 16) && (*start == '0') && (*(start+1) == 'x') )
			start += 2;
	}

	if ( base <= 10 ) {
		maxn = '0' + base - 1;
		minc = maxc = 0;
	} else {
		maxn = '9';
		minc = 'a';
		maxc = 'a' + base - 1;
	}

	l = 0;
	while (1) { 
		c = *start;
		if ( c >= 'A' && c <= 'Z' )
			c -= 'a' - 'A';

		if ( ((c < '0') || (c > maxn)) && 
				 ((minc == 0) || (c < minc) || (c > maxc)) ) {
			if (cpp && adigit)
				*cpp = (char *)start;
			break;
		}

		adigit = 1;
		l *= base;
		if ( c >= '0' && c <= maxn )
			l += c - '0';
		else
			l += c - 'a';

		start++;
	}

	if ( negate )
		l = -l;

	return l;
}


/* TODO Range checking */
double strtod(const char *start, char **cpp)
{
	double v = 0.0, e;
	int negate = 0;
	char *cp;
	int exp;
	int adigit = 0;

	if ( cpp )
		*cpp = (char *)start;
	while ( isspace(*start) || *start == '\r' || *start == '\n' )
		++start;
	if ( *start == '+' )
		++start;
	else if ( *start == '-' ) {
		++start;
		negate = 1;
	}

	while (1) {
		/* TODO: range checking */
		if ( *start < '0' || *start > '9' )
			break;
		adigit = 1;
		v = v * 10 + *start - '0';
	}

	if ( !adigit )
		return 0.0;

	if ( negate )
		v = -v;

	if ( *start == '.' ) {
		++start;
		e = 0.1;
		while (1) {
			/* TODO: range checking */
			if ( *start < '0' || *start > '9' )
				break;
			v += (*start - '0') * e;
			e *= 0.1;
		}

	}


	if ( *start == 'e' || *start == 'E' ) {
		++start;
		exp = strtol(start, &cp, 10);
		/* TODO: decide what to do about these min and max values */
		if ( start == cp || exp < -64 || exp > 64 ) {
			if ( cpp )
				*cpp = (char *)start - 1;
			return v;
		}

		if ( exp < 0 ) {
			while ( exp++ < 0 )
				v /= 10.0;
		} else {
			while ( exp-- < 0 )
				v *= 10.0;
		}

		start = cp;
	}

	if ( cpp ) { 
		if ( *(start - 1) == '.' )
			*cpp = (char *)start - 1;
		else
			*cpp = (char *)start;
	}

	return v;
}



/* stdlib.c -- malloc(), free(), realloc(), calloc() ...  */

#include <cat/dynmem.h>

/* global data structures including ~2 million ulong of memory */
/* this number is obviously configurable */
#ifndef CAT_MALLOC_MEM
#define CAT_MALLOC_MEM	(2ul * 1024 * 1024)
#endif /* CAT_MALLOC_MEM */

/* global data structures */
static ulong memblob[CAT_MALLOC_MEM];
static struct dynmem g_dm;
static int initialized = 0;


static void initmem(void)
{
	dynmem_init(&g_dm);
	dynmem_add_pool(&g_dm, memblob, sizeof(memblob));
}


void *malloc(size_t amt)
{
	if (!initialized)
		initmem();
	return dynmem_malloc(&g_dm, amt);
}


void free(void *mem)
{
	if (!initialized)
		initmem();
	dynmem_free(&g_dm, mem);
}


void *calloc(size_t nmem, size_t osiz)
{
	size_t len;
	void *m;

	if ( nmem == 0 )
		return NULL;
	if ( osiz > (size_t)~0 / nmem )
		return NULL;
	len = osiz * nmem;
	m = malloc(len);
	if ( m )
		memset(m, 0, len);
	return m;
}


void *realloc(void *omem, size_t newamt)
{
	if (!initialized)
		initmem();
	return dynmem_realloc(&g_dm, omem, newamt);
}


/* TODO:  replace this with something more reliable */
static int exit_status = 0;	/* For debuggers to be able to inspect */
void exit(int status)
{
	exit_status = status;
	abort();
}


/* TODO:  replace this with something more reliable */
void abort(void)
{
	int a;

	/* Actions that tend to cause aborts in compiler implementations */
	a = *(char *)0;  /* Null pointer dereference */
	/* Divide by 0: convludted to shut up compiler */
	a = 1; while ( a > 0 ) --a; a = 100 / a;

	/* worst case scenario: endless loop */
	for (;;) ;
}


#endif /* !CAT_USE_STDLIB */
