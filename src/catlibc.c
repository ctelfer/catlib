/*
 * catlibc.c -- local implementation of standard libc functions.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2014 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/catlibc.h>

/* We use several functions even if we don't use the standard library */
#if !CAT_USE_STDLIB
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>	/* should be my copy */
#include <string.h>	/* should be my copy */
#include <ctype.h>	/* should be my copy */
#include <limits.h>	/* should be my copy */
#include <stdio.h>	/* should be my copy */

#include <cat/emit.h>
#include <cat/emit_format.h>



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
		return (int)*b1 - (int)*b2;
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
	while ( len-- ) *p = c;
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
		return (int)*(uchar *)c1 - (int)*(uchar *)c2;
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
		return (int)*(uchar *)c1 - (int)*(uchar *)c2;
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
	char *dsave = dst;
	do {
		*dst++ = *src;
	} while ( *src++ != '\0' );
	return dsave;
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
	return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}


int tolower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
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
#if CAT_HAS_FLOAT
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
#else /* CAT_HAS_FLOAT */
	return 0.0;
#endif /* CAT_HAS_FLOAT */
}



/* stdlib.c -- malloc(), free(), realloc(), calloc() ...  */

#include <cat/dynmem.h>

static struct dynmem dmheap;


void *malloc(size_t amt)
{
	return dynmem_malloc(&dmheap, amt);
}


void free(void *mem)
{
	dynmem_free(&dmheap, mem);
}


void *calloc(size_t nmem, size_t osiz)
{
	size_t len;
	void *m;

	if ( nmem == 0 )
		return NULL;
#if CAT_HAS_DIV
	if ( osiz > (size_t)~0 / nmem )
		return NULL;
#else
	if ( osiz > uldivmod((size_t)~0, nmem, 1) )
		return NULL;
#endif
	len = osiz * nmem;
	m = malloc(len);
	if ( m )
		memset(m, 0, len);
	return m;
}


void *realloc(void *omem, size_t newamt)
{
	return dynmem_realloc(&dmheap, omem, newamt);
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

#if CAT_HAS_DIV
	/* Divide by 0: convludted to shut up compiler */
	a = 1; while ( a > 0 ) --a; a = 100 / a;
#endif /* CAT_HAS_DIV */

	/* worst case scenario: endless loop */
	for (;;) ;
}


#define MAXFILES		32
#define CAT_STDIN_FILENO	0
#define CAT_STDOUT_FILENO	1
#define CAT_STDERR_FILENO	2

static char Stdin_buf[BUFSIZ];
static char Stdout_buf[BUFSIZ];
static char Stderr_buf[BUFSIZ];

static FILE File_table[MAXFILES] = { 

	{ Stdin_buf, sizeof(Stdin_buf),
	  0, _IOFBF|CAT_SIO_READ|CAT_SIO_BUFGIVEN, 
	  CAT_STDIN_FILENO, 0 },

	{ Stdout_buf, sizeof(Stdout_buf),
	  0, _IOFBF|CAT_SIO_WRITE|CAT_SIO_BUFGIVEN, 
	  CAT_STDOUT_FILENO, 0 },

	{ 
	  Stderr_buf, sizeof(Stderr_buf),
	  0, _IONBF|CAT_SIO_WRITE|CAT_SIO_BUFGIVEN, 
	  CAT_STDERR_FILENO, 0 },

	{ 0 }
};


FILE *stdin  = &File_table[0];
FILE *stdout = &File_table[1];
FILE *stderr = &File_table[2];


FILE *fopen(const char *path, const char *mode)
{
	return NULL;
}


FILE *freopen(const char *path, const char *mode, FILE *file)
{
	if ( fflush(file) < 0 )
		return NULL;

	fclose(file);

	return NULL;
}


static size_t do_fput_bytes(FILE* file, const char *buf, size_t len)
{
	if ( file == NULL ||
	     (len > 0 && (buf == NULL || file->f_buffer == NULL)) ) {
		if ( file != NULL )
			file->f_flags |= CAT_SIO_ERR;
		return 0;
	}

	abort_unless(file->f_fill <= file->f_buflen);

	if ( len > file->f_buflen - file->f_fill )
		len = file->f_buflen - file->f_fill;

	memcpy(&file->f_buffer[file->f_fill], buf, len);
	file->f_fill += len;
	file->f_lastop = CAT_SIO_WRITE;

	return len;
}


