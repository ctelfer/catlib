#ifndef __cat_str_h
#define __cat_str_h

#include <cat/cat.h>
#include <stdarg.h>

uchar  chnval(char digit);

size_t str_copy(char *dst, const char *src, size_t dlen);
size_t str_cat(char *dst, const char *src, size_t dlen);
int    str_fmt(char *s, size_t len, const char *fmt, ...);
int    str_vfmt(char *s, size_t len, const char *fmt, va_list ap); 

/* 
 * Useful parsing function since v6 addresses have zero-compression, etc.
 * The return value goes in 'addr'.  It must refer to 16 bytes.
 * Returns the index of the first character after the address in the string
 * or -1 if there is an error.  The address is stored in network byte order
 * (MSB first).
 */
int str_parse_ip6a(void *addr, const char *src);


/* UTF conversion routines */
extern const signed char utf8_lentab[256];
#define utf8_nbytes(c) (utf8_lentab[(uchar)(c)])
int utf8_find_nbytes(const uchar firstc);
char * utf8_findc(const char *s, const char *c);
size_t utf8_span(const char *s, const char *accept);
size_t utf8_cspan(const char *s, const char *reject);
int utf8_validate_char(const char *str, size_t max);
int utf8_validate_str(const char *str, char const **errp);
int utf8_validate(const char *str, size_t slen, char const **errp, int term);
size_t utf8_nchars(const char *str, size_t slen, int *maxlen);
int utf8_to_utf32(ulong *dst, size_t dlen, const char *s, size_t slen);
int utf8_to_utf16(ushort *dst, size_t dlen, const char *s, size_t slen);
int utf32_to_utf8(char *dst, size_t dlen, const ulong *s, size_t slen);
int utf16_to_utf8(char *dst, size_t dlen, const ushort *src, size_t sl);
char * utf8_skip(char *start, size_t nchar);
char * utf8_skip_tck(char *start, size_t nchar);

#endif /* __cat_str_h */
