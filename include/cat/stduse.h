/*
 * stduse.h -- application level nicities built on the rest of the catlib
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007-2015 -- See accompanying license
 *
 */

#ifndef __cat_stduse_h
#define __cat_stduse_h

#include <cat/cat.h>
#include <cat/emalloc.h>

/* Memory manager extras */
#include <cat/mem.h>

extern struct memmgr estdmm;

struct raw *erawdup(struct raw const * const r);


/* Application level list data structure */
#include <cat/list.h>

struct clist_node {
	struct list		entry;
	struct clist *		list;
	void *			data;
};


struct clist_attr {
	struct clist_node *	(*node_alloc)(struct clist *list);
	void 			(*node_free)(struct clist *list,
					     struct clist_node *node);
	attrib_t		ctx;
};


struct clist {
	struct clist_node	base;
	size_t 			fill;
	struct clist_node * 	(*node_alloc)(struct clist *list);
	void 			(*node_free)(struct clist *list,
					     struct clist_node *node);
	attrib_t		ctx;
};

#define l_to_cln(ln) 	container(ln, struct clist_node, entry)
#define cl_next(clnp)	l_to_cln((clnp)->entry.next)
#define cl_prev(clnp)	l_to_cln((clnp)->entry.prev)
#define cl_head(clp)	(&(clp)->base)
#define cl_end(clp)	(&(clp)->base)
#define cl_first(clp)	l_to_cln((clp)->base.entry.next)
#define cl_last(clp)	l_to_cln((clp)->base.entry.prev)
#define cl_for_each(node, list)		\
	for ( (node) = cl_head(list) ;	\
	      (node) != cl_end(list) ;	\
	      (node) = cl_next(node) )

struct clist *cl_new(const struct clist_attr *attr);
void   cl_free(struct clist *list);
int    cl_isempty(struct clist *list);
size_t cl_fill(struct clist *list);
int    cl_ins(struct clist *list, struct clist_node *prev, void *val);
void * cl_del(struct clist *list, struct clist_node *node);
int    cl_enq(struct clist *list, void *val);
void * cl_deq(struct clist *list);
int    cl_push(struct clist *list, void *val);
void * cl_pop(struct clist *list);
void * cl_top(struct clist *list);
void   cl_apply(struct clist *list, apply_f f, void *arg);


#include <cat/str.h>

char * str_copy_a(const char *src);
char * str_cat_a(const char *first, const char *second);
char * str_fmt_a(const char *fmt, ...);
char * str_vfmt_a(const char *fmt, va_list ap);
char * str_tok_a(char **start, const char *wschars);


#include <cat/dlist.h>

struct cdlist { 
	struct dlist		entry;
	void *			data;
};

void *    	cdl_data(struct dlist *node);
void      	cdl_set(struct dlist *node, void *data);
struct dlist * 	cdl_new(cat_time_t t, void *data);
void *	       	cdl_free(struct dlist *node);


/* Application layer hash table functions */
#include <cat/hash.h>

struct chnode {
	struct hnode	node;
	void *		data;
};

#define cht_data(_n) (container((_n), struct chnode, node)->data)

struct chtab;

struct chtab_attr {
	cmp_f		kcmp;
	hash_f		hash;
	size_t		hctx_size;
	struct chnode *	(*node_alloc)(struct chtab *, void *k);
	void		(*node_free)(struct chtab *, struct chnode *);
	void *		ctx;
};

struct chtab {
	struct htab	table;
	struct chnode *	(*node_alloc)(struct chtab *t, void *k);
	void		(*node_free)(struct chtab *t, struct chnode *n);
	void *		ctx;
};

extern struct chtab_attr cht_std_attr_skey;	/* string key table */
extern struct chtab_attr cht_std_attr_rkey;	/* raw key table */
extern struct chtab_attr cht_std_attr_ikey;	/* int key table */

struct chtab *	cht_new(size_t nbkts, struct chtab_attr *attr, void *hctx);
void		cht_free(struct chtab *t);
void *		cht_get(struct chtab *t, void *key);
int		cht_put(struct chtab *t, void *key, void *data);
void *		cht_del(struct chtab *t, void *key);
void		cht_apply(struct chtab *t, apply_f f, void *ctx);


#include <cat/avl.h>

struct canode {
	struct anode	node;
	void *		data;
};

#define cavl_data(_n) (container((_n), struct canode, node)->data)

struct cavltree;

struct cavltree_attr {
	cmp_f		kcmp;
	struct canode *	(*node_alloc)(struct cavltree *t, void *k);
	void		(*node_free)(struct cavltree *t, struct canode *n);
	void *		ctx;
};

struct cavltree {
	struct avltree	tree;
	struct canode *	(*node_alloc)(struct cavltree *t, void *k);
	void		(*node_free)(struct cavltree *t, struct canode *n);
	void *		ctx;
};

