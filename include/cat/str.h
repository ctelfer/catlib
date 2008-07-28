#ifndef __cat_str_h
#define __cat_str_h

#include <cat/cat.h>

#if CAT_USE_STDLIB
#include <stdarg.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */

size_t str_copy(char *dst, const char *src, size_t dlen);
size_t str_cat(char *dst, const char *src, size_t dlen);
int    str_fmt(char *s, size_t len, const char *fmt, ...);
int    str_vfmt(char *s, size_t len, const char *fmt, va_list ap); 
char * str_findc(const char *s, const char *c);
size_t str_span(const char *s, const char *accept);
size_t str_cspan(const char *s, const char *reject);

/* UTF conversion routines */
extern const signed char utf8_lentab[256];
#define utf8_nbytes(c) (utf8_lentab[(unsigned char)(c)])
int utf8_find_nbytes(const unsigned char firstc);

int utf8_validate_char(const char *str, size_t max);
int utf8_validate_str(const char *str, char const **errp);
int utf8_validate(const char *str, size_t slen, char const **errp, int term);
size_t utf8_nchars(const char *str, size_t slen, int *maxlen);
int utf8_to_utf32(unsigned long *dst, size_t dlen, const char *s, size_t slen);
int utf8_to_utf16(unsigned short *dst, size_t dlen, const char *s, size_t slen);
int utf32_to_utf8(char *dst, size_t dlen, const unsigned long *s, size_t slen);
int utf16_to_utf8(char *dst, size_t dlen, const unsigned short *src, size_t sl);
char * utf8_skip(char *start, size_t nchar);
char * utf8_skip_tck(char *start, size_t nchar);

#endif /* __cat_str_h */