static int flush_all()
{
	FILE *fp, *end = File_table + MAXFILES;
	for ( fp = File_table ; fp < end ; ++fp )
		if ( fflush(fp) < 0 )
			return -1;
	return 0;
}


int fflush(FILE *file)
{
	size_t rv;

	if ( file == NULL )
		return flush_all();

	if ( file->f_lastop == CAT_SIO_WRITE ) {
		if ( file->f_fill > 0 ) {
			rv = do_fput_bytes(file, file->f_buffer, file->f_fill);
			if ( rv < file->f_fill )
				return -1;
			file->f_fill = 0;
		}
	} else {
		/* TODO: once we get read operations */
	}

	return 0;
}


int fclose(FILE *file)
{
	if ( fflush(file) < 0 )
		return -1;

#if CAT_HAS_POSIX
	if ( close(file->f_fd) < 0 )
		return -1;
#endif /* CAT_HAS_POSIX */

	if ( file->f_buffer != NULL && 
	     (file->f_flags & CAT_SIO_BUFGIVEN) == 0 ) {
		free(file->f_buffer);
	}
	file->f_buffer = NULL;
	file->f_buflen = 0;
	file->f_flags = _IOUNUSED;

	return 0;
}


void clearerr(FILE *file)
{
	if ( file == NULL )
		return;
	file->f_flags &= ~CAT_SIO_ERR;
}


int feof(FILE *file)
{
	if ( file == NULL )
		return -1;
	return (file->f_flags & CAT_SIO_EOF) != 0;
}


int ferror(FILE *file)
{
	if ( file == NULL )
		return -1;
	return (file->f_flags & CAT_SIO_ERR) != 0;
}


int fileno(FILE *file)
{
	if ( file == NULL )
		return -1;
	return file->f_fd;
}


size_t fwrite(const void *ptr, size_t msiz, size_t nmem, FILE *file)
{
	size_t i, rv;
	const char *buf = ptr;

	if ( msiz == 0 || nmem == 0 )
		return 0;

	if ( ptr == NULL || file == NULL )
		return 0;

	for ( i = 0 ; i < nmem ; ++i ) {
		rv = do_fput_bytes(file, buf, msiz);
		if ( rv == 0 )
			return i;
		buf += msiz;
	}
	return i;
}


int fputc(int c, FILE *file)
{
	char ch = c;
	int rv;
	rv = do_fput_bytes(file, &ch, sizeof(ch));
	if ( rv < 0 )
		return EOF;
	return c;
}


int fputs(const char *s, FILE *file)
{
	size_t rv;
	rv = do_fput_bytes(file, s, strlen(s));
	return (rv == 0) ? -1 : 0;
}


int putc(int c, FILE *file)
{
	return fputc(c, file);
}


int putchar(int c)
{
	return fputc(c, stdout);
}


int puts(const char *s)
{
	return fputs(s, stdout);
}


size_t fread(void *ptr, size_t size, size_t nmemb, FILE *file)
{
	/* TODO */
	if ( file != NULL ) {
		if ( file->f_flags & CAT_SIO_READ )
			file->f_flags |= CAT_SIO_EOF;
		else
			file->f_flags |= CAT_SIO_ERR;
	}

	return 0;
}


void setbuf(FILE *file, char *buf)
{
	setvbuf(file, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}


void setbuffer(FILE *file, char *buf, size_t size)
{
	setvbuf(file, buf, buf ? _IOFBF : _IONBF, size);
}


void setlinebuf(FILE *file)
{
	setvbuf(file, NULL, _IOLBF, 0);
}


int setvbuf(FILE *file, char *buf, int mode, size_t bsiz)
{
	if ( file == NULL )
		return -1;

	if ( mode != _IONBF && mode != _IOLBF && mode != _IOFBF )
		return -1;

	/* enforce the rule that buffering must be performed between initial */
	/* open and any subsequent operations */
	if ( file->f_lastop )
		return -1;

	if ( file->f_fill > 0 && fflush(file) < 0 )
		return -1;

	if ( bsiz > INT_MAX )
		return -1;

	file->f_flags &= _IOBFMASK;
	file->f_flags |= mode;

	if ( buf != NULL ) {
		abort_unless(bsiz > 0);
		file->f_flags = CAT_SIO_BUFGIVEN;
		file->f_buffer = buf;
		file->f_buflen = bsiz;
	}

	return 0;
}


int printf(const char *fmt, ...)
{
	va_list ap;
	int rv;
	va_start(ap, fmt);
	rv = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return rv;
}


int fprintf(FILE *file, const char *fmt, ...)
{
	va_list ap;
	int rv;
	va_start(ap, fmt);
	rv = vfprintf(file, fmt, ap);
	va_end(ap);
	return rv;
}


int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}


