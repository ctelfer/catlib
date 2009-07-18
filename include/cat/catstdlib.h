#ifndef __catstdlib_h
#define __catstdlib_h

#include <cat/cat.h>
#include <stdarg.h>

/* We use the mem* functions even if we don't use the standard library */
#if !CAT_USE_STDLIB

/* string.h */
int memcmp(const void *b1p, const void *b2p, size_t len);
void *memcpy(void *dst, const void *src, size_t len);
void *memmove(void *dst, const void *src, size_t len);
void *memset(void *dst, int c, size_t len);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t max);
char *strstr(const char *str, const char *pat);
char *strchr(const char *s, int ch);
char *strrchr(const char *s, int ch);
char *strcpy(char *dst, const char *src);
char *strdup(const char *s);

/* ctype.h */
int isalnum(int c);
int isdigit(int c);
int isxdigit(int c);
int isspace(int c);
int isalpha(int c);
int islower(int c);
int isupper(int c);
int toupper(int c);
int tolower(int c);

/* stdlib.h */
long strtol(const char *start, char **cp, int base);
ulong strtoul(const char *start, char **cp, int base);
double strtod(const char *start, char **cp);

void *malloc(size_t len);
void *calloc(size_t nmem, size_t ilen);
void *realloc(void *omem, size_t len);
void free(void *mem);


/* stdio.h */
int snprintf(char *buf, size_t len, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int vsnprintf(char *buf, size_t len, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);

/* TODO: in the near future
 * errno
 */

void exit(int status);
void abort(void);

#endif /* !CAT_USE_STDLIB */

#endif /* __catstdlib_h */
