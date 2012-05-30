/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/optparse.h>
#include <cat/err.h>
#include <stdio.h>
#include <ctype.h>

struct clopt g_optarr[] = { 
	CLOPT_INIT(CLOPT_NOARG, 'a', "--opt-a", "option A (no arg)"),
	CLOPT_INIT(CLOPT_STRING, 'b', "--opt-b", "option B (string)"),
	CLOPT_INIT(CLOPT_INT, 'c', "--opt-c", "option C (int)"),
	CLOPT_INIT(CLOPT_UINT, 'd', "--opt-d", "option D (unsigned int)"),
	CLOPT_INIT(CLOPT_DOUBLE, 'e', "--opt-e", "option E (double)"),
	CLOPT_INIT(CLOPT_NOARG, 0, "--opt-f", "option F (no arg, no char)"),
	CLOPT_INIT(CLOPT_INT, 'g', NULL, "option G (int, no string)"),
	CLOPT_INIT(CLOPT_NOARG, 'h', "--help", "print help")
};

struct clopt_parser g_options = 
	CLOPTPARSER_INIT(g_optarr, array_length(g_optarr));


static void print_option(struct clopt *opt)
{
	char ons[64];
	const char *optname = clopt_name(opt, ons, sizeof(ons));
	switch(opt->type) {
	case CLOPT_NOARG:
		printf("Option %s is set\n", optname);
		break;
	case CLOPT_STRING:
		printf("Option %s is set to '%s'\n", optname, opt->val.str_val);
		break;
	case CLOPT_INT:
		printf("Option %s is set to '%d'\n", optname, opt->val.int_val);
		break;
	case CLOPT_UINT:
		printf("Option %s is set to '%u'\n", optname,opt->val.uint_val);
		break;
	case CLOPT_DOUBLE:
		printf("Option %s is set to '%f'\n",optname,opt->val.dbl_val); 
		break;
	default:
		err("Corrupted option %s", optname);
	}
}


int main(int argc, char *argv[])
{
	int argi;
	char buf[4096];
	struct clopt *opt;

	optparse_print(&g_options, buf, sizeof(buf));
	printf("usage: %s [options] [args]\n%s\n", argv[0], buf);

	optparse_reset(&g_options, argc, argv);
	while ( !(argi = optparse_next(&g_options, &opt)) ) {
		print_option(opt);
	}
	if ( argi < 0 ) {
		err("Error -- %s\n", g_options.errbuf);
		return -1;
	}

	for ( ; argi < argc ; ++argi )
		printf("Argument: '%s'\n", argv[argi]);

	return 0;
}
