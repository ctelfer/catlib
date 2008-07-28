/*
 * cat/regex.h -- simplified UNIX regular expressions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */
#ifndef __CAT_REGEX_H
#define __CAT_REGEX_H

#include <sys/types.h>
#include <cat/cat.h>
#include <regex.h>

int re_find(const char *s, const char *p, regmatch_t *m, int nm); 

int re_lastrep(const char *r);

int re_replen(const char *s, const char *r, regmatch_t *m);

int re_replace(const char *s, const char *r, const regmatch_t *m, char *d);


#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB

#ifndef CAT_RE_MAXSUB
#define CAT_RE_MAXSUB	20
#endif /* CAT_RE_MAXSUB */

char * re_sr(const char *s, const char *p, const char *r);

#endif /* CAT_USE_STDLIB */

#endif /* __CAT_REGEX_H */
