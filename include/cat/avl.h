/*
 * cat/avl.h -- AVL tree implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004, 2005 See accompanying license
 *
 */

#ifndef __cat_avl_h
#define __cat_avl_h

#include <cat/cat.h>
#include <cat/aux.h>

struct anode {
	struct anode *	p[3];
#define CA_L 0	/* left node */
#define CA_R 2	/* right node */
#define CA_P 1  /* parent node */
#define CA_N 3  /* Used to denote an exact node when given a (p,dir) pair */

	signed char	b;	/* - == left and + == right */
	uchar	        pdir;	/* on the parent's left or right ? */
	struct avl *	tree;
	void *		key;
	void *		data;
} ;


struct avl {
	cmp_f		cmp;
	struct anode	root;
} ; 


#define avl_left	p[CA_L]
#define avl_right	p[CA_R]
#define avl_parent	p[CA_P]
#define avl_root	root.p[CA_P]


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define PTRDECL static
#define CAT_AVL_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#define PTRDECL
#endif /* CAT_USE_INLINE */


/* main functions */
DECL void avl_init(struct avl *a, cmp_f cmp);
DECL void avl_ninit(struct anode *a, void *k, void *d);

DECL struct anode * avl_lkup(struct avl *t, void *key, int *dir);
DECL struct anode * avl_ins(struct avl *t, struct anode *n, struct anode *loc, 
		            int dir);
DECL void           avl_rem(struct anode *node);

DECL void avl_apply(struct avl *a, apply_f func, void * ctx);


/* Auxiliary (helper) functions (don't use) */
DECL void avl_fix(struct anode *p, struct anode *c, int dir);
DECL void avl_findloc(struct avl *t, void *key, struct anode **p, int *d);
DECL void avl_ins_at(struct avl *t, struct anode *node, struct anode *par, 
		     int dir);
DECL void avl_rleft(struct anode *n1, struct anode *n2, int ins);
DECL void avl_rright(struct anode *n1, struct anode *n2, int ins);
DECL void avl_zleft(struct anode *n1, struct anode *n2, struct anode *n3);
DECL void avl_zright(struct anode *n1, struct anode *n2, struct anode *n3);
DECL struct anode *avl_findrep(struct anode *node);


/* actual implementation */
#if defined(CAT_AVL_DO_DECL) && CAT_AVL_DO_DECL

DECL void avl_init(struct avl *a, cmp_f cmp)
{
	abort_unless(a);
	abort_unless(cmp);
	a->cmp = cmp;
	avl_ninit(&a->root, NULL, NULL);
	a->root.pdir = CA_P;
}


DECL void avl_ninit(struct anode *a, void *k, void *d)
{
	abort_unless(a);
	a->p[0] = a->p[1] = a->p[2] = NULL;
	a->pdir = CA_P;
	a->b = 0;
	a->key = k;
	a->data = d;
}


DECL struct anode *avl_lkup(struct avl *t, void *key, int *rdir)
{
	struct anode *p;
	int dir;

	abort_unless(t);
	avl_findloc(t, key, &p, &dir);
	if ( rdir ) {
		*rdir = dir;
		return p;
	} else {
		if ( dir == CA_N )
			return p;
		else
			return NULL;
	}
}


DECL struct anode *avl_ins(struct avl *t, struct anode *node, struct anode *loc,
		           int atdir)
{
	struct anode *p;
	int dir;
	void *key;

	abort_unless(t);
	if ( !node )
		return NULL;

	key = node->key;

	if ( loc ) {
		p = loc;
		dir = atdir;
		abort_unless((dir == CA_L) || (dir == CA_R) || 
			     ((dir == CA_P) && (p == &t->root)));
	} else {
		avl_findloc(t, key, &p, &dir);
	}

	if ( dir == CA_N ) {
		avl_fix(node, p->p[CA_L], CA_L);
		avl_fix(node, p->p[CA_R], CA_R);
		avl_fix(p->p[CA_P], node, p->pdir);
		node->tree = t;
		p->tree = NULL;
		avl_ninit(p, p->key, p->data);
		return p;
	} else {
		avl_ins_at(t, node, p, dir);
		node->tree = t;
		return NULL;
	}
}


