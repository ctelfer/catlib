/*
 * cat/splay.h -- Splay tree implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2006 See accompanying license
 *
 */

#ifndef __cat_splay_h
#define __cat_splay_h

#include <cat/cat.h>
#include <cat/aux.h>

struct stnode {
	struct stnode *	p[3];
	char		pdir;
	struct splay *	tree;
	void *		key;
	void *		data;
} ;

#define CST_L 0	 /* left node */
#define CST_R 2	 /* right node */
#define CST_P 1  /* parent node */
#define CST_N 3  /* Used to denote an exact node when given a (p,dir) pair */

struct splay {
	cmp_f		cmp;
	struct stnode	root;
} ; 


#define st_left		p[CST_L]
#define st_right	p[CST_R]
#define st_par		p[CST_P]
#define st_root		root.p[CST_P]


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define PTRDECL static
#define CAT_SP_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#define PTRDECL
#endif /* CAT_USE_INLINE */

/* main functions */
DECL void            st_init(struct splay *t, cmp_f cmp);
DECL void            st_ninit(struct stnode *n, void *k, void *d);
DECL struct stnode * st_lkup(struct splay *t, void *key);
DECL struct stnode * st_ins(struct splay *t, struct stnode *node);
DECL void            st_rem(struct stnode *node);
DECL void            st_apply(struct splay *t, apply_f func, void * ctx);


/* Auxiliary (helper) functions (don't use) outside the module */
DECL void st_findloc(struct splay *t, void *key, struct stnode **pn, int *pd);
DECL void st_fix(struct stnode *par, struct stnode *cld, int dir);
DECL void st_rl(struct stnode *n);
DECL void st_rr(struct stnode *n);
DECL void st_zll(struct stnode *n);
DECL void st_zlr(struct stnode *n);
DECL void st_zrl(struct stnode *n);
DECL void st_zrr(struct stnode *n);
DECL void st_splay(struct stnode *n);


#if defined(CAT_SP_DO_DECL) && CAT_SP_DO_DECL

DECL void st_init(struct splay *t, cmp_f cmp)
{
	abort_unless(t);
	abort_unless(cmp);
	t->cmp = cmp;
	st_ninit(&t->root, NULL, NULL);
	t->root.pdir = CST_P;
}


DECL void st_ninit(struct stnode *n, void *k, void *d)
{
	abort_unless(n);
	n->p[CST_L] = n->p[CST_R] = n->p[CST_P] = NULL;
	n->pdir = CST_N;
	n->key = k;
	n->data = d;
}


DECL struct stnode * st_lkup(struct splay *t, void *key)
{
	struct stnode *n;
	int dir;

	abort_unless(t);
	st_findloc(t, key, &n, &dir);
	if ( n == &t->root )
		return NULL;
	st_splay(n);
	if ( dir == CST_N )
		return n;
	else
		return NULL;
}


DECL struct stnode * st_ins(struct splay *t, struct stnode *node)
{
	struct stnode *n;
	int dir;

	abort_unless(t);
	node->tree = t;
	st_findloc(t, node->key, &n, &dir);
	if ( n == &t->root ) {
		st_fix(&t->root, node, CST_P);
		n = NULL;
	} else if ( dir == CST_N ) {
		/* approach: splay the original to the top of the tree and */
		/* then replace it with the new one */
		st_splay(n);
		st_fix(node, n->st_left, CST_L);
		st_fix(node, n->st_right, CST_R);
		st_fix(n->st_par, node, n->pdir);
		n->st_left = n->st_right = n->st_par = NULL;
		n->pdir = CST_N;
	} else {
		st_fix(n, node, dir);
		st_splay(node);
		n = NULL;
	}

	return n;
}


DECL void st_rem(struct stnode *node)
{
	struct stnode *root, *rep;
	struct splay *t;

	abort_unless(node);
	if ( ! node || ! node->tree )
		return;
	t = node->tree;
	root = &t->root;
	st_splay(node);

	if ( ! node->st_left && ! node->st_right ) {
		st_fix(root, NULL, CST_P);
	} else if ( ! node->st_right ) {
		st_fix(root, node->st_left, CST_P);
	} else if ( ! node->st_left ) {
		st_fix(root, node->st_right, CST_P);
	} else {
		rep = node->st_left;
		while ( rep->st_right )
			rep = rep->st_right;
		st_fix(root, node->st_left, CST_P);
		st_splay(rep);
		abort_unless(rep->st_right == NULL);
		st_fix(rep, node->st_right, CST_R);
	}
	node->st_left = node->st_right = node->st_par = NULL;
	node->pdir = CST_N;
}