extern struct cavltree_attr cavl_std_attr_skey;	/* string key table */
extern struct cavltree_attr cavl_std_attr_rkey;	/* raw key table */
extern struct cavltree_attr cavl_std_attr_ikey;	/* int key table */


struct cavltree * cavl_new(struct cavltree_attr *attr);
void		cavl_free(struct cavltree *t);
void *		cavl_get(struct cavltree *t, void *key);
int		cavl_put(struct cavltree *t, void *key, void *data);
void *		cavl_del(struct cavltree *t, void *key);
void		cavl_apply(struct cavltree *t, apply_f f, void *ctx);


#include <cat/rbtree.h>

struct crbnode {
	struct rbnode	node;
	void *		data;
};

#define crb_data(_n) (container((_n), struct crbnode, node)->data)

struct crbtree;

struct crbtree_attr {
	cmp_f		kcmp;
	struct crbnode *(*node_alloc)(struct crbtree *t, void *k);
	void		(*node_free)(struct crbtree *t, struct crbnode *n);
	void *		ctx;
};

struct crbtree {
	struct rbtree	tree;
	struct crbnode *(*node_alloc)(struct crbtree *t, void *k);
	void		(*node_free)(struct crbtree *t, struct crbnode *n);
	void *		ctx;
};

extern struct crbtree_attr crb_std_attr_skey;	/* string key table */
extern struct crbtree_attr crb_std_attr_rkey;	/* raw key table */
extern struct crbtree_attr crb_std_attr_ikey;	/* int key table */


struct crbtree *crb_new(struct crbtree_attr *attr);
void		crb_free(struct crbtree *t);
void *		crb_get(struct crbtree *t, void *key);
int		crb_put(struct crbtree *t, void *key, void *data);
void *		crb_del(struct crbtree *t, void *key);
void		crb_apply(struct crbtree *t, apply_f f, void *ctx);


#include <cat/splay.h>

struct cstnode {
	struct stnode	node;
	void *		data;
};

#define cst_data(_n) (container((_n), struct cstnode, node)->data)

struct cstree;

struct cstree_attr {
	cmp_f		kcmp;
	struct cstnode *(*node_alloc)(struct cstree *t, void *k);
	void		(*node_free)(struct cstree *t, struct cstnode *n);
	void *		ctx;
};

struct cstree {
	struct sptree	tree;
	struct cstnode *(*node_alloc)(struct cstree *t, void *k);
	void		(*node_free)(struct cstree *t, struct cstnode *n);
	void *		ctx;
};

extern struct cstree_attr cst_std_attr_skey;	/* string key table */
extern struct cstree_attr cst_std_attr_rkey;	/* raw key table */
extern struct cstree_attr cst_std_attr_ikey;	/* int key table */


struct cstree * cst_new(struct cstree_attr *attr);
void		cst_free(struct cstree *t);
void *		cst_get(struct cstree *t, void *key);
int		cst_put(struct cstree *t, void *key, void *data);
void *		cst_del(struct cstree *t, void *key);
void		cst_apply(struct cstree *t, apply_f f, void *ctx);


#include <cat/heap.h>

struct heap * hp_new(size_t size, cmp_f cmp);
void	      hp_free(struct heap *hp);


#include <cat/ring.h>

char * ring_alloc(struct ring *r, size_t len);
int    ring_fmt(struct ring *r, const char *fmt, ...);


#include <cat/match.h>

struct std_kmppat {
	struct kmppat	pattern;
	struct memmgr *	mm;
};

struct kmppat *	 kmp_pnew(struct memmgr *mm, struct raw *pat);
void 		 kmp_free(struct kmppat *kmp);

struct std_bmpat {
	struct bmpat	pattern;
	struct memmgr * mm;
};

struct bmpat *	 bm_pnew(struct memmgr *mm, struct raw *pat);
void		 bm_free(struct bmpat *bmp);

struct sfxtree * sfx_new(struct memmgr *mm, struct raw *pat);
void		 sfx_free(struct sfxtree *sfx);


#include <cat/bitset.h>

struct safebitset {
	uint		nbits;
	uint		len;
	bitset_t *	set;
	struct memmgr *	mm;
};

struct safebitset *sbs_new(struct memmgr *mm, uint nbits);
void sbs_free(struct safebitset *set);
void sbs_zero(struct safebitset *set);
void sbs_fill(struct safebitset *set);
uint sbs_copy(struct safebitset *dst, struct safebitset *src);
int  sbs_test(struct safebitset *set, uint index);
void sbs_set(struct safebitset *set, uint index);
void sbs_clr(struct safebitset *set, uint index);
void sbs_flip(struct safebitset *set, uint index);
void sbs_set_to(struct safebitset *set, uint index, int val);


#endif /* __cat_stduse_h */