/* currently an inorder traversal:  we could add an arg to change this */
DECL void avl_apply(struct avl *a, apply_f func, void * ctx)
{
	struct anode *trav;
	int dir = CA_P;

	abort_unless(a);
	abort_unless(func);
	trav = a->avl_root;
	if ( ! trav )
		return;
	do {
		switch(dir) {
		case CA_P:
			if ( trav->p[CA_L] )
				trav = trav->p[CA_L];	/* dir stays the same */
			else if ( trav->p[CA_R] )
				trav = trav->p[CA_R];	/* dir stays the same */
			else {
				func(trav->data, ctx);
				dir  = trav->pdir;
				trav = trav->p[CA_P];
			}
			break;

		case CA_L:
			if ( trav->p[CA_R] ) {
				dir  = CA_P;
				trav = trav->p[CA_R];	/* dir stays the same */
			} else {
				func(trav->data, ctx);
				dir  = trav->pdir;
				trav = trav->p[CA_P];
			}
			break;

		case CA_R:
			func(trav->data, ctx);
			dir  = trav->pdir;
			trav = trav->p[CA_P];
			break;
		}
	} while ( trav != &a->root );
}


DECL void avl_findloc(struct avl *t, void *key, struct anode **pn, int *pd)
{
	struct anode *tmp, *par;
	int dir = CA_P;

	abort_unless(t);
	abort_unless(pn);
	abort_unless(pd); 
	par = &t->root;

	while ( (tmp = par->p[dir]) ) {
		par = tmp;
		dir = (*t->cmp)(key, tmp->key);
		if ( dir < 0 )
			dir = CA_L;
		else if ( dir > 0 )
			dir = CA_R;
		else {
			*pn = par;
			*pd = CA_N;
			return;
		}
	}

	*pn = par;
	*pd = dir;
}


DECL void avl_ins_at(struct avl *t, struct anode *node, struct anode *par, 
		     int dir)
{
	struct anode *tmp;

	abort_unless(t);
	abort_unless(node);
	abort_unless(par);
	abort_unless(dir >= CA_L && dir <= CA_R);
	avl_fix(par, node, dir);

	while ( (dir = node->pdir) != CA_P ) {
		node = node->p[CA_P];
		/* dir - 1 == -1 if dir == CA_L and 1 if dir == CA_R */
		if ( node->b += (dir - 1) ) {
			if ( node->b < -1 ) {
				if ( (tmp = node->p[CA_L])->b < 0 )
					avl_rright(node, tmp, 1);
				else
					avl_zright(node, tmp, tmp->p[CA_R]);
				return;
			} else if ( node->b > 1 ) {
				if ( (tmp = node->p[CA_R])->b > 0 )
					avl_rleft(node, tmp, 1);
				else
					avl_zleft(node, tmp, tmp->p[CA_L]);
				return;
			}
		} else /* balance is now 0 */
			break;
	}
}


DECL struct anode *avl_findrep(struct anode *node)
{
	struct anode *tmp;

	abort_unless(node);
	tmp = node->p[CA_L];
	if ( ! tmp )
		return node->p[CA_R];
	else {
		while ( tmp->p[CA_R] ) 
			tmp = tmp->p[CA_R];
	}

	return tmp;
}


