/*
 * stdarg.h -- variable argument macros
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __catstdarg_h
#define __catstdarg_h


/* stdarg.h -- a best guess: works for x86 */
typedef char *va_list;
#define __CAT_ASIZ sizeof(int)
#define __CAT_ROUND(x)	((((x) + __CAT_ASIZ - 1) / __CAT_ASIZ) * __CAT_ASIZ)

#define va_start(ap, last)	((ap)=(char*)&(last)+__CAT_ROUND(sizeof(last)))
#define va_arg(ap, type)						\
	((sizeof(type) < sizeof(int)) ?					\
	 ((ap) += sizeof(int), *((type *)((ap) - sizeof(int)))) :	\
	 ((ap) += sizeof(type), *((type *)((ap) - sizeof(type)))))
	 
#define va_copy(dst, src)	((dst) = (src))
#define __va_copy(d,s)		va_copy(d,s)
#define va_end(ap)		((void) 0)

#endif /* __catstdarg_h */
