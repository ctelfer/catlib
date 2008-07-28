#include <cat/cat.h>
#include <stdarg.h>

#if !CAT_USE_STDLIB
#include <cat/catstdio.h>
#include <cat/emit.h>
#include <cat/emit_format.h>
#include <cat/catstdlib.h>

/* This is the machine dependent part */
#if CAT_HAS_POSIX
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAXFILES	32
static char Stdin_buf[BUFSIZ];
static char Stdout_buf[BUFSIZ];
static FILE File_Table[MAXFILES] = { 
	{ Stdin_buf, sizeof(Stdin_buf), 0, _IOFBF|CAT_SIO_READ|CAT_SIO_BUFGIVEN,
	      	0, 0 },
	{ Stdout_buf, sizeof(Stdout_buf), 0, 
		_IOFBF|CAT_SIO_WRITE|CAT_SIO_BUFGIVEN, 1, 0 },
	{ NULL, 0, 0, _IONBF|CAT_SIO_WRITE, 2, 0 },
	{ 0 }
};


static int mode_to_flags(const char *mode, int *oflags, int *fflags)
{
	abort_unless(mode && oflags && fflags);
	if ( *mode == 'r' ) {
		if ( *(mode + 1) == '+' ) {
			*oflags = O_RDWR;
			*fflags = CAT_SIO_RDWR;
		} else {
			if ( *(mode + 1) != '\0' )
				return -1;
			*oflags = O_RDONLY;
			*fflags = CAT_SIO_READ;
		}
	} else if ( *mode == 'w' ) {
		if ( *(mode + 1) == '+' ) {
			*oflags = O_RDWR | O_CREAT | O_TRUNC;
			*fflags = CAT_SIO_RDWR;
		} else {
			if ( *(mode + 1) != '\0' )
				return -1;
			*oflags = O_WRONLY | O_CREAT | O_TRUNC;
			*fflags = CAT_SIO_WRITE;
		}
	} else if ( *mode == 'a' ) {
		if ( *(mode + 1) == '+' ) {
			*oflags = O_RDWR | O_CREAT | O_APPEND;
			*fflags = CAT_SIO_RDWR;
		} else {
			if ( *(mode + 1) != '\0' )
				return -1;
			*oflags = O_WRONLY | O_CREAT | O_APPEND;
			*fflags = CAT_SIO_WRITE;
		}
	} else
		return -1;

	return 0;
}


/* used by frepoen() and fdopen() */
static int posix_open_file(FILE *fp, const char *path, const char *mode)
{
	int oflags, fflags;
	int fd;

	abort_unless(fp && path);
	if ( mode == NULL )
		return -1;

	if ( mode_to_flags(mode, &oflags, &fflags) < 0 )
		return -1;

	if ( (fd = open(path, oflags)) < 0 )
		return -1;

	fp->f_fill   = 0;
	fp->f_lastop = 0;
	fp->f_fd     = fd;
	fp->f_flags  = fflags;

	return 0;
}


FILE *fopen(const char *path, const char *mode)
{
	FILE *fp, *end = File_Table + MAXFILES;

	/* XXX Not reentrant */
	for ( fp = File_Table ; fp < end ; ++fp )
		if ( (fp->f_flags & _IOBFMASK) == _IOUNUSED )
			break;
	if ( fp == end )
		return NULL;

	if ( posix_open_file(fp, path, mode) < 0 ) {
		fp->f_flags = _IOUNUSED;
		return NULL;
	}

	fp->f_buffer = NULL;
	fp->f_buflen = 0;
	
	return fp;
}


FILE *freopen(const char *path, const char *mode, FILE *file)
{
	if ( fflush(file) < 0 )
		return NULL;

	if ( close(file->f_fd) < 0 )
		return NULL;

	if ( posix_open_file(file, path, mode) < 0 )
		return NULL;

	return file;
}


static int flush_all()
{
	FILE *fp, *end = File_Table + MAXFILES;
	for ( fp = File_Table ; fp < end ; ++fp )
		if ( fflush(fp) < 0 )
			return -1;
	return 0;
}


static int writen(int fd, const char *buf, int len)
{
	int nwritten = 0;
	ssize_t nw;

	abort_unless(buf && len <= INT_MAX);

	while ( nwritten < len ) {
		nw = write(fd, buf + nwritten, len - nwritten);
		if ( nw == -1 ) {
			if ( errno == EINTR )
				continue;
			return -1;
		} 
		nwritten += nw;
	}

	return nwritten;
}


