/*
 * catstr.h -- application level strings
 *
 * by Christopher Adam Telfer
 *   
 * Copyright 2007, 2008 See accompanying license
 *   
 */

#ifndef __cat_catstr_h
#define __cat_catstr_h

#include <cat/cat.h>

#if CAT_USE_STDLIB
#include <stdio.h>
#else
#include <cat/catstdio.h>
#endif

struct catstr {
	size_t		cs_size;
	size_t		cs_dlen;
	unsigned char	cs_data[1];
};

#define cs_alloc_size(dsiz)	(offsetof(struct catstr, cs_data) + dsiz + 1)
#define cs_data_size(asiz)	(asiz - offsetof(struct catstr, cs_data))
#define cs_tlen(cs)		cs_alloc_size((cs).cs_size)
#define CS_MAXLEN		(((size_t)~0) - cs_alloc_size(0) - 1)
#define CS_ERROR		(CS_MAXLEN + 1)
#define CS_NOTFOUND		(CS_MAXLEN + 2)
#define CS_UNINITIALIZED	(CS_MAXLEN + 3)
#define CS_DECLARE(name,len)	\
	unsigned char __csbuf__#name[cs_alloc_size(len)] = { 0 };\
	struct catstr *name = (struct catstr *)&__csbuf__#name;

void   cs_init(struct catstr *cs, size_t size);
size_t cs_set_cstr(struct catstr *cs, const char *cstr);
size_t cs_trunc_d(struct catstr *cs, size_t len);
size_t cs_concat_d(struct catstr *toappend, const struct catstr *scnd);
size_t cs_find_cc(const struct catstr *cs, char ch);
size_t cs_span_cc(const struct catstr *cs, const char *accept);
size_t cs_cspan_cc(const struct catstr *cs, const char *reject);

size_t cs_find_uc(const struct catstr *cs, const char *utf8ch);
size_t cs_span_uc(const struct catstr *cs, const char *utf8accept, int nc);
size_t cs_cspan_uc(const struct catstr *cs, const char *utf8reject, int nc);

#if CAT_USE_STDLIB
size_t cs_find(const struct catstr *findin, const struct catstr *find);
size_t cs_find_str(const struct catstr *findin, const char *s);
/* TODO: move out of here once we have stduse.h in the nolibc version */
size_t cs_find_raw(const struct catstr *findin, const struct raw *r);
#endif /* CAT_USE_STDLIB */

struct catstr *cs_from_chars(const char *s);
struct catstr *cs_format(const char *fmt, ...);
struct catstr *cs_concat(struct catstr *first, struct catstr *second);
struct catstr *cs_grow(struct catstr *cs, size_t maxlen);
struct catstr *cs_substr(const struct catstr *cs, size_t off, size_t len);
size_t         cs_rev_off(const struct catstr *cs, size_t roff);
#if CAT_USE_STDLIB
struct catstr *cs_fd_readline(int fd);
/* TODO: move out of here once we have input in the nolibc version*/
struct catstr *cs_file_readline(FILE *file);
#endif /* CAT_USE_STDLIB */

struct catstr *cs_alloc(size_t len);
void           cs_free(struct catstr *cs);

#endif /* __cat_catstr_h */