struct file_emitter {
	struct emitter	emitter;
	FILE *		file;
};


int file_emit_func(struct emitter *e, const void *buf, size_t len)
{
	size_t rv;
	struct file_emitter *fe = (struct file_emitter *)e;
	abort_unless(e && buf);
	if ( (rv = do_fput_bytes(fe->file, buf, len)) == 0 ) {
		e->emit_state = EMIT_ERR;
		return -1;
	}
	return rv;
}


int vfprintf(FILE *file, const char *fmt, va_list ap)
{
	struct file_emitter e = { {EMIT_OK, file_emit_func}, NULL };
	abort_unless(file && fmt);
	e.file = file;
	return emit_vformat(&e.emitter, fmt, ap);
}


int vsnprintf(char *buf, size_t len, const char *fmt, va_list ap)
{
	int rlen;
	struct string_emitter se;
	char tbuf[1] = { '\0' };

	abort_unless(buf && fmt);
	if ( len <= 1 )
		return 0;

	if ( buf )
		string_emitter_init(&se, buf, len);
	else
		string_emitter_init(&se, tbuf, 1);
	rlen = emit_vformat(&se.se_emitter, fmt, ap);
	string_emitter_terminate(&se);
	
	return rlen;
}


int vsprintf(char *buf, const char *fmt, va_list ap)
{
	return vsnprintf(buf, (size_t)~0, fmt, ap);
}


int snprintf(char *buf, size_t len, const char *fmt, ...)
{
	struct string_emitter se;
	va_list ap;
	int rlen;
	
	abort_unless(buf && fmt);
	if ( len <= 1 )
		return 0;

	va_start(ap, fmt);
	string_emitter_init(&se, buf, len);
	rlen = emit_vformat(&se.se_emitter, fmt, ap);
	string_emitter_terminate(&se);
	va_end(ap);

	return rlen;
}


int sprintf(char *buf, const char *fmt, ...)
{
	int rlen;
	va_list ap;

	va_start(ap, fmt);
	rlen = vsprintf(buf, fmt, ap);
	va_end(ap);

	return rlen;
}


int fgetc(FILE *file)
{
	/* TODO: not yet implemented */
	return EOF;
}


/* --------- initialize all cat standard library manually -------- */

void catlibc_reset(struct catlibc_cfg *cprm)
{
	if ( cprm->heap_base != NULL && cprm->heap_sz > 0 ) {
		dynmem_init(&dmheap);
		dynmem_add_pool(&dmheap, cprm->heap_base, cprm->heap_sz);
	} else {
		dmheap.dm_init = 0;
	}

	exit_status = 0;

	stdin = &File_table[0];
	stdin->f_buffer = cprm->stdin_buf;
	stdin->f_buflen = cprm->stdin_bufsz;
	stdin->f_fill = 0;
	stdin->f_flags = _IOFBF|CAT_SIO_READ|CAT_SIO_BUFGIVEN;
	stdin->f_fd = CAT_STDIN_FILENO;
	stdin->f_lastop = 0;

	stdout = &File_table[1];
	stdout->f_buffer = cprm->stdout_buf;
	stdout->f_buflen = cprm->stdout_bufsz;
	stdout->f_fill = 0;
	stdout->f_flags =_IOFBF|CAT_SIO_WRITE|CAT_SIO_BUFGIVEN;
	stdout->f_fd = CAT_STDOUT_FILENO;
	stdout->f_lastop = 0;

	stderr = &File_table[2];
	stderr->f_buffer = cprm->stderr_buf;
	stderr->f_buflen = cprm->stderr_bufsz;
	stderr->f_fill = 0;
	stderr->f_flags =_IONBF|CAT_SIO_WRITE|CAT_SIO_BUFGIVEN;
	stderr->f_fd = CAT_STDERR_FILENO;
	stderr->f_lastop = 0;

	if ( MAXFILES > 3 )
		memset(&File_table[3], 0, sizeof(FILE) * (MAXFILES - 3));
}

#endif /* !CAT_USE_STDLIB */
