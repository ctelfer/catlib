#include <stdio.h>
#include <string.h>

#include <cat/graph.h>
#include <cat/str.h>
#include <cat/stduse.h>
#include <cat/shell.h>

int gnew(struct shell_env *env, int na, char *args[], struct shell_value *rv);
int gdel(struct shell_env *env, int na, char *args[], struct shell_value *rv);
int add_node(struct shell_env *env, int na, char *args[],
	     struct shell_value *rv);
int del_node(struct shell_env *env, int na, char *args[],
	     struct shell_value *rv);
int edge(struct shell_env *env, int na, char *args[], struct shell_value *rv);
int find(struct shell_env *env, int na, char *args[], struct shell_value *rv);
int print(struct shell_env *env, int na, char *args[], struct shell_value *rv);

struct shell_cmd_entry cmds[] = { 
	{"gnew", gnew},
	{"gdel", gdel},
	{"node", add_node},
	{"ndel", del_node},
	{"edge", edge},
	{"edel", edge},
	{"print", print},
};

DECLARE_SHELL_ENV(env, cmds);


/* Assumption: we'll never need more than INT_MAX graph nodes in the */
/* graph at a time */
struct tgraph { 
	struct graph *graph;
	struct htab *node_tab;
};


int gnew(struct shell_env *env, int na, char *args[], struct shell_value *rv)
{
	int isbi;
	struct tgraph *g;

	if ( na != 2 || (shell_arg2int(env, args[1], &isbi) < 0)  ) {
		fprintf(stderr, "usage: gnew <is_bidirectional>\n");
		return -1;
	}

	g = emalloc(sizeof(*g));
	g->graph = gr_new(&estdmm, isbi);
	g->node_tab = ht_new(32, CAT_DT_NUM);

	rv->sval_type = SVT_PTR;
	rv->sval_ptr = g;
	rv->sval_mm = NULL;
	rv->sval_free = NULL;

	return 0;
}


int gdel(struct shell_env *env, int na, char *args[], struct shell_value *rv)
{
	void *p;
	struct tgraph *g;
	int i;

	if ( na != 2 || (shell_arg2ptr(env, args[1], &p) < 0)  ) {
		fprintf(stderr, "usage: gdel <graph>\n");
		return -1;
	}
	g = p;

	gr_free(g->graph);
	ht_free(g->node_tab);
	free(g);

	rv->sval_type = SVT_NIL;
	rv->sval_int = 0;
	rv->sval_mm = NULL;
	rv->sval_free = NULL;

	return 0;
}


int add_node(struct shell_env *env, int na, char *args[],
	     struct shell_value *rv)
{
	int id;
	struct gr_node *n;
	void *p;
	struct tgraph *g;
	scalar_t sid;

	if ( na != 3 || 
	     (shell_arg2ptr(env, args[1], &p) < 0) ||
	     (shell_arg2int(env, args[2], &id) < 0) ) {
		fprintf(stderr, "usage: node <graph> <id>\n");
		return -1;
	}
	g = p;

	if ( ht_get(g->node_tab, (void *)id) ) {
		fprintf(stderr, "Node %d already exists in the graph\n", id);
		return -1;
	}

	sid.int_val = id;
	n = gr_add_node(g->graph, sid);
	ht_put(g->node_tab, (void *)id, n);
	printf("Created node %d\n", id);

	rv->sval_type = SVT_PTR;
	rv->sval_ptr = n;
	rv->sval_mm = NULL;
	rv->sval_free = NULL;

	return 0;
}


int del_node(struct shell_env *env, int na, char *args[],
	     struct shell_value *rv)
{
	int id;
	struct gr_node *n;
	void *p;
	struct tgraph *g;

	if ( (na != 3) || 
	     (shell_arg2ptr(env, args[1], &p) < 0) ||
	     (shell_arg2int(env, args[2], &id) < 0) ) {
		fprintf(stderr, "usage: ndel <graph> <nid>\n");
		return -1;
	}
	g = p;

	n = ht_get(g->node_tab, (void *)id);
	if ( !n ) {
		fprintf(stderr, "couldn't find node %d\n", id);
		return -1;
	}

	gr_del_node(n);
	ht_clr(g->node_tab, (void *)id);

	rv->sval_type = SVT_NIL;
	rv->sval_ptr = NULL;
	rv->sval_mm = NULL;
	rv->sval_free = NULL;

	return 0;
}


int edge(struct shell_env *env, int na, char *args[], struct shell_value *rv)
{
	int id1, id2;
	scalar_t id = { 0 };
	struct gr_node *n1, *n2;
	struct gr_edge *e;
	struct tgraph *g;
	void *p;


	if ( (na != 4) || 
	     (shell_arg2ptr(env, args[1], &p) < 0) ||
	     (shell_arg2int(env, args[2], &id1) < 0) ||
	     (shell_arg2int(env, args[3], &id2) < 0) ) {
		fprintf(stderr, "usage: %s <graph> <nid> <nid>\n", args[0]);
		return -1;
	}
	g = p;

	n1 = ht_get(g->node_tab, (void *)id1);
	if ( !n1 ) {
		fprintf(stderr, "couldn't find node %d\n", id1);
		return -1;
	}

	n2 = ht_get(g->node_tab, (void *)id2);
	if ( !n2 ) {
		fprintf(stderr, "couldn't find node %d\n", id2);
		return -1;
	}

	if ( e = gr_find_edge(n1, n2) ) {
		if ( strcmp(args[0], "edge") == 0 ) {
			fprintf(stderr, "edge %d,%d already exists\n", 
				id1, id2);
			return -1;
		} else {
			gr_del_edge(e);
		}
	} else {
		if ( strcmp(args[0], "edel") == 0 ) {
			fprintf(stderr, "couldn't find edge %d,%d\n", 
				id1, id2);
			return -1;
		} else {
			gr_add_edge(n1, n2, id);
		}
	}

	rv->sval_type = SVT_NIL;
	rv->sval_ptr = NULL;
	rv->sval_mm = NULL;
	rv->sval_free = NULL;

	return 0;
}


int print(struct shell_env *env, int na, char *args[], struct shell_value *rv)
{
	struct list *le;
	struct gr_node *n;
	struct gr_edge **e, **eend;
	int first;
	void *p;
	struct tgraph *g;

	if ( na != 2 || (shell_arg2ptr(env, args[1], &p) < 0)  ) {
		fprintf(stderr, "usage: print <graph>\n");
		return -1;
	}
	g = p;

	for ( le = l_head(&g->graph->nodes); 
	      le != l_end(&g->graph->nodes); 
	      le = le->next ) {
		n = container(le, struct gr_node, entry);
		printf("Node %d: ", n->decor.int_val);
		e = n->out.arr;
		eend = e + n->out.fill;
		first = 1;
		for ( ; e < eend ; ++e ) {
			if ( first ) {
				first = 0;
			} else {
				printf(", ");
			}
			printf("%d", gr_edge_dst(n, *e)->decor.int_val);
		}
		printf("\n");
	}

	return 0;
}


int main(int argc, char *argv[])
{
	char line[256];

	printf("> ");
	while ( fgets(line, sizeof(line), stdin) ) {
		if ( line[0] != '\0' && line[strlen(line)-1] != '\n' ) {
			fprintf(stderr, "line too long!\n");
			break;
		}
		shell_run(&env, line);
		printf("> ");
	}

	return 0;
}
