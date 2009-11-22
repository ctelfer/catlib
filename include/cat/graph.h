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
	union attrib_u		grn_u;
};
#define gr_node_val		grn_u.au_value
#define gr_node_ptr		grn_u.au_pointer
#define gr_node_data		grn_u.au_data


struct gr_edge {
	struct list		entry;
	struct gr_node *	n1;
	struct gr_node *	n2;
	union attrib_u		gre_u;
};
#define gr_edge_val		gre_u.au_value
#define gr_edge_ptr		gre_u.au_pointer
#define gr_edge_data		gre_u.au_data


struct graph {
	struct list 	        nodes;
	struct list	        edges;
	int		        isbi;
	uint			nodex;
	uint			edgex;
	struct memmgr *		mm;
};


struct graph *   gr_new(struct memmgr *mm, int isbi, uint nxsize, uint exsize);
struct gr_node * gr_add_node(struct graph *g);
struct gr_edge * gr_add_edge(struct gr_node *src, struct gr_node *dst);
struct gr_edge * gr_find_edge(struct gr_node *from, struct gr_node *to);
void gr_del_node(struct gr_node *node);
void gr_del_edge(struct gr_edge *edge);
void gr_free(struct graph *g);
struct gr_node * gr_edge_dst(const struct gr_node *node, 
		             const struct gr_edge *edge);
	
#endif /* __graph_h */
