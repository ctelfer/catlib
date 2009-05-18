#ifndef __cat_stdio_h
#define __cat_stdio_h
#include <cat/cat.h>

#if !CAT_USE_STDLIB
#include <cat/catstdlib.h>

typedef struct {
  char *	f_buffer;
  size_t	f_buflen;
  size_t	f_fill;
  int		f_flags;
  int		f_fd;
  int		f_lastop;
} FILE;


#define BUFSIZ			4096
#define _IOUNUSED		0x0
#define _IONBF			0x1
#define _IOLBF			0x2
#define _IOFBF			0x3
#define _IOBFMASK		0x3
#define EOF			-1
#define CAT_SIO_READ		0x10
#define CAT_SIO_WRITE		0x20
#define CAT_SIO_RDWR		(CAT_SIO_READ|CAT_SIO_WRITE)
#define CAT_SIO_BINARY		0x40
#define CAT_SIO_EOF		0x100
#define CAT_SIO_ERR		0x200
#define CAT_SIO_BUFGIVEN	0x400


FILE *fopen(const char *path, const char *mode);
int   fflush(FILE *stream);
int   fclose(FILE *stream);

void  clearerr(FILE *stream);
int   feof(FILE *stream);
int   ferror(FILE *stream);
int   fileno(FILE *stream);

size_t fwrite(const void *ptr, size_t msiz, size_t nmem, FILE *file);
int   fputc(int c, FILE *stream);
int   fputs(const char *s, FILE *stream);
int   putc(int c, FILE *stream);
int   putchar(int c);
int   puts(const char *s);

void  setbuf(FILE *stream, char *buf);
void  setbuffer(FILE *stream, char *buf, size_t size);
void  setlinebuf(FILE *file);
int   setvbuf(FILE *file, char *buf, int mode, size_t bsiz);

int   printf(const char *format, ...);
int   fprintf(FILE *stream, const char *format, ...);
int   vprintf(const char *format, va_list ap);
int   vfprintf(FILE *stream, const char *format, va_list ap);

/* not implemented */
int   fgetc(FILE *stream);


extern FILE *stdin, *stdout, *stderr;


#endif /* !CAT_USE_STDLIB */

#endif /* __cat_stdio_h */
