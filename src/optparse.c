/*
 * optparse.c -- Command line option parsing
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/optparse.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>


int optparse_reset(struct clopt_parser *clp, int argc, char *argv[])
{
	size_t n;
	struct clopt *opt;

	if ( clp == NULL || argc < 1 || argv == NULL )
		return -1;

	for ( n = 0; n < clp->num ; ++n ) {
		opt = &clp->options[n];
		if ( opt->type < CLOPT_NOARG || opt->type > CLOPT_DOUBLE )
			return -1;
		if ( !isalnum(opt->ch) && opt->str == NULL )
			return -1;
	}

	clp->argc = argc;
	clp->argv = argv;
	clp->vidx = 1;
	clp->non_opt = 1;
	clp->used_arg = 0;
	clp->chptr = NULL;

	return 0;
}

static void movedown(char *argv[], int lo, int hi)
{
	char *hold = argv[hi];
	if ( lo < hi ) {
		memmove(&argv[lo + 1], &argv[lo], (hi - lo) * sizeof(char *));
		argv[lo] = hold;
	}
}


static int read_arg(struct clopt *opt, const char *arg)
{
	char *cp;
	switch(opt->type) {
	case CLOPT_STRING:
		opt->val.str_val = (char *)arg;
		break;
	case CLOPT_INT:
		opt->val.int_val = strtol(arg, &cp, 0);
		if ( cp == arg || *cp != '\0' || *arg == '\0' )
			return -1;
		break;
	case CLOPT_UINT:
		opt->val.uint_val = strtoul(arg, &cp, 0);
		if ( cp == arg || *cp != '\0' || *arg == '\0' )
			return -1;
		break;
	case CLOPT_DOUBLE:
		opt->val.dbl_val = strtod(arg, &cp);
		if ( cp == arg || *cp != '\0' || *arg == '\0' )
			return -1;
		break;
	default:
		abort_unless(0);
	}

	return 0;
}


char *clopt_name(struct clopt *opt, char *buf, size_t buflen)
{
	int n = 0;
	if ( buflen < 1 )
		return NULL;
	*buf = '\0';
	if ( isalnum(opt->ch) ) {
		if ( buflen < 3 )
			return buf;
		buf[0] = '-'; buf[1] = opt->ch;
		if ( opt->str == NULL ) {
			buf[2] = '\0';
			return buf;
		} else {
			buf[2] = ',';
			n = 3;
		} 
	}
	if ( opt->str != NULL )
		snprintf(buf + n, buflen - n, "%s", opt->str);
	return buf;
}


static int parse_long_opt(struct clopt_parser *clp, struct clopt **optp)
{
	int i; 
	const char *cp = clp->argv[clp->vidx], *end;
	size_t elen;
	struct clopt *opt = NULL;
	char onamestr[64];

	end = strchr(cp, '=');
	if ( end != NULL )
		elen = end - cp;
	else
		elen = strlen(cp);

	for ( i = 0; i < clp->num; ++i ) {
		opt = &clp->options[i];
		if ( opt->str != NULL && strncmp(opt->str, cp, elen) == 0 )
			break;
	}

	if ( opt == NULL ) {
		*optp = NULL;
		clp->argc = -1;
		snprintf(clp->errbuf, sizeof(clp->errbuf), 
			 "Unknown option: %s", cp);
		return CLORET_UNKOPT;
	}

	*optp = opt;
	if ( opt->type != CLOPT_NOARG ) {
		if ( end == NULL ) {
			clp->argc = -1;
			snprintf(clp->errbuf, sizeof(clp->errbuf), 
				 "No parameter for option %s", 
				 clopt_name(opt, onamestr, sizeof(onamestr)));
			return CLORET_NOPARAM;
		}
		if ( read_arg(opt, end + 1) < 0 ) {
			clp->argc = -1;
			snprintf(clp->errbuf, sizeof(clp->errbuf), 
				 "Bad parameter for option %s: %s", 
				 clopt_name(opt, onamestr, sizeof(onamestr)),
				 end + 1);
			opt->val.str_val = (char *)end + 1;
			return CLORET_BADPARAM;
		}
	} else if ( end != NULL ) {
		clp->argc = -1;
		snprintf(clp->errbuf, sizeof(clp->errbuf), 
			 "Unwanted parameter for option %s: %s", 
			 clopt_name(opt, onamestr, sizeof(onamestr)),
			 end + 1);
		opt->val.str_val = (char *)end + 1;
		return CLORET_BADPARAM;
	}
	movedown(clp->argv, clp->non_opt++, clp->vidx++);

	return 0;
}


static int parse_short_opts(struct clopt_parser *clp, struct clopt **optp)
{
	int i;
	struct clopt *opt = NULL;
	const char *cp = clp->chptr;
	char onamestr[64];

	for ( i = 0; i < clp->num ; ++i ) {
		opt = &clp->options[i];
		if ( isalnum(opt->ch) && opt->ch == *cp )
			break;
	}
	if ( i >= clp->num ) {
		*optp = NULL;
		clp->argc = -1;
		snprintf(clp->errbuf, sizeof(clp->errbuf), 
			 "Unknown option: -%c", *cp);
		return CLORET_UNKOPT;
	}
	*optp = opt;
	if ( opt->type != CLOPT_NOARG ) {
		if ( clp->vidx >= clp->argc - 1 || clp->used_arg ) {
			clp->argc = -1;
			snprintf(clp->errbuf, sizeof(clp->errbuf), 
				 "No parameter for option %s", 
				 clopt_name(opt, onamestr, sizeof(onamestr)));
			return CLORET_NOPARAM;
		}
		if ( read_arg(opt, clp->argv[clp->vidx+1]) < 0 ) {
			clp->argc = -1;
			snprintf(clp->errbuf, sizeof(clp->errbuf), 
				 "Bad parameter for option %s: %s", 
				 clopt_name(opt, onamestr, sizeof(onamestr)),
				 clp->argv[clp->vidx+1]);
			opt->val.str_val = clp->argv[clp->vidx+1];
			return CLORET_BADPARAM;
		}
		clp->used_arg = 1;
	}
	if ( *++clp->chptr == '\0' ) {
		movedown(clp->argv, clp->non_opt++, clp->vidx++);
		if ( clp->used_arg )
			movedown(clp->argv, clp->non_opt++, clp->vidx++);
		clp->chptr = NULL;
	}

	return 0;
}


int optparse_next(struct clopt_parser *clp, struct clopt **optp)
{
	int i;
	char **argv;

	abort_unless(clp);
	abort_unless(optp);

	if ( clp->argc <= 0 ) {
		snprintf(clp->errbuf, sizeof(clp->errbuf), 
			 "Must call optparse_reset() upon error return code");
		return CLORET_RESET;
	}

	if ( clp->vidx < 0 )
		return clp->non_opt;

	if ( clp->chptr != NULL )
		return parse_short_opts(clp, optp);

	i = clp->vidx;
	argv = clp->argv;
	while ( (i < clp->argc) && ((argv[i][0] != '-')||(argv[i][1] == '\0')) )
		i = ++clp->vidx;
		
	if ( i >= clp->argc )
		return clp->non_opt;
	if ( strcmp(argv[i], "--") == 0 ) {
		movedown(clp->argv, clp->non_opt++, clp->vidx);
		clp->vidx = -1;
		return clp->non_opt;
	}

	if ( strncmp(argv[i], "--", 2) == 0 ) {
		return parse_long_opt(clp, optp);
	} else {
		clp->used_arg = 0;
		clp->chptr = &argv[i][1];
		return parse_short_opts(clp, optp);
	}
}


static void adj_str(char **cp, size_t *remlen, size_t *linelen, size_t added)
{
	if ( added < *remlen ) {
		*remlen -= added;
		(*cp) += added;
	} else {
		(*cp) += *remlen;
		added = *remlen;
		*remlen = 0;
	}
	*linelen += added;
}


void optparse_print(struct clopt_parser *clp, char *str, size_t ssize)
{
	int i;
	char *cp = str;
	char pad[80];
	size_t fsz, lsz;
	struct clopt *opt;

	abort_unless(ssize > 1);
	if ( ssize == 0 ) {
		return;
	} else if ( ssize == 1 ) {
		*str = '\0';
		return;
	}
	ssize -= 1;
	for ( i = 0; i < clp->num; ++i ) { 
		opt = &clp->options[i];
		if ( !isalnum(opt->ch) && opt->str == NULL )
			continue;
		lsz = 0;
		if ( isalnum(opt->ch) ) {
			fsz = (size_t)snprintf(cp, ssize, "        -%c%s",
					       opt->ch,
					       (opt->str == NULL) ? "" : ", ");
			adj_str(&cp, &ssize, &lsz, fsz);
		} else {
			fsz = (size_t)snprintf(cp, ssize, "        ");
			adj_str(&cp, &ssize, &lsz, fsz);
		}
		if ( opt->str != NULL ) {
			fsz = (size_t)snprintf(cp, ssize, "%s", opt->str);
			adj_str(&cp, &ssize, &lsz, fsz);
		}
		if ( lsz < 40 ) {
			memset(pad, ' ', 40-lsz);
			pad[40-lsz] = '\0';
			fsz = (size_t)snprintf(cp, ssize, "%s", pad);
			adj_str(&cp, &ssize, &lsz, fsz);
		} else {
			fsz = (size_t)snprintf(cp, ssize, " ");
			adj_str(&cp, &ssize, &lsz, fsz);
		}
		fsz = (size_t)snprintf(cp, ssize, "%s\n", opt->desc);
		adj_str(&cp, &ssize, &lsz, fsz);
	}
}

