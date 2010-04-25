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
#include <cat/mem.h>
#include <stdio.h>

struct catstr {
	size_t	cs_size;
	size_t	cs_dlen;
	char	cs_dynamic;
	char *	cs_data;
};

#define cs_alloc_size(dsiz)	((dsiz) + 1)
#define cs_to_cstr(cs)		((cs)->cs_data)
#define cs_isfull(cs)		((cs)->cs_dlen == (cs)->cs_size)
#define CS_MAXLEN		(((size_t)~0) - 3)
#define CS_ERROR		(CS_MAXLEN + 1)
#define CS_NOTFOUND		(CS_MAXLEN + 2)

#define CS_DECLARE(name, len)	CS_DECLARE_Q(, name, len)
#define CS_DECLARE_Q(qual, name, len)	    			        \
qual char __csbuf__##name[cs_alloc_size(len)+sizeof(struct catstr)]={0};\
qual struct catstr name = { (len), 0, 0, __csbuf__##name }

void   cs_init(struct catstr *cs, char *data, size_t size, int data_is_str);
void   cs_clear(struct catstr *cs);
size_t cs_set_cstr(struct catstr *cs, const char *cstr);
size_t cs_trunc_d(struct catstr *cs, size_t len);
size_t cs_concat_d(struct catstr *toappend, const struct catstr *scnd);
size_t cs_copy_d(struct catstr *dst, struct catstr *src);
size_t cs_format_d(struct catstr *dst, const char *fmt, ...);
size_t cs_find_cc(const struct catstr *cs, char ch);
size_t cs_span_cc(const struct catstr *cs, const char *accept);
size_t cs_cspan_cc(const struct catstr *cs, const char *reject);

size_t cs_find_uc(const struct catstr *cs, const char *utf8ch);
size_t cs_span_uc(const struct catstr *cs, const char *utf8accept, int nc);
size_t cs_cspan_uc(const struct catstr *cs, const char *utf8reject, int nc);

struct catstr *cs_alloc(size_t len);
void           cs_free(struct catstr *cs);

size_t cs_find(const struct catstr *findin, const struct catstr *find);
size_t cs_find_str(const struct catstr *findin, const char *s);
size_t cs_find_raw(const struct catstr *findin, const struct raw *r);

struct catstr *cs_copy_from_chars(const char *s);
struct catstr *cs_format(const char *fmt, ...);
struct catstr *cs_copy(struct catstr *src);
int cs_concat(struct catstr *dst, struct catstr *src);
int cs_grow(struct catstr *cs, size_t maxlen);
int cs_addch(struct catstr *cs, char ch);
struct catstr *cs_substr(const struct catstr *cs, size_t off, size_t len);
size_t         cs_rev_off(const struct catstr *cs, size_t roff);

#if CAT_USE_STDLIB
int cs_fd_readline(int fd, struct catstr **csp);
/* TODO: move out of here once we have input in the nolibc version*/
int cs_file_readline(FILE *file, struct catstr **csp);
#endif /* CAT_USE_STDLIB */

/* returns -1 if there are outstanding dynamic strings with the current mm */
void cs_setmm(struct memmgr *mm);

#endif /* __cat_catstr_h */
