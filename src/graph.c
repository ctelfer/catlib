/*
 * graph.c -- generic graph data structure representation.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/graph.h>

#define CAT_GRAPH_INIT_EDGE_LEN	16

static void del_edge_help(struct gr_edge *edge, struct gr_node *node,
		          int fromout);
static int add_edge_help(struct gr_edge *edge, struct gr_edge_arr *ea,
		         struct gr_edge_arr *ea2);


struct graph *gr_new(struct memmgr *mm, int isbi, uint nxsize, uint exsize)
{
	struct graph *g;
	if ( !mm )
		return NULL;

	nxsize = attrib_csize(struct gr_node, grn_u, nxsize);
	exsize = attrib_csize(struct gr_edge, gre_u, exsize);
	if ( nxsize < sizeof(struct gr_node) || exsize < sizeof(struct gr_edge) )
		return NULL;

	g = mem_get(mm, sizeof(*g));
	if ( !g )
		return NULL;

	l_init(&g->nodes);
	l_init(&g->edges);
	g->isbi = isbi;
	g->mm = mm;
	g->nsize = nxsize;
	g->esize = exsize;

	return g;
}


struct gr_node * gr_add_node(struct graph *g)
{
	struct gr_node *n;

	abort_unless(g && g->mm);

	n = mem_get(g->mm, g->nsize);
	if ( !n )
		return NULL;
	n->graph = g;
	n->out.fill = n->in.fill = 0;
	n->out.len = n->in.len = CAT_GRAPH_INIT_EDGE_LEN;
	n->out.arr = mem_get(g->mm, sizeof(struct gr_edge*) * 
			     CAT_GRAPH_INIT_EDGE_LEN);
	if ( !n->out.arr ) {
		mem_free(g->mm, n);
		return NULL;
	}

	if ( !g->isbi ) {
		n->in.arr = mem_get(g->mm, sizeof(struct gr_edge*) * 
				    CAT_GRAPH_INIT_EDGE_LEN);
		if ( !n->in.arr ) {
			mem_free(g->mm, n->out.arr);
			mem_free(g->mm, n);
			return NULL;
		}

	} else {
		n->in.arr = n->out.arr;
	}
	l_ins(&g->nodes, &n->entry);
	return n;
}


static int add_edge_help(struct gr_edge *edge, struct gr_edge_arr *ea,
		         struct gr_edge_arr *ea2)
{
	uint i;
	struct graph *g;

	g = edge->n1->graph;

	for ( i = 0 ; i < ea->fill ; ++i ) {
		if ( !ea->arr[i] ) {
			ea->arr[i] = edge;
			return 0;
		}
	}
	
	if ( ea->fill == ea->len ) {
		struct gr_edge **newarr;
		size_t newlen;

		/* need to grow the array */
		abort_unless(ea->len <= ((size_t)~0) >> 1);
		newlen = ea->len << 1;
		abort_unless(newlen <= ((size_t)~0) / sizeof(struct gr_edge*));

		newarr = mem_resize(g->mm, ea->arr, 
				    newlen * sizeof(struct gr_edge*));
		if ( !newarr )
			return -1;
		ea->arr = newarr;
		ea->len = newlen;
	}

	ea->arr[ea->fill++] = edge;
	if ( ea2 )
		*ea2 = *ea;
	return 0;
}


struct gr_edge * gr_add_edge(struct gr_node *from, struct gr_node *to)
{
	struct graph *g;
	struct gr_edge *edge;

	abort_unless(from && to);
	g = from->graph;
	abort_unless(g && g == to->graph && g->mm);

	edge = mem_get(g->mm, g->esize);
	if ( !edge )
		return NULL;

	l_ins(&g->edges, &edge->entry);
	edge->n1 = from;
	edge->n2 = to;

