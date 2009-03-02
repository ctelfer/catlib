#include <cat/optparse.h>
#include <cat/err.h>
#include <stdio.h>
#include <ctype.h>

struct cli_opt g_optarr[] = { 
	CLOPT_INIT(CLOT_NOARG, 'a', "--opt-a", "option A (no arg)"),
	CLOPT_INIT(CLOT_STRING, 'b', "--opt-b", "option B (string)"),
	CLOPT_INIT(CLOT_INT, 'c', "--opt-c", "option C (int)"),
	CLOPT_INIT(CLOT_UINT, 'd', "--opt-d", "option D (unsigned int)"),
	CLOPT_INIT(CLOT_DOUBLE, 'e', "--opt-e", "option E (double)"),
	CLOPT_INIT(CLOT_NOARG, 0, "--opt-f", "option F (no arg, no char)"),
	CLOPT_INIT(CLOT_INT, 'g', NULL, "option G (int, no string)"),
	CLOPT_INIT(CLOT_NOARG, 'h', "--help", "print help")
};

struct cli_parser g_options = CLIPARSE_INIT(g_optarr, array_length(g_optarr));


static void handle_error()
{
	switch(g_options.etype) {
	case CLOERR_UNKOPT:
		if ( *g_options.eval != '-' )
			err("Unknown option -%c\n", *g_options.eval);
		else
			err("Unknown option %s\n", *g_options.eval);
		break;
	case CLOERR_NOPARAM:
		if ( *g_options.eval != '-' )
			err("No parameter for -%c\n", *g_options.eval);
		else
			err("No parameter for %s\n", *g_options.eval);
		break;
	case CLOERR_BADPARAM:
		if ( *g_options.eval != '-' )
			err("Bad parameter for -%c: %s\n", 
			    g_options.options[g_options.eopt].ch,
			    g_options.eval);
		else
			err("Bad parameter for %s: %s\n",
			    g_options.options[g_options.eopt].str,
			    g_options.eval);
		break;
	default:
		err("unkown error type");
	}
}


static const char *optstr(struct cli_opt *opt)
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


static void print_option(struct cli_opt *opt)
{
	if ( !opt->present )
		return;
	switch(opt->type) {
	case CLOT_NOARG:
		printf("Option %s is set\n", optstr(opt));
		break;
	case CLOT_STRING:
		printf("Option %s is set to '%s'\n", optstr(opt), opt->arg);
		break;
	case CLOT_INT:
		printf("Option %s is set to '%d'\n", optstr(opt), 
		       opt->val.int_val);
		break;
	case CLOT_UINT:
		printf("Option %s is set to '%u'\n", optstr(opt), 
		       opt->val.uint_val);
		break;
	case CLOT_DOUBLE:
		printf("Option %s is set to '%lf'\n", optstr(opt), 
		       opt->val.dbl_val);
		break;
	default:
		err("Corrupted option %s", optstr(opt));
	}
}


int main(int argc, char *argv[])
{
	int argi, i;
	char buf[4096];

	print_options(&g_options, buf, sizeof(buf));
	printf("usage: %s [options] [args]\n%s", argv[0], buf);

	argi = parse_options(&g_options, argc, argv);
	if ( argi < 0 )
		handle_error();
	for ( i = 0; i < array_length(g_optarr); ++i )
		print_option(&g_optarr[i]);
	for ( ; argi < argc ; ++argi )
		printf("Argument: '%s'\n", argv[argi]);
	return 0;
}
