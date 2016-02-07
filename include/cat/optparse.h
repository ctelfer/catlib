/*
 * cat/optparse.h -- Command-line option parsing
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009-2012 -- See accompanying license
 *
 */

#ifndef __cat_optparse_h
#define __cat_optparse_h

#include <cat/cat.h>

#define CLOPT_NOARG	0
#define CLOPT_STRING	1
#define CLOPT_INT	2
#define CLOPT_UINT	3
#define CLOPT_DOUBLE	4

#define CLORET_OPTION	0
#define CLORET_UNKOPT	-1
#define CLORET_NOPARAM	-2
#define CLORET_BADPARAM	-3
#define CLORET_RESET	-4

struct clopt {
	int		type;
	char 		ch;
	const char *	str;
	const char *	argnam;
	const char *	desc;
	attrib_t	val;
};

#define CLOPT_INIT(type, ch, str, desc) { type, ch, str, NULL, desc, {0} }
#define CLOPT_I_NOARG(ch, str, desc) \
	{ CLOPT_NOARG, (ch), (str), NULL, (desc), {0} }
#define CLOPT_I_STRING(ch, str, name, desc) \
	{ CLOPT_STRING, (ch), (str), (name), (desc), {0} }
#define CLOPT_I_INT(ch, str, name, desc) \
	{ CLOPT_INT, (ch), (str), (name), (desc), {0} }
#define CLOPT_I_UINT(ch, str, name, desc) \
	{ CLOPT_UINT, (ch), (str), (name), (desc), {0} }
#define CLOPT_I_DOUBLE(ch, str, name, desc) \
	{ CLOPT_DOUBLE, (ch), (str), (name), (desc), {0} }

struct clopt_parser {
	struct clopt *	options;
	size_t		num;
	int		argc;
	char **		argv;
	int		vidx;
	int 		non_opt;
	const char *	chptr;
	char 		errbuf[128];
};

#define CLOPTPARSER_INIT(_optarr, _arrlen) \
	{ (_optarr), (_arrlen), 0, NULL, 0, 0, NULL, { 0 } }

#define CLOPTPARSER_INIT_ARR(_optarr) \
	CLOPTPARSER_INIT((_optarr), array_length(_optarr))

/* returns 0 if the parser, argc, and argv are well formatted */
int optparse_reset(struct clopt_parser *clp, int argc, char *argv[]);

/* returns 0 on an option, < 0 on err, (CLORET_*), > 0 on index after options */
int optparse_next(struct clopt_parser *clp, struct clopt **p);

/* prints up to slen -1 characters: always null terminates */
void optparse_print(struct clopt_parser *clp, char *str, size_t slen);

/* format the option name into a string */
char *clopt_name(struct clopt *opt, char *buf, size_t buflen);

#endif /* __cat_optparse_h */
