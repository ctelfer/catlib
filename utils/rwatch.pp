#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cat/err.h>
#include <cat/emalloc.h>
#include <cat/stduse.h>
#include <cat/raw.h>
#include <cat/list.h>
#include <cat/pspawn.h>

enum etype { STRING, INT, FLOAT, RATE };

struct elem {
	struct list ln;
	int type;
	char *str;
	unsigned long rate;
};
#define ln2e(_l) container(_l, struct elem, ln)

struct list before_elems;
struct list after_elems;

void add_elem(struct raw *s, void *ctx, int type)
{
	struct list *curlist = ctx;
	struct elem *e = emalloc(sizeof(*e));

	e->str = erawsdup(s);
	e->type = type;
	l_enq(curlist, &e->ln);
}


%%

start <- elem*

elem <- floatnum { add_elem(cpeg_text, cpeg_ctx, FLOAT); }
        / intnum { add_elem(cpeg_text, cpeg_ctx, INT); } 
        / string { add_elem(cpeg_text, cpeg_ctx, STRING); }

floatnum <- [1-9][0-9]* '.' [1-9][0-9]*
intnum   <- [1-9][0-9]* / '0x' [0-9a-fA-F]+ / '0' [0-7]+ / '0'
string   <- (![0-9] .)+

%%


uint duration = 10;
extern char **environ;


void parse_cmd_out(char **argv, struct list *linelist)
{
	struct pspawn *ps;
	struct cpeg_parser parser;

	l_init(linelist);
	cpeg_init(&parser, NULL);
	ps = ps_spawn_x("1+2", argv, environ);
	if ( ps == NULL )
		errsys("error running command: ");
	if ( cpeg_parse(&parser, ps_get_locfile(ps, 1), linelist) < 0 )
		err("Error parsing input from command\n");
	cpeg_fini(&parser);
	ps_cleanup(ps, 1);
}


int get_rate(struct elem *be, struct elem *ae)
{
	if ( be->type != ae->type )
		return -1;
	if ( strcmp(be->str, ae->str) == 0 )
		return 0;
	if ( be->type == INT ) {
		unsigned long bv, av;
		sscanf(be->str, "%lu", &bv);
		sscanf(ae->str, "%lu", &av);
		ae->rate = (av - bv) / duration;
		ae->type = RATE;
	} else if ( be->type == FLOAT ) {
		double bv, av;
		sscanf(be->str, "%lf", &bv);
		sscanf(ae->str, "%lf", &av);
		ae->rate = (av - bv) / duration;
		ae->type = RATE;
	}
	return 0;
}


void get_rates(struct list *blist, struct list *alist)
{
	struct list *bln, *aln;
	struct elem *be, *ae;

	bln = l_head(blist);
	l_for_each( aln, alist ) {
		if ( bln == l_end(blist) )
			break;
		if ( get_rate(ln2e(bln), ln2e(aln)) < 0 )
			break;
		bln = l_next(bln);
	}
}


void print_rates(struct list *alist)
{
	struct list *aln;
	struct elem *e;

	l_for_each( aln, alist ) {
		e = ln2e(aln);
		if ( e->type == RATE )
			printf("(%lu / sec)", e->rate);
		else
			fputs(e->str, stdout);
	}
}


void usage(char *prog)
{
	err("usage: %s [-d duration] command arg1 arg2...\n", prog);
}


int main(int argc, char *argv[])
{
	int i = 2;
	int arg = 1;
	char **cmd;
	char *cp;

	if ( argc < 2 || strcmp(argv[1], "-h") == 0 )
		usage(argv[0]);

	if ( strcmp(argv[1], "-d") == 0 ) {
		if ( argc < 4 )
			usage(argv[0]);
		duration = strtoul(argv[2], &cp, 0);
		if ( cp == argv[2] || *cp != '\0' )
			usage(argv[0]);
		arg = 3;
	}

	cmd = emalloc(sizeof(char *) * (argc + 2));
	cmd[0] = "/bin/sh";
	cmd[1] = "-c";
	while ( (cmd[i++] = argv[arg++]) != NULL ) ; 

	ps_ignore_sigcld();
	parse_cmd_out(cmd, &before_elems);
	sleep(duration);
	parse_cmd_out(cmd, &after_elems);

	get_rates(&before_elems, &after_elems);
	print_rates(&after_elems);

	return 0;
}
