/*
 * cat/ex.h -- Exceptions via longjmp
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#ifndef __CAT_EX_H
#define __CAT_EX_H

#include <cat/cat.h>
#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB

#include <setjmp.h>

#ifndef MAXEXSTK
#define MAXEXSTK	64
#endif /* MAXEXSTK */


struct ex_ent {
	jmp_buf		env;
	void *		data;
};


struct ex_stack {
	int		top;
	void 		(*uncaught)(void *);
	struct ex_ent	stack[MAXEXSTK];
};


int 	ex_try(void);
void	ex_throw(void *data);
void *	ex_get(void);
void	ex_end(void);
void	ex_setu(void (*uncaught)(void *));

#endif /* CAT_USE_STDLIB */

#endif /* __CAT_EX_H */
