#include <cat/optparse.h>
#if CAT_USE_STDLIB
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <cat/catstdlib.h>
#include <cat/catstdio.h>
#endif


static void movedown(char *argv[], int lo, int hi)
{
	char *hold = argv[hi];
	if ( lo < hi ) {
		memmove(&argv[lo + 1], &argv[lo], (hi - lo) * sizeof(char *));
		argv[lo] = hold;
	}
}


static int type_is_valid(int type) 
{
	return (type >= CLOT_NOARG && type <= CLOT_DOUBLE);
}


static int read_arg(struct cli_opt *opt, const char *arg)
{
	char *cp;
	switch(opt->type) {
	case CLOT_STRING:
		opt->arg = arg;
		break;
	case CLOT_INT:
		opt->val.int_val = strtol(arg, &cp, 0);
		if ( cp == arg || *cp != '\0' || *arg == '\0' )
			return -1;
		break;
	case CLOT_UINT:
		opt->val.uint_val = strtoul(arg, &cp, 0);
		if ( cp == arg || *cp != '\0' || *arg == '\0' )
			return -1;
		break;
	case CLOT_DOUBLE:
		opt->val.uint_val = strtod(arg, &cp);
		if ( cp == arg || *cp != '\0' || *arg == '\0' )
			return -1;
		break;
	default:
		abort_unless(0);
	}

	return 0;
}


static int parse_long_opt(const char *cp, struct cli_parser *clp)
{
	int i; 
	const char *end;
	size_t elen;
	struct cli_opt *opt = NULL;

	cp += 2;
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
		clp->eval = cp - 2;
		clp->etype = CLOERR_UNKOPT;
		return -1;
	}

	if ( opt->type != CLOT_NOARG ) {
		if ( end == NULL ) {
			clp->etype = CLOERR_NOPARAM;
			clp->eopt = i;
			return -1;
		}
		if ( read_arg(opt, end + 1) < 0 ) {
			clp->eval = end + 1;
			clp->etype = CLOERR_BADPARAM;
			clp->eopt = i;
			return -1;
		}
	}
	opt->present = 1;

	return 0;
}


static int parse_short_opts(const char *cp, const char *next, 
		            struct cli_parser *clp)
{
	int i;
	int used_arg = 0;
	struct cli_opt *opt = NULL;
	cp += 1;
	while ( *cp != '\0' ) {
		for ( i = 0; i < clp->num ; ++i ) {
			opt = &clp->options[i];
			if ( isalnum(opt->ch) && opt->ch == *cp )
				break;
		}
		if ( i >= clp->num ) {
			clp->eval = cp;
			clp->etype = CLOERR_UNKOPT;
		}
		if ( opt->type != CLOT_NOARG ) {
			if ( next == NULL || used_arg ) {
				clp->eval = cp;
				clp->etype = CLOERR_NOPARAM;
				clp->eopt = i;
				return -1;
			}
			if ( read_arg(opt, next) < 0 ) {
				clp->eval = next;
				clp->etype = CLOERR_BADPARAM;
				clp->eopt = i;
				return -1;
			}
		}
		opt->present = 1;
		++cp;
	}

	return used_arg;
}


int parse_options(struct cli_parser *clp, int argc, char *argv[])
{
	int i, non_opt = 1, rv;

	abort_unless(argv);
	abort_unless(clp);

	for ( i = 0; i < clp->num; ++i ) {
		abort_unless(type_is_valid(clp->options[i].type));
		abort_unless(clp->options[i].ch > 0 || 
			     (clp->options[i].str != NULL && 
			      strcmp(clp->options[i].str, "--") != 0));
		clp->options[i].present = 0;
	}
	clp->etype = 0;
	clp->eopt = 0;
	clp->eval = NULL;

	for ( i = 1; i < argc ; i++ ) {
		abort_unless(argv[i] != NULL);
		if ( (argv[i][0] != '-') || (argv[i][1] == '\0') )
			continue;
		if ( strcmp(argv[i], "--") == 0 ) {
			movedown(argv, non_opt, i);
			break;
		}
		if ( strncmp(argv[i], "--", 2) == 0 ) {
			movedown(argv, non_opt++, i);
			rv = parse_long_opt(argv[non_opt-1], clp);
			if ( rv != 0 )
				return -1;
		} else {
			rv = parse_short_opts(argv[i], 
					     (i < argc - 1) ? argv[i+1] : NULL, 
					     clp);
			if ( rv < 0 )
				return -1;
			movedown(argv, non_opt++, i);
			if ( rv > 0 )
				movedown(argv, non_opt++, ++i);
		}
	}

	return non_opt;
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


void print_options(struct cli_parser *clp, char *str, size_t ssize)
{
	int i;
	char *cp = str;
	char pad[80];
	size_t fsz, lsz;
	struct cli_opt *opt;

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
			fsz = (size_t)snprintf(cp, ssize, "-%c%s", opt->ch,
					       (opt->str == NULL) ? "" : ", ");
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