	/* insert into arrays:  grow if necessary */
	if ( add_edge_help(edge, &from->out, g->isbi ? &from->in : NULL) < 0 ) {
		mem_free(g->mm, edge);
		return NULL;
	}
	if ( from != to || !g->isbi ) {
		if ( add_edge_help(edge, &to->in, 
				   g->isbi ? &to->out : NULL ) < 0 ) {
			/* XXX double check that this is ok */
			del_edge_help(edge, edge->n1, 1);
			mem_free(g->mm, edge);
			return NULL;
		}
	}

	return edge;
}


struct gr_edge * gr_find_edge(struct gr_node *from, struct gr_node *to)
{
	struct graph *g;
	uint i, isbi;
	struct gr_edge **epp;
	
	abort_unless(from);
	abort_unless(to);
	g = from->graph;
	abort_unless(g);
	abort_unless(g == to->graph);
	isbi = g->isbi;

	for ( i = 0, epp = from->out.arr ; i < from->out.fill ; ++i, ++epp )
		if ( *epp && 
		     (((*epp)->n2 == to) || (isbi && (*epp)->n1 == to)) )
			return *epp;

	return NULL;
}


void gr_del_node(struct gr_node *node)
{
	struct gr_edge **edge;
	struct graph *g;
	uint i;

	abort_unless(node);
	g = node->graph;
	abort_unless(g && g->mm);

	for ( i = 0, edge = node->out.arr ; i < node->out.fill ; ++i, ++edge )
		if ( *edge )
			gr_del_edge(*edge);
	mem_free(g->mm, node->out.arr);

	if ( !g->isbi ) {
		for ( i = 0, edge = node->in.arr ; i < node->in.fill ; 
	              ++i, ++edge )
			if ( *edge )
				gr_del_edge(*edge);
		mem_free(g->mm, node->in.arr);
	}

	l_rem(&node->entry);
	node->graph = NULL;
	mem_free(g->mm, node);
}


static void del_edge_help(struct gr_edge *edge, struct gr_node *node,
		          int fromout)
{
	struct gr_edge **epp;
	uint i, fill;
	struct graph *g;

	g = node->graph;
	abort_unless(g && g->mm);

	if ( fromout ) {
		epp = node->out.arr;
		fill = node->out.fill;
	} else {
		epp = node->in.arr;
		fill = node->in.fill;
	}

	for ( i = 0 ; i < fill ; ++i, ++epp ) {
		if ( *epp == edge ) {
			*epp = NULL;
			break;
		}
	}
	abort_unless(i < fill);

	if ( i == fill - 1 ) {
		struct gr_edge_arr *ea = fromout ? &node->out : &node->in;

		while ( ea->fill > 0 && !ea->arr[ea->fill - 1] )
			--ea->fill;
		if ( node->graph->isbi ) {
			if ( fromout )
				node->in.fill = node->out.fill;
			else
				node->out.fill = node->in.fill;
		}
	}

	l_rem(&edge->entry);
}


void gr_del_edge(struct gr_edge *edge)
{
	struct graph *g;

	abort_unless(edge && edge->n1 && edge->n2);
	g = edge->n1->graph;
	abort_unless(g && g->mm);

	del_edge_help(edge, edge->n1, 1);
	if ( (edge->n1 != edge->n2) || !g->isbi )
		del_edge_help(edge, edge->n2, 0);
	mem_free(g->mm, edge);
}


void gr_free(struct graph *g)
{
	struct gr_node *node;

	abort_unless(g && g->mm);

	while ( !l_isempty(&g->nodes) ) {
		node = container(l_head(&g->nodes), struct gr_node, entry);
		gr_del_node(node);
	}
	mem_free(g->mm, g);
}


struct gr_node * gr_edge_dst(const struct gr_node *node, 
			     const struct gr_edge *edge)
{
	abort_unless(node && node->graph);
	abort_unless(edge);
	if ( node->graph->isbi && edge->n2 == node )
		return edge->n1;
	else
		return edge->n2;
}