DECL void avl_rem(struct anode *node)
{
	struct anode *rep, *par, *trav, *tmp, *tmp2;
	int dir;

	abort_unless(node);
	if ( ! node || ! node->tree )
		return;

	par = node->p[CA_P];
	rep = avl_findrep(node);

	/* if replacement is not a direct child and is not NULL */
	if ( rep && (node->p[CA_L] != rep) && (node->p[CA_R] != rep) ) {
		trav = rep->p[CA_P];
		dir  = rep->pdir;
		avl_fix(rep->p[CA_P], rep->p[CA_L], rep->pdir);
		avl_fix(rep, node->p[CA_L], CA_L);
		avl_fix(rep, node->p[CA_R], CA_R);
		avl_fix(node->p[CA_P], rep, node->pdir);
		rep->b = node->b;
	} else {
		trav = par;
		dir = node->pdir;
		avl_fix(par, rep, dir);

		/* due to findrep alg, either node->p[CA_L] must be NULL or */
		/* rep->p[CA_R] must be NULL */
		/* rep is the right node iff the left is NULL */
		if ( rep && (node->p[CA_L] == rep) ) {
			trav = rep;
			dir = CA_L;
			rep->b = node->b;
			avl_fix(rep, node->p[CA_R], CA_R);
		}
		}

	node->p[0] = node->p[1] = node->p[2] = NULL;
	node->tree = NULL;
	node->pdir = CA_P;
	node->b = 0;

	/* now traverse up the tree */
	while ( dir != CA_P ) {
		if ( trav->b -= (dir - 1) ) {
			if ( trav->b < -1 ) {
				if ( (tmp = trav->p[CA_L])->b <= 0 ) {
					avl_rright(trav, tmp, 0);
					if ( tmp->b )
						break;
					trav = tmp;
				} else {
					tmp2 = tmp->p[CA_R];
					avl_zright(trav, tmp, tmp2);
					trav = tmp2;
				}
			} else if ( trav->b > 1 ) {
				if ( (tmp = trav->p[CA_R])->b >= 0 ) {
					avl_rleft(trav, tmp, 0);
					if ( tmp->b )
						break;
					trav = tmp;
				} else {
					tmp2 = tmp->p[CA_L];
					avl_zleft(trav, tmp, tmp2);
					trav = tmp2;
				}

			} else /* unbalanced but no height change */
				break;
		}

		dir  = trav->pdir;
		trav = trav->p[CA_P];
	}
}


DECL void avl_fix(struct anode *p, struct anode *c, int dir)
{
	abort_unless(p);
	abort_unless(dir >= CA_L && dir <= CA_R);
	p->p[dir] = c;
	if ( c ) {
		c->pdir = dir;
		c->p[CA_P] = p;
	}
}


DECL void avl_rleft(struct anode *n1, struct anode *n2, int ins)
{
	abort_unless(n1);
	abort_unless(n2);
	avl_fix(n1, n2->p[CA_L], CA_R);
	avl_fix(n1->p[CA_P], n2, n1->pdir);
	avl_fix(n2, n1, CA_L);

	if ( ins || (n2->b > 0) ) {
		n1->b = 0;
		n2->b = 0;
	} else {
		n1->b = 1;
		n2->b = -1;
	}
}


DECL void avl_rright(struct anode *n1, struct anode *n2, int ins)
{
	abort_unless(n1);
	abort_unless(n2);
	avl_fix(n1, n2->p[CA_R], CA_L);
	avl_fix(n1->p[CA_P], n2, n1->pdir);
	avl_fix(n2, n1, CA_R);

	if ( ins || (n2->b < 0) ) {
		n1->b = 0;
		n2->b = 0;
	} else {
		n1->b = -1;
		n2->b = 1;
	}
}


DECL void avl_zleft(struct anode *n1, struct anode *n2, struct anode *n3)
{
	abort_unless(n1);
	abort_unless(n2);
	abort_unless(n3);
	avl_fix(n2, n3->p[CA_R], CA_L);
	avl_fix(n1, n3->p[CA_L], CA_R);
	avl_fix(n1->p[CA_P], n3, n1->pdir);
	avl_fix(n3, n1, CA_L);
	avl_fix(n3, n2, CA_R);

	switch ( n3->b ) {
	case -1:
		n1->b = 0;
		n2->b = 1;
		break;

	case 0:
		n1->b = 0;
		n2->b = 0;
		break;

	case 1:
		n1->b = -1;
		n2->b = 0;
		break;
	}

	n3->b = 0;
}


DECL void avl_zright(struct anode *n1, struct anode *n2, struct anode *n3)
{
	abort_unless(n1);
	abort_unless(n2);
	abort_unless(n3);
	avl_fix(n2, n3->p[CA_L], CA_R);
	avl_fix(n1, n3->p[CA_R], CA_L);
	avl_fix(n1->p[CA_P], n3, n1->pdir);
	avl_fix(n3, n1, CA_R);
	avl_fix(n3, n2, CA_L);

	switch ( n3->b ) {
	case -1:
		n1->b = 1;
		n2->b = 0;
		break;

	case 0:
		n1->b = 0;
		n2->b = 0;
		break;

	case 1:
		n1->b = 0;
		n2->b = -1;
		break;
	}
	n3->b = 0;
}

#endif /* CAT_AVL_DO_DECL */


#undef PTRDECL
#undef DECL
#undef CAT_AVL_DO_DECL

#endif /* __cat_avl_h */
