/*
 * cat/optparse.h -- Command-line option parsing
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009, See accompanying license
 *
 */

#ifndef __cat_aux_h
#define __cat_aux_h

#include <cat/cat.h>

#define CLOT_NOARG	0
#define CLOT_STRING	1
#define CLOT_INT	2
#define CLOT_UINT	3
#define CLOT_DOUBLE	4

#define CLOERR_NONE	0
#define CLOERR_UNKOPT	1
#define CLOERR_NOPARAM	2
#define CLOERR_BADPARAM	3

struct cli_opt {
	int		type;
	char 		ch;
	const char *	str;
	const char *	desc;
	int		present;
	const char *	arg;
	scalar_t	val; /* for non string/no-arg types */
};

#define CLOPT_INIT(type, ch, str, desc) { type, ch, str, desc, 0, NULL, 0 }

struct cli_parser {
	struct cli_opt *	options;
	unsigned		num;
	const char *		eval;
	int			etype;
	int			eopt;  /* option # for CLOERR_NO/BADPARAM */
				       /* option char when eidx < 0 */
};

#define CLIPARSE_INIT(optarr, arrlen) { optarr, arrlen, NULL, 0, 0 }

int  parse_options(struct cli_parser *clp, int argc, char *argv[]);
void print_options(struct cli_parser *clp, char *str, size_t ssize);

#endif /* __cat_optparse_h */
