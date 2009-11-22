/*
 * stduse.h -- application level nicities built on the rest of the catlib
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007, 2008  See accompanying license
 *
 */

#ifndef __cat_stduse_h
#define __cat_stduse_h

#include <cat/cat.h>
#include <cat/emalloc.h>

/* Memory manager extras */
#include <cat/mem.h>

extern struct memmgr estdmm;

char *estrdup(const char *s);
struct raw *erawdup(struct raw const * const r);


/* Application level list data structure */
#include <cat/list.h>

struct clist_node { 
	struct list	cln_entry;
	struct memmgr * cln_mm;
	struct clist *	cln_list;
	union attrib_u	cln_data_u;
};

struct clist {
	struct clist_node	cl_base;
	struct memmgr *		cl_mm;
	size_t			cl_fill;
	size_t			cl_node_size;
	size_t			cl_data_size;
};

#define cln_int_val	cln_data_u.au_value
#define cln_ptr_val	cln_data_u.au_pointer
#define cln_data_ptr	cln_data_u.au_data
#define l_to_cln(ln) 	container(ln, struct clist_node, cln_entry)
#define cln_next(clnp)	l_to_cln((clnp)->cln_entry.next)
#define cln_prev(clnp)	l_to_cln((clnp)->cln_entry.prev)
#define cln_data(clnp, type)	(*((type *)&(clnp)->cln_data_u))
#define cl_head(clp)	(&(clp)->cl_base)
#define cl_end(clp)	(&(clp)->cl_base)
#define cl_first(clp)	l_to_cln((clp)->cl_base.cln_entry.next)
#define cl_last(clp)	l_to_cln((clp)->cl_base.cln_entry.prev)

struct clist *clist_new_list(struct memmgr *mm, size_t len);
void clist_free_list(struct clist *list);

void clist_init_list(struct clist *list, struct memmgr *mm, size_t dlen);
void clist_clear_list(struct clist *list);

int clist_isempty(struct clist *list);
size_t clist_fill(struct clist *list);

struct clist_node *clist_new_node(struct clist *list, void *val);
int clist_insert(struct clist *list, struct clist_node *prev, 
		 struct clist_node *node);
int clist_remove(struct clist_node *node);
void clist_delete(struct clist_node *node);

int clist_enqueue(struct clist *list, void *val);
int clist_dequeue(struct clist *list, void *val);
int clist_push(struct clist *list, void *val);
int clist_pop(struct clist *list, void *val);
int clist_top(struct clist *list, void *val);


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
struct dlist * 	cdl_new(long sec, long nsec, void *data);
void *	       	cdl_free(struct dlist *node);



/* Generic data types for container adaptors */
#define CAT_DT_STR	0
#define CAT_DT_RAW	1
#define CAT_DT_PTR	2
#define CAT_DT_NUM	3
#define CAT_DT_RAWCPY	4


#include <cat/hash.h>

/* Default hash table implementation functions */
/*  String comparison string hashing, key duplication, malloc: die on fail */
/*  Plus, dynamically allocated and freed table */
struct htab *	ht_new(size_t size, int type);
void		ht_free(struct htab *t);
struct hnode *	ht_snalloc(void *k, void *d, uint h, void *c);
void		ht_snfree(struct hnode *n, void *ctx);
void *		ht_get(struct htab *t, void *key);
void		ht_put(struct htab *t, void *key, void *data);
void		ht_clr(struct htab *t, void *key);
struct hnode *	ht_nnew(struct hashsys *sys, void *key, void *data, 
		        uint hash);
void		ht_nfree(struct hashsys *sys, struct hnode *node);


#include <cat/avl.h>

struct avl *	avl_new(int type);
void		avl_free(struct avl *t);
struct anode *	avl_snalloc(void *key, void *data, void *ctx);
void		avl_snfree(struct anode *n, void *ctx);
void *		avl_get(struct avl *t, void *key);
void		avl_put(struct avl *t, void *key, void *data);
void		avl_clr(struct avl *t, void *key);
struct anode *	avl_nnew(struct avl *t, void *key, void *data);
void		avl_nfree(struct avl *t, struct anode *node);

struct xavl {
	struct avl	avl;
	void *		ctx;
};


#include <cat/rbtree.h>

struct rbtree *	rb_new(int type);
void		rb_free(struct rbtree *t);
struct rbnode *	rb_snalloc(void *key, void *data, void *ctx);
void		rb_snfree(struct rbnode *n, void *ctx);
void *		rb_get(struct rbtree *t, void *key);
void		rb_put(struct rbtree *t, void *key, void *data);
void		rb_clr(struct rbtree *t, void *key);
struct rbnode *	rb_nnew(struct rbtree *t, void *key, void *data);
void		rb_nfree(struct rbtree *t, struct rbnode *node);

struct xrbtree {
	struct rbtree	rbt;
	void *		ctx;
};


#include <cat/splay.h>

struct splay *	st_new(int type);
void		st_free(struct splay *t);
struct stnode *	st_snalloc(void *key, void *data, void *ctx);
void		st_snfree(struct stnode *n, void *ctx);
void *		st_get(struct splay *t, void *key);
void		st_put(struct splay *t, void *key, void *data);
void		st_clr(struct splay *t, void *key);
struct stnode *	st_nnew(struct splay *t, void *key, void *data);
void		st_nfree(struct splay *t, struct stnode *node);

struct xsplay {
	struct splay	tree;
	void *		ctx;
};


#include <cat/heap.h>

struct heap * hp_new(int size, cmp_f cmp);
void	      hp_free(struct heap *hp);


#include <cat/ring.h>

char * ring_alloc(struct ring *r, size_t len);
int    ring_fmt(struct ring *r, const char *fmt, ...);


#include <cat/cb.h>

struct callback * cb_new(struct list *event, callback_f f, void *closure);
void              cb_clr(struct callback *cb);


#include <cat/match.h>

struct kmppat *	 kmp_pnew(struct raw *pat);
struct bmpat *	 bm_pnew(struct raw *pat);
struct sfxtree * sfx_new(struct raw *pat);
void		 sfx_free(struct sfxtree *sfx);


#include <cat/bitset.h>

struct safebitset {
	uint	nbits;
	uint	len;
	bitset_t *	set;
};

struct safebitset *sbs_new(uint nbits);
void sbs_free(struct safebitset *set);
void sbs_zero(struct safebitset *set);
void sbs_fill(struct safebitset *set);
uint sbs_copy(struct safebitset *dst, struct safebitset *src);
int  sbs_test(struct safebitset *set, uint index);
void sbs_set(struct safebitset *set, uint index);
void sbs_clr(struct safebitset *set, uint index);
void sbs_set_to(struct safebitset *set, uint index, int val);


#if CAT_HAS_POSIX 
#include <cat/time.h>
struct cat_time * tm_uget(struct cat_time *t);
#endif /* CAT_HAS_POSIX  */


#endif /* __cat_stduse_h */