/* This is the routine where actual buffering takes place */
static size_t do_fput_bytes(FILE* file, const char *buf, size_t len)
{
	size_t nwritten = 0, towrite, bspace;
	ssize_t nw;
	const char *p;
	const char *start;
	int writeout;

	if ( !(file->f_flags & CAT_SIO_WRITE) || 
	     (file->f_flags & CAT_SIO_EOF) ||
	     (file->f_flags & CAT_SIO_ERR) )
		return 0;
	abort_unless(file);
	abort_unless(file->f_buflen < INT_MAX);

	if ( file->f_buffer == NULL && (file->f_flags & _IOBFMASK) != _IONBF ) {
		if ( (file->f_buffer = malloc(file->f_buflen)) == NULL )
			return 0;
	}

	if ( file->f_lastop != CAT_SIO_WRITE ) {
		if ( fflush(file) < 0 )
			return 0;
	}
	file->f_lastop = CAT_SIO_WRITE;

	while ( nwritten < len ) {
		if ( (file->f_flags & _IOBFMASK) == _IONBF ) {
			towrite = (towrite > INT_MAX) ? INT_MAX : towrite;
			if ( (nw = writen(file->f_fd, buf, towrite)) == -1 )
				return 0;
			nwritten += nw;
			continue;
		} 

		writeout = 0;
		bspace = file->f_buflen - file->f_fill;
		start = buf + nwritten;

		if ( (file->f_flags & _IOBFMASK) == _IOLBF ) {
			for ( p = start; p < buf + len ; ++p )
				if ( *p == '\n' ) {
					writeout = 1;
					++p;
					break;
				}
			towrite = p - start;
		} else { 
			abort_unless((file->f_flags & _IOBFMASK) == _IOFBF);
			towrite = len - nwritten;
		}

		if ( towrite > bspace ) {
			towrite = bspace;
			writeout = 1;
		}

		memmove(file->f_buffer + file->f_fill, start, towrite);
		file->f_fill += towrite;
		nwritten += towrite;

		if ( writeout ) {
			nw = writen(file->f_fd, file->f_buffer, file->f_fill);
			if ( nw == -1 )
				return 0;
			abort_unless(nw == file->f_buflen);
			file->f_fill = 0;
		}
	}

	return nwritten;
}


int fflush(FILE *file)
{
	if ( file == NULL )
		return flush_all();

	if ( file->f_lastop == CAT_SIO_WRITE ) {
		if ( file->f_fill > 0 && 
		     do_fput_bytes(file, file->f_buffer, file->f_fill) < 0 )
			return -1;
		file->f_fill = 0;
	} else {
		/* TODO: once we get read operations */
	}

	return 0;
}


int fclose(FILE *file)
{
	if ( fflush(file) < 0 )
		return -1;

	if ( close(file->f_fd) < 0 )
		return -1;

	if ( file->f_buffer != NULL && 
	     (file->f_flags & CAT_SIO_BUFGIVEN) == 0 ) {
		free(file->f_buffer);
	}
	file->f_buffer = NULL;
	file->f_buflen = 0;
	file->f_flags = _IOUNUSED;

	return 0;
}


FILE *stdin  = &File_Table[0];
FILE *stdout = &File_Table[1];
FILE *stderr = &File_Table[2];

#else /* CAT_HAS_POSIX */

static FILE dummy_FILE;

FILE *fopen(const char *path, const char *mode)
{
	return &dummy_FILE;
}

int fflush(FILE *file)
{
	return 0;
}


int fclose(FILE *file)
{
	return 0;
}


static size_t do_fput_bytes(FILE* file, const char *buf, size_t len)
{
	abort_unless(file && buf);
	file->f_lastop = CAT_SIO_WRITE;
	return len;
}

FILE *stdout = &dummy_FILE;
FILE *stderr = &dummy_FILE;

#endif /* CAT_HAS_POSIX */

void clearerr(FILE *file)
{
	abort_unless(file);
	file->f_flags &= ~CAT_SIO_ERR;
}


int feof(FILE *file)
{
	abort_unless(file);
	return (file->f_flags & CAT_SIO_EOF) != 0;
}


int ferror(FILE *file)
{
	abort_unless(file);
	return (file->f_flags & CAT_SIO_ERR) != 0;
}


int fileno(FILE *file)
{
	abort_unless(file);
	return file->f_fd;
}


size_t fwrite(const void *ptr, size_t msiz, size_t nmem, FILE *file)
{
	size_t i, rv;
	const char *buf = ptr;

	abort_unless(ptr && file);

	if ( msiz == 0 || nmem == 0 )
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
	abort_unless(file);
	rv = do_fput_bytes(file, &ch, sizeof(ch));
	if ( rv < 0 )
		return EOF;
	return c;
}


int fputs(const char *s, FILE *file)
{
	size_t rv;
	abort_unless(s && file);
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
	abort_unless(file);
	setvbuf(file, NULL, _IOLBF, 0);
}


int setvbuf(FILE *file, char *buf, int mode, size_t bsiz)
{
	abort_unless(file);
	abort_unless(mode == _IONBF || mode == _IOLBF || mode == _IOFBF);

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
	return emit_format(&e.emitter, fmt, ap);
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
	rlen = emit_format(&se.se_emitter, fmt, ap);
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
	rlen = emit_format(&se.se_emitter, fmt, ap);
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


#endif /* !CAT_USE_STDLIB */
