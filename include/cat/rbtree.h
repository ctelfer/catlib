/*
 * cat/rbtree.h -- Red-Black tree implementation
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2004, 2005 See accompanying license
 *
 */

#ifndef __cat_rbtree_h
#define __cat_rbtree_h

#include <cat/cat.h>
#include <cat/aux.h>

struct rbnode {
  struct rbnode *	p[3];
  char			pdir;	/* on the parent's left or right ? */
  char			col;
  struct rbtree *	tree;
  void *		key;
  void *		data;
} ;

#define CRB_L 0	/* left node */
#define CRB_R 2	/* right node */
#define CRB_P 1  /* parent node */
#define CRB_N 3  /* Used to denote an exact node when given a (p,dir) pair */
#define CRB_RED 0
#define CRB_BLACK 1

struct rbtree {
  cmp_f		cmp;
  struct rbnode	root;
} ; 


#define rb_left		p[CRB_L]
#define rb_right	p[CRB_R]
#define rb_par		p[CRB_P]
#define rb_root		root.p[CRB_P]


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define PTRDECL static
#define CAT_RB_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#define PTRDECL
#endif /* CAT_USE_INLINE */

/* main functions */
DECL void            rb_init(struct rbtree *t, cmp_f cmp);
DECL void            rb_ninit(struct rbnode *n, void *k, void *d);
DECL struct rbnode * rb_lkup(struct rbtree *t, void *key, int *dir);
DECL struct rbnode * rb_ins(struct rbtree *t, struct rbnode *node,
                            struct rbnode *loc, int dir);
DECL void            rb_rem(struct rbnode *node);
DECL void            rb_apply(struct rbtree *t, apply_f func, void * ctx);


/* Auxiliary (helper) functions (don't use) */
DECL void rb_findloc(struct rbtree *t, void *key, struct rbnode **p, int *dir);
DECL void rb_ins_at(struct rbtree *t, struct rbnode *node, struct rbnode *par, 
                    int dir);
DECL void rb_fix(struct rbnode *par, struct rbnode *cld, int dir);
DECL void rb_rleft(struct rbnode *n);
DECL void rb_rright(struct rbnode *n);


/* actual implementation */
#if defined(CAT_RB_DO_DECL) && CAT_RB_DO_DECL

DECL void rb_init(struct rbtree *t, cmp_f cmp)
{
  abort_unless(t);
  abort_unless(cmp);
  t->cmp = *cmp;
  rb_ninit(&t->root, NULL, NULL);
  t->root.pdir = CRB_P;
}


DECL void rb_ninit(struct rbnode *n, void *k, void *d)
{
  abort_unless(n);
  n->p[0] = n->p[1] = n->p[2] = NULL;
  n->pdir = 0;
  n->col  = CRB_RED;
  n->key  = k;
  n->data = d;
}


DECL struct rbnode *rb_lkup(struct rbtree *t, void *key, int *rdir)
{
  struct rbnode *p;
  int dir;

  abort_unless(t);

  rb_findloc(t, key, &p, &dir);
  if ( rdir ) {
    *rdir = dir;
    return p;
  }
  else {
    if ( dir == CRB_N )
      return p;
    else
      return NULL;
  }
}


DECL struct rbnode *rb_ins(struct rbtree *t, struct rbnode *node, 
                           struct rbnode *loc, int atdir)
{
  struct rbnode *p;
  int dir;
  void *key;

  abort_unless(t);
  if ( !node )
    return NULL;

  key = node->key;

  if ( loc ) {
    p = loc;
    dir = atdir;
    abort_unless((dir == CRB_L) || (dir == CRB_R) || 
           ((dir == CRB_P) && (p == &t->root)));
  }
  else
    rb_findloc(t, key, &p, &dir);

  if ( dir == CRB_N ) {
    rb_fix(node, p->p[CRB_L], CRB_L);
    rb_fix(node, p->p[CRB_R], CRB_R);
    rb_fix(p->p[CRB_P], node, p->pdir);
    node->col = p->col;
    node->tree = t;
    p->tree = NULL;
    p->col = CRB_RED;
    rb_ninit(p, p->key, p->data);
    return p;
  } 
  else {
    rb_ins_at(t, node, p, dir);
    node->tree = t;
    return NULL;
  }
}