DECL void st_apply(struct splay *t, apply_f func, void * ctx)
{
	struct stnode *trav;
	int dir = CST_P;

	abort_unless(t);
	abort_unless(func);
	trav = t->root.p[CST_P];
	if ( ! trav )
		return;

	do { 
		switch (dir) {
		case CST_P:
			if ( trav->st_left )
				trav = trav->st_left;
			else if ( trav->st_right )
				trav = trav->st_right;
			else {
				(*func)(trav->data, ctx);
				dir = trav->pdir;
				trav = trav->st_par;
			}
			break;
		case CST_L:
			if ( trav->st_right ) {
				dir = CST_P;
				trav = trav->st_right;
			} else {
				(*func)(trav->data, ctx);
				dir = trav->pdir;
				trav = trav->st_par;
			}
			break;
		case CST_R:
			(*func)(trav->data, ctx);
			dir = trav->pdir;
			trav = trav->st_par;
			break;
		}
	} while ( trav != &t->root );
}


DECL void st_findloc(struct splay *t, void *key, struct stnode **pn, int *pd)
{
	struct stnode *tmp, *par;
	int dir = CST_P;

	abort_unless(t);
	abort_unless(pn);
	abort_unless(pd);
	par = &t->root;

	while ( (tmp = par->p[dir]) ) {
		par = tmp;
		dir = (*t->cmp)(key, tmp->key);
		if ( dir < 0 )
			dir = CST_L;
		else if ( dir > 0 )
			dir = CST_R;
		else {
			*pn = tmp;
			*pd = CST_N;
			return;
		}
	}

	*pn = par;
	*pd = dir;
}


DECL void st_fix(struct stnode *par, struct stnode *cld, int dir)
{
	abort_unless(par);
	abort_unless(dir >= CST_L && dir <= CST_R);
	par->p[dir] = cld;
	if ( cld ) {
		cld->pdir = dir;
		cld->p[CST_P] = par;
	}
}

DECL void st_rl(struct stnode *n)
{
	struct stnode *c;
	abort_unless(n);
	c = n->st_right;
	abort_unless(c);
	st_fix(n, c->st_left, CST_R);
	st_fix(n->st_par, c, n->pdir);
	st_fix(c, n, CST_L);
}

DECL void st_rr(struct stnode *n)
{
	struct stnode *c;
	abort_unless(n);
	c = n->st_left;
	abort_unless(c);
	st_fix(n, c->st_right, CST_L);
	st_fix(n->st_par, c, n->pdir);
	st_fix(c, n, CST_R);
}

DECL void st_zll(struct stnode *n)
{
	struct stnode *p, *gp;
	p = n->st_par;
	abort_unless(p);
	gp = p->st_par;
	abort_unless(gp);
	st_fix(gp, p->st_left, CST_R);
	st_fix(p, n->st_left, CST_R);
	st_fix(gp->st_par, n, gp->pdir);
	st_fix(n, p, CST_L);
	st_fix(p, gp, CST_L);
}

DECL void st_zlr(struct stnode *n)
{
	struct stnode *p, *gp;
	p = n->st_par;
	abort_unless(p);
	gp = p->st_par;
	abort_unless(gp);
	st_fix(gp, n->st_right, CST_L);
	st_fix(p, n->st_left, CST_R);
	st_fix(gp->st_par, n, gp->pdir);
	st_fix(n, p, CST_L);
	st_fix(n, gp, CST_R);
}

DECL void st_zrl(struct stnode *n)
{
	struct stnode *p, *gp;
	p = n->st_par;
	abort_unless(p);
	gp = p->st_par;
	abort_unless(gp);
	st_fix(gp, n->st_left, CST_R);
	st_fix(p, n->st_right, CST_L);
	st_fix(gp->st_par, n, gp->pdir);
	st_fix(n, p, CST_R);
	st_fix(n, gp, CST_L);
}

DECL void st_zrr(struct stnode *n)
{
	struct stnode *p, *gp;
	p = n->st_par;
	abort_unless(p);
	gp = p->st_par;
	abort_unless(gp);
	st_fix(gp, p->st_right, CST_L);
	st_fix(p, n->st_right, CST_L);
	st_fix(gp->st_par, n, gp->pdir);
	st_fix(n, p, CST_R);
	st_fix(p, gp, CST_R);
}


DECL void st_splay(struct stnode *n)
{
	struct stnode *p;
	abort_unless(n);
	while ( n->pdir != CST_P ) {
		p = n->st_par;
		abort_unless(p);
		if ( p->pdir == CST_P ) {
			if ( n->pdir == CST_L )
				st_rr(p);
			else
				st_rl(p);
		} else if ( p->pdir == CST_L ) {
			if ( n->pdir == CST_L )
				st_zrr(n);
			else
				st_zlr(n);
		} else {
			abort_unless(p->pdir == CST_R);
			if ( n->pdir == CST_L )
				st_zrl(n);
			else
				st_zll(n);
		}
	}
}

#endif /* defined(CAT_SP_DO_DECL) && CAT_SP_DO_DECL */


#undef PTRDECL
#undef DECL
#undef CAT_SP_DO_DECL

#endif /* __cat_splay_h */
