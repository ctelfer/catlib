#ifndef __graph_h
#define __graph_h


#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/list.h>


struct gr_edge_arr {
	struct gr_edge **	arr;
	uint			fill;
	uint			len;
};


struct gr_node {
	struct list		entry;
	struct graph *		graph;
	struct gr_edge_arr	out;
	struct gr_edge_arr	in;
	union scalar_u		decor;
};


struct gr_edge {
	struct list		entry;
	struct gr_node *	n1;
	struct gr_node *	n2;
	union scalar_u		decor;
};


struct graph {
	struct list 	        nodes;
	struct list	        edges;
	int		        isbi;
	struct memmgr *		mm;
};


struct graph *   gr_new(struct memmgr *mm, int isbi);
struct gr_node * gr_add_node(struct graph *g, union scalar_u decor);
struct gr_edge * gr_add_edge(struct gr_node *src, struct gr_node *dst, 
		             union scalar_u decor);
struct gr_edge * gr_find_edge(struct gr_node *from, struct gr_node *to);
void gr_del_node(struct gr_node *node);
void gr_del_edge(struct gr_edge *edge);
void gr_free(struct graph *g);
struct gr_node * gr_edge_dst(const struct gr_node *node, 
		             const struct gr_edge *edge);
	
#endif /* __graph_h */