/* currently an inorder traversal:  we could add an arg to change this */
DECL void rb_apply(struct rbtree *t, apply_f func, void * ctx)
{
  struct rbnode *trav;
  int dir = CRB_P;

  abort_unless(t);
  abort_unless(func);
  trav = t->rb_root;
  if ( ! trav )
    return;

  do {
    switch(dir) {
    case CRB_P:
      if ( trav->p[CRB_L] )
        trav = trav->p[CRB_L];	/* dir stays the same */
      else if ( trav->p[CRB_R] )
        trav = trav->p[CRB_R];	/* dir stays the same */
      else {
        func(trav->data, ctx);
        dir  = trav->pdir;
        trav = trav->p[CRB_P];
      }
      break;

    case CRB_L:
      if ( trav->p[CRB_R] ) {
        dir  = CRB_P;
        trav = trav->p[CRB_R];	/* dir stays the same */
      } else {
        func(trav->data, ctx);
        dir  = trav->pdir;
        trav = trav->p[CRB_P];
      }
      break;

    case CRB_R:
      func(trav->data, ctx);
      dir  = trav->pdir;
      trav = trav->p[CRB_P];
      break;
    }
  } while ( trav != &t->root );
}


DECL void rb_findloc(struct rbtree *t, void *key, struct rbnode **pn, int *pd)
{
  struct rbnode *tmp, *par;
  int dir = CRB_P;

  abort_unless(t);
  abort_unless(pn);
  abort_unless(pd);
  par = &t->root;

  while ( (tmp = par->p[dir]) ) {
    par = tmp;
    dir = (*t->cmp)(key, tmp->key);
    if ( dir < 0 )
      dir = CRB_L;
    else if ( dir > 0 )
      dir = CRB_R;
    else {
      *pn = par;
      *pd = CRB_N;
      return;
    }
  }

  *pn = par;
  *pd = dir;
}


#define COL(node)	((node) ? (node)->col : CRB_BLACK)
DECL void rb_ins_at(struct rbtree *t, struct rbnode *node, struct rbnode *par, 
                    int dir)
{
  struct rbnode *gp, *unc, *tmp;
  abort_unless(t);
  abort_unless(node);
  abort_unless(par);
  abort_unless(dir >= CRB_L && dir <= CRB_R);

  node->col = CRB_RED;
  rb_fix(par, node, dir);
  while ( node != t->rb_root && (par = node->p[CRB_P])->col == CRB_RED ) {

    if ( par->pdir == CRB_L ) { 
      gp = par->p[CRB_P];
      unc = gp->p[CRB_R];
      if ( COL(unc) == CRB_RED ) { 
        par->col = CRB_BLACK;
        unc->col = CRB_BLACK;
        gp->col  = CRB_RED;
        node = gp;
      } else {
        if ( node->pdir == CRB_R ) { 
          tmp = node;
          node = par;
          par = tmp;
          rb_rleft(node);
        }
        par->col = CRB_BLACK;
        gp->col = CRB_RED;
        rb_rright(gp);
      }
    }

    else {
      gp = par->p[CRB_P];
      unc = gp->p[CRB_L];
      if ( COL(unc) == CRB_RED ) { 
        par->col = CRB_BLACK;
        unc->col = CRB_BLACK;
        gp->col  = CRB_RED;
        node = gp;
      } else {
        if ( node->pdir == CRB_L ) { 
          tmp = node;
          node = par;
          par = tmp;
          rb_rright(node);
        }
        par->col = CRB_BLACK;
        gp->col = CRB_RED;
        rb_rleft(gp);
      }
    }
  }

  t->rb_root->col = CRB_BLACK;
}


DECL struct rbnode *rb_findrep(struct rbnode *node)
{
  struct rbnode *tmp;

  abort_unless(node);

  tmp = node->p[CRB_L];
  if ( ! tmp )
    return node->p[CRB_R];
  else {               
    while ( tmp->p[CRB_R] )
    tmp = tmp->p[CRB_R];
  }

  return tmp;
}


