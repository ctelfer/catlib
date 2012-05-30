/*
 * cat/raw.h -- raw blob manipulation functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#ifndef __CAT_RAW_H
#define __CAT_RAW_H

#include <cat/cat.h>

int          raw_cmp(void *r1, void *r2);
struct raw * str_to_raw(struct raw *r, char *s, int terminate);


#endif /* __CAT_RAW_H */
