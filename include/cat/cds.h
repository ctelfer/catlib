/*
 * cat/cds.h -- Container data structure macros.
 *   Macros for creating simple container nodes for different data types.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009, See accompanying license
 *
 */

#ifndef __cat_cds_h
#define __cat_cds_h

#include <cat/cat.h>
#ifdef CAT_USE_STDLIB
#include <stdlib.h>
#else /* CAT_USE_STDLIB */
#include <catstdlib.h>
#endif

#define CDS_NEWSTRUCT(_ds, _dt, _name)			\
struct _name {						\
	_ds	node;					\
	_dt	data;					\
}


#define CDS_NEW0(_pvar)					\
	do { 						\
		(_pvar) = calloc(1, sizeof(*_pvar));	\
	} while (0)
	

#define CDS_NEW(_pvar, _val)				\
	do { 						\
		(_pvar) = calloc(1, sizeof(*_pvar));	\
		(_pvar)->data = (_val);			\
	} while (0)

#define CDS_FREE(_pvar)					\
	do { 						\
		free(_pvar);				\
	} while (0)


#define CDS_DATA(_np, _name)	\
	(container((_np), struct _name, node)->data)

#define CDS_DPTR(_np)		\
	(&container((_np), struct _name, node)->data)


#endif /* __cat_cds_h */
