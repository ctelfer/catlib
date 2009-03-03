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


static void handle_error(struct clopt_parser *clp, struct clopt *opt, int etype)
{
	switch(etype) {
	case CLORET_UNKOPT:
		if ( *g_options.errval != '-' )
			err("Unknown option -%c\n", *g_options.errval);
		else
			err("Unknown option %s\n", *g_options.errval);
		break;
	case CLORET_NOPARAM:
		if ( isalnum(opt->ch) )
			err("No parameter for -%c\n", opt->ch);
		else
			err("No parameter for %s\n", opt->str);
		break;
	case CLORET_BADPARAM:
		if ( isalnum(opt->ch) )
			err("Bad parameter for -%c: %s\n", 
			    opt->ch, clp->errval);
		else
			err("Bad parameter for %s: %s\n",
			    opt->str, clp->errval);
		break;
	default:
		err("unkown error type");
	}
}


static const char *optstr(struct clopt *opt)
{
	static char str[16];
	if ( isalnum(opt->ch) ) {
		sprintf(str, "-%c", opt->ch);
		return str;
	}
	else {
		return opt->str;
	}
}


static void print_option(struct clopt *opt)
{
	switch(opt->type) {
	case CLOPT_NOARG:
		printf("Option %s is set\n", optstr(opt));
		break;
	case CLOPT_STRING:
		printf("Option %s is set to '%s'\n", optstr(opt), 
			opt->val.str_val);
		break;
	case CLOPT_INT:
		printf("Option %s is set to '%d'\n", optstr(opt), 
		       opt->val.int_val);
		break;
	case CLOPT_UINT:
		printf("Option %s is set to '%u'\n", optstr(opt), 
		       opt->val.uint_val);
		break;
	case CLOPT_DOUBLE:
		printf("Option %s is set to '%lf'\n", optstr(opt), 
		       opt->val.dbl_val);
		break;
	default:
		err("Corrupted option %s", optstr(opt));
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
		handle_error(&g_options, opt, argi);
		return -1;
	}

	for ( ; argi < argc ; ++argi )
		printf("Argument: '%s'\n", argv[argi]);

	return 0;
}
