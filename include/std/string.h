#ifndef __cat_string_h
#define __cat_string_h

#include <cat/cat.h>
#include <stdarg.h>

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
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strdup(const char *s);

#endif /* !CAT_USE_STDLIB */

#endif /* __cat_string_h */
