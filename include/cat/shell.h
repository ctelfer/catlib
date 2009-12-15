#ifndef __shell_h
#define __shell_h
#include <cat/cat.h>
#include <cat/mem.h>

#define CAT_MAX_CMD_ARGS 32
#define CAT_MAX_VARS	 32
#define CAT_MAX_VARNLEN	 32

struct shell_env;


enum { 
	SVT_NIL = 0,
	SVT_INT,
	SVT_UINT,
	SVT_DOUBLE,
	SVT_STRING,
	SVT_PTR
};

struct shell_value {
	int		sval_type;
	scalar_t	sval_val;
	struct memmgr *	sval_mm;
	free_f		sval_free;
};
#define sval_int	sval_val.int_val
#define sval_uint	sval_val.uint_val
#define sval_dbl	sval_val.dbl_val
#define sval_str	sval_val.ptr_val
#define sval_ptr	sval_val.ptr_val


typedef int (*shell_cmd_f)(struct shell_env *env, int nargs, char *args[],
		           struct shell_value *rv);


struct shell_cmd_entry {
	char *			cmd_name;
	shell_cmd_f		cmd_func;
};

struct shell_var {
	char 			sv_name[CAT_MAX_VARNLEN];
	struct shell_value	sv_value;
};

struct shell_env {
	struct shell_cmd_entry *se_cmds;
	uint			se_ncmds;
	struct shell_var	se_vars[CAT_MAX_VARS];
	void *			se_ctx;
};

#define DECLARE_SHELL_ENV(name, cmdarray) 				 \
struct shell_env name = { cmdarray,					 \
			  sizeof(cmdarray)/sizeof(struct shell_cmd_entry),\
			  { 0 }, NULL }

void shell_env_init(struct shell_env *env, struct shell_cmd_entry *ents,
		    unsigned nents, void *ctx);
int  shell_parse(char *str, char **retvar, char *args[CAT_MAX_CMD_ARGS]);
int  shell_run(struct shell_env *env, char *cmdstr);

int  shell_is_var(const char *str);
struct shell_var *shell_find_var(struct shell_env *env, const char *str);
void shell_free_var(struct shell_var *sv);

int  shell_arg2int(struct shell_env *env, const char *str, int *rv);
int  shell_arg2uint(struct shell_env *env, const char *str, uint *rv);
int  shell_arg2dbl(struct shell_env *env, const char *str, double *rv);
int  shell_arg2str(struct shell_env *env, char *str, char **rv);
int  shell_arg2ptr(struct shell_env *env, char *str, void **rv);

#endif /* __shell_h */
