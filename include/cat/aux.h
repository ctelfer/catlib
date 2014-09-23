/*
 * cat/aux.h -- Auxiliary functions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007-2014 -- See accompanying license
 *
 */

#ifndef __cat_aux_h
#define __cat_aux_h

#include <cat/cat.h>

/* These NEED to be extern so that function pointers can point to them */
extern int cmp_ptr(const void *, const void *);
extern int cmp_str(const void *, const void *);
extern int cmp_raw(const void *, const void *);


ulong uldivmod(ulong dend, ulong dsor, int div);
#if CAT_HAS_LONGLONG
ullong ulldivmod(ullong dend, ullong dsor, int div);
#endif

#endif /* __cat_aux_h */
