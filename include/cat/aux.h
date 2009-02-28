/*
 * cat/aux.h -- Auxiliary functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007, See accompanying license
 *
 */

#ifndef __cat_aux_h
#define __cat_aux_h

#include <cat/cat.h>

/* These NEED to be extern so that function pointers can point to them */
extern int cmp_ptr(const void *, const void *);
extern int cmp_str(const void *, const void *);
extern int cmp_raw(const void *, const void *);

#endif /* __cat_aux_h */
