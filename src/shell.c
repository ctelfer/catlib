#include <cat/cat.h>
#include <cat/shell.h>
#include <cat/str.h>
#include <cat/stduse.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


/* 
 * Possible TODOs
 *   Allow quoted strings
 */


static void shell_free_value(struct shell_value *val)
{
	free_f freep;
	if ( (freep = val->sval_free) )
		(*freep)(val->sval_mm, val->sval_ptr);
}


void shell_env_init(struct shell_env *env, struct shell_cmd_entry *ents,
		    unsigned nents, void *ctx)
{
	uint i;
	abort_unless(env);
	abort_unless(ents);

	env->se_cmds = ents;
	env->se_ncmds = nents;
	memset(env->se_vars, 0, CAT_MAX_VARS * sizeof(env->se_vars[0]));
	/* XXX redundant in most systems */
	for ( i = 0 ; i < CAT_MAX_VARS ; ++i )
		shell_free_var(&env->se_vars[i]);
	env->se_ctx = NULL;
}


int shell_parse(char *str, char **retvar, char *args[CAT_MAX_VARS])
{
	int nargs = 0;
	char *tok, *state = str, *first, *eq;

	*retvar = NULL;

	/* get the first argument*/
	first = str_tok_a(&state, NULL);

	/* if there was none, return 0: a noop */
	if ( !first )
		return 0;

	/* check if the first token was a variable assignment */
	/* proper form is "VARNAME= " */
	if ( (eq = strchr(first, '=')) != NULL ) {
		/* check that the variable name does not start with $ */
		if ( first[0] == '$' )
			return -1;

		/* get rid of the = */
		*eq = '\0';
		*retvar = first;
		if ( *(eq + 1) != '\0' ) {
			args[0] = eq + 1;
			++nargs;
		}
	} else {
		/* if no variable assignment, first token was first argument */
		args[0] = first;
		++nargs;
	}
	
	/* read out the rest of the arguments */
	while ( nargs < CAT_MAX_CMD_ARGS ) {
		tok = str_tok_a(&state, NULL);
		if ( !tok ) {
			if ( *state == '\0' )
				break;
			else
				goto err;
		}
		args[nargs++] = tok;
	}

	/* check if there are more than CAT_MAX_CMD_ARGS and return an error */
	/* if there are */
	if ( (nargs == CAT_MAX_CMD_ARGS) && 
			 (tok = str_tok_a(&state, NULL)) ) {
			 goto err;
	}

	return nargs;
err:
	if ( *retvar )
		*retvar = NULL;
	return -1;
}


static struct shell_var *new_var(struct shell_env *env, const char *vname)
{
	struct shell_var *var, *end;
	size_t slen;

	abort_unless(env && vname);
	abort_unless(vname[0] != '\0');

	slen = strlen(vname);
	if ( slen > CAT_MAX_VARNLEN - 1 )
		return NULL;

	for ( var = env->se_vars, end = var + CAT_MAX_VARS ; var < end ; ++var )
		if ( var->sv_name[0] == '\0' ) {
			memcpy(var->sv_name, vname, slen + 1);
			return var;
		}

	return NULL;
}


struct shell_var *shell_find_var(struct shell_env *env, const char *name)
				 
{
	struct shell_var *var, *end;

	abort_unless(env && name);

	/* simple linear search for the variable entry */
	for ( var = env->se_vars, end = var + CAT_MAX_VARS ; var < end ; ++var )
		if ( strcmp(var->sv_name, name) == 0 )
			return var;

	return NULL;
}


