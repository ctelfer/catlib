/*
 * emit_format.h -- Formatted output over the emit interface.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_emit_format_h
#define __cat_emit_format_h

#include <cat/cat.h>
#include <cat/emit.h>
#include <stdarg.h>

int emit_format(struct emitter *em, const char *fmt, ...);
int emit_vformat(struct emitter *em, const char *fmt, va_list ap);

enum {
	FMT_INTVAL = 0x0,
	FMT_DBLVAL = 0x1,
	FMT_STRVAL = 0x2,
	FMT_PTRVAL = 0x3,

	FMT_REGSIZE  = 0x00,
	FMT_HALFSIZE = 0x10,
	FMT_BYTESIZE = 0x20,
	FMT_LONGSIZE = 0x30,
	FMT_LONGLONGSIZE = 0x40
};

#define FMT_BASETYPE(type) ((type) & 0x0F)
#define FMT_SIZETYPE(type) ((type) & 0xF0)

int emit_format_getprm(const char *fmt, uchar ptypes[], int maxpt);
int emit_format_ckprm(const char *fmt, uchar ptypes[], int npt);

#endif /* __cat_emit_format_h */
