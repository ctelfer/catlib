#include <stdio.h>
#include <string.h>
#include <cat/shell.h>
#include <cat/stduse.h>
#include <cat/str.h>

int func(struct shell_env *env, int nargs, char *args[], struct shell_value *r);

struct shell_cmd_entry cmds[] = { 
	{"ival", func},
	{"uval", func},
	{"dval", func},
	{"sval", func},
	{"pval", func},
	{"help", func},
};

DECLARE_SHELL_ENV(env, cmds);


int func(struct shell_env *env, int nargs, char *args[], struct shell_value *rv)
{
	int nrequired = 1;
	printf("function f (int) -- %d\n", *(int*)env->se_ctx);

	if ( strcmp(args[0], "help") == 0 ) 
		nrequired = 0;
	if ( nargs != nrequired+1 ) {
		printf("need %d argument got %d\n", nrequired, nargs-1);
		return -1;
	}

	if ( strcmp(args[0], "ival") == 0 ) {
		int ival;
		printf("arg = /%s/\n", args[1]);
		if ( shell_arg2int(env, args[1], &ival) < 0 )
			goto err;
		printf("Integer val: %d\n", ival);
		rv->sval_type = SVT_INT;
		rv->sval_int = ival;
		rv->sval_free = NULL;
	} else if ( strcmp(args[0], "uval") == 0 ) {
		unsigned int uval;
		if ( shell_arg2uint(env, args[1], &uval) < 0 )
			goto err;
		printf("Unsigned Integer val: %u\n", uval);
		rv->sval_type = SVT_UINT;
		rv->sval_uint = uval;
		rv->sval_free = NULL;
	} else if ( strcmp(args[0], "dval") == 0 ) {
		double dval;
		if ( shell_arg2dbl(env, args[1], &dval) < 0 )
			goto err;
		printf("Double val: %f\n", dval);
		rv->sval_type = SVT_DOUBLE;
		rv->sval_dbl = dval;
		rv->sval_free = NULL;
	} else if ( strcmp(args[0], "sval") == 0 ) {
		char *sval;
		if ( shell_arg2str(env, args[1], &sval) < 0 )
			goto err;
		printf("String val: %s\n", sval);
		rv->sval_type = SVT_STRING;
		rv->sval_str = str_copy_a(sval);
		rv->sval_mm = &estdmm;
		rv->sval_free = mem_free;
	} else if ( strcmp(args[0], "pval") == 0 ) {
		void *pval;
		if ( shell_arg2ptr(env, args[1], &pval) < 0 )
			goto err;
		printf("Pointer val: %p\n", pval);
		rv->sval_type = SVT_PTR;
		rv->sval_ptr = pval;
		rv->sval_free = NULL;
	} else {
		printf("Commands: ival, uval, dval, sval, pval\n");
		printf("Syntax: [VAR=] <cmd> <value>\n");
		printf("Ctrl-D (on UNIX) ends the program.\n\n");
	}

	return 0;

err:
	return -1;

}


int main(int argc, char *argv[])
{
	char line[256];
	unsigned int ctx = 0;

	env.se_ctx = &ctx;
	while ( fgets(line, sizeof(line), stdin) ) {
		++ctx;
		if ( line[0] != '\0' && line[strlen(line)-1] != '\n' ) {
			fprintf(stderr, "line too long!\n");
			break;
		}
		if ( shell_run(&env, line) < 0 )
			fprintf(stderr, "Error running line\n");
	}

	return 0;
}