int shell_run(struct shell_env *env, char *cmdstr)
{
	char *args[CAT_MAX_CMD_ARGS] = { 0 };
	char *retvar;
	int nargs, rv = -1;
	struct shell_value result;
	struct shell_cmd_entry *cmd, *end;
	struct shell_var *var = NULL;

	/* parse the line into arguments and possibly a return variable name */
	if ( (nargs = shell_parse(cmdstr, &retvar, args)) < 0 )
		return nargs;

	/* if there is a return variable name look it up */
	if ( retvar ) {
		var = shell_find_var(env, retvar);
		if ( !var ) {
			var = new_var(env, retvar);
			if ( !var )
				return -1;
		}
	}

	/* if there were no arguments it is either a noop or a variable */
	/* clear operation:  handle each accordingly */
	/* XXX note that this can cause a memory leak if not careful :( */
	/* We need to see how to address this: maybe allow variables to be */
	/* reference counted pointers which are automatically destructed */
	/* this is a TODO */
	if ( nargs == 0 ) {
		if ( var )
			shell_free_var(var);
		return 0;
	}
	
	/* find the actual command and run it */
	for ( cmd = env->se_cmds, end = cmd+env->se_ncmds; cmd < end ; ++cmd ) {
		if ( strcmp(cmd->cmd_name, args[0]) == 0 ) {
			/* found it */
			/* run the command, set the var, return success */
			rv = (*cmd->cmd_func)(env, nargs, args, &result);
			if ( var && rv >= 0 ) {
				shell_free_value(&var->sv_value);
				var->sv_value = result;
			}
			break;
		}
	}

	return rv;
}


int shell_is_var(const char *name)
{
	abort_unless(name);
	return name[0] == '$';
}


void shell_free_var(struct shell_var *sv)
{
	struct shell_value *val;

	abort_unless(sv);
	val = &sv->sv_value;
	shell_free_value(val);
	sv->sv_name[0] = '\0';
	val->sval_type = SVT_NIL;
	val->sval_free = NULL;
	val->sval_mm = NULL;
	val->sval_int = 0;
}


int shell_arg2int(struct shell_env *env, const char *str, int *rv)
{
	struct shell_var *v;
	char *endp;
	int ival;

	abort_unless(env && str && rv);

	if ( shell_is_var(str) ) {
		v = shell_find_var(env, str+1);
		if ( !v || v->sv_value.sval_type != SVT_INT )
			return -1;
		*rv = v->sv_value.sval_int;
	} else {
		ival = strtol(str, &endp, 0);
		if ( str == endp )
			return -1;
		*rv = ival;
	}
	return 0;
}


int shell_arg2uint(struct shell_env *env, const char *str, uint *rv)
{
	struct shell_var *v;
	char *endp;
	uint uval;

	abort_unless(env && str && rv);

	if ( shell_is_var(str) ) {
		v = shell_find_var(env, str+1);
		if ( !v || v->sv_value.sval_type != SVT_UINT )
			return -1;
		*rv = v->sv_value.sval_uint;
	} else {
		uval = strtoul(str, &endp, 0);
		if ( str == endp )
			return -1;
		*rv = uval;
	}
	return 0;
}


int shell_arg2dbl(struct shell_env *env, const char *str, double *rv)
{
	struct shell_var *v;
	char *endp;
	double dval;

	abort_unless(env && str && rv);

	if ( shell_is_var(str) ) {
		v = shell_find_var(env, str+1);
		if ( !v || v->sv_value.sval_type != SVT_DOUBLE )
			return -1;
		*rv = v->sv_value.sval_dbl;
	} else {
		dval = strtod(str, &endp);
		if ( str == endp )
			return -1;
		*rv = dval;
	}
	return 0;
}


int shell_arg2str(struct shell_env *env, char *str, char **rv)
{
	struct shell_var *v;

	abort_unless(env && str && rv);

	if ( shell_is_var(str) ) {
		v = shell_find_var(env, str+1);
		if ( !v || v->sv_value.sval_type != SVT_STRING )
			return -1;
		*rv = v->sv_value.sval_ptr;
	} else {
		*rv = str;
	}
	return 0;
}


int shell_arg2ptr(struct shell_env *env, char *str, void **rv)
{
	struct shell_var *v;

	abort_unless(env && str && rv);

	if ( shell_is_var(str) ) {
		v = shell_find_var(env, str+1);
		if ( !v || v->sv_value.sval_type != SVT_PTR )
			return -1;
		*rv = v->sv_value.sval_ptr;

	} else {
		char *endp;
		ulong ul = strtoul(str, &endp, 0);
		if ( str == endp )
			return -1;
		*rv = (void *)ul;
	}

	return 0;
}