DECL void rb_rem(struct rbnode *node)
{
  struct rbnode *tmp, *par;
  int cdir, oldc;
  struct rbtree *t;

  abort_unless(node);
  t = node->tree;

  tmp = rb_findrep(node);
  if ( ! tmp ) {
    par = node->p[CRB_P];
    cdir = node->pdir;
    rb_fix(par, NULL, cdir);
    oldc = node->col;
  } else if ( tmp == node->p[CRB_L] || tmp == node->p[CRB_R] ) {
    par = tmp;
    if ( tmp == node->p[CRB_L] ) {
      rb_fix(tmp, node->p[CRB_R], CRB_R);
      cdir = CRB_L;
    } else
      cdir = CRB_R;
    rb_fix(node->p[CRB_P], tmp, node->pdir);
    oldc = tmp->col;
    tmp->col = node->col;
  } else {
    cdir = tmp->pdir;
    par = tmp->p[CRB_P];
    rb_fix(par, tmp->p[CRB_L], cdir);
    oldc = tmp->col;
    tmp->col = node->col;
    rb_fix(tmp, node->p[CRB_L], CRB_L);
    rb_fix(tmp, node->p[CRB_R], CRB_R);
    rb_fix(node->p[CRB_P], tmp, node->pdir);
  }
  rb_ninit(node, node->key, node->data);
  if ( oldc == CRB_RED )
    return;


  while ( cdir != CRB_P && COL(par->p[cdir]) == CRB_BLACK ) {
    if ( cdir == CRB_L ) {
      tmp = par->p[CRB_R];
      if ( tmp->col == CRB_RED ) {
        tmp->col = CRB_BLACK;
        par->col = CRB_RED;
        rb_rleft(par);
        tmp = par->p[CRB_R];
      }

      if ( COL(tmp->p[CRB_L]) == CRB_BLACK && 
           COL(tmp->p[CRB_R]) == CRB_BLACK  ) {
        tmp->col = CRB_RED;
        cdir = par->pdir;
        par = par->p[CRB_P];
      } else {
        if ( COL(tmp->p[CRB_R]) == CRB_BLACK ) {
          tmp->p[CRB_L]->col = CRB_BLACK;
          tmp->col = CRB_RED;
          rb_rright(tmp);
          tmp = par->p[CRB_R];
        }
        tmp->col = par->col;
        par->col = CRB_BLACK;
        tmp->p[CRB_R]->col = CRB_BLACK;
        rb_rleft(par);
        par = &t->root;
        cdir = CRB_P;
      }
    } else {
      tmp = par->p[CRB_L];
      if ( tmp->col == CRB_RED ) {
        tmp->col = CRB_BLACK;
        par->col = CRB_RED;
        rb_rright(par);
        tmp = par->p[CRB_L];
      }

      if ( COL(tmp->p[CRB_L]) == CRB_BLACK && 
           COL(tmp->p[CRB_R]) == CRB_BLACK  ) {
        tmp->col = CRB_RED;
        cdir = par->pdir;
        par = par->p[CRB_P];
      } else {
        if ( COL(tmp->p[CRB_L]) == CRB_BLACK ) {
          tmp->p[CRB_R]->col = CRB_BLACK;
          tmp->col = CRB_RED;
          rb_rleft(tmp);
          tmp = par->p[CRB_L];
        }
        tmp->col = par->col;
        par->col = CRB_BLACK;
        tmp->p[CRB_L]->col = CRB_BLACK;
        rb_rright(par);
        par = &t->root;
        cdir = CRB_P;
      }
    }
  }

  if ( par->p[cdir] )
    par->p[cdir]->col = CRB_BLACK;
}
#undef COL


DECL void rb_fix(struct rbnode *par, struct rbnode *cld, int dir)
{

  abort_unless(par);
  abort_unless(dir >= CRB_L && dir <= CRB_R);
  par->p[dir] = cld;
  if ( cld ) {
    cld->pdir = dir;
    cld->p[CRB_P] = par;
  }
}


DECL void rb_rleft(struct rbnode *n)
{
  struct rbnode *c;
  abort_unless(n);
  c = n->p[CRB_R];
  abort_unless(c);
  rb_fix(n, c->p[CRB_L], CRB_R);
  rb_fix(n->p[CRB_P], c, n->pdir);
  rb_fix(c, n, CRB_L);
}


DECL void rb_rright(struct rbnode *n)
{
  struct rbnode *c;
  abort_unless(n);
  c = n->p[CRB_L];
  abort_unless(c);
  rb_fix(n, c->p[CRB_R], CRB_L);
  rb_fix(n->p[CRB_P], c, n->pdir);
  rb_fix(c, n, CRB_R);
}

#endif /* CAT_RB_DO_DECL */


#undef PTRDECL
#undef DECL
#undef CAT_RB_DO_DECL

#endif /* __cat_rb_h */
