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

#define cln_intval	cln_data_u.au_intval
#define cln_uintval	cln_data_u.au_uintval
#define cln_pointer	cln_data_u.au_pointer
#define cln_raw		cln_data_u.au_raw
#define cln_attr_ptr	cln_data_u.au_data
#define l_to_cln(ln) 	container(ln, struct clist_node, cln_entry)
#define cln_next(clnp)	l_to_cln((clnp)->cln_entry.next)
#define cln_prev(clnp)	l_to_cln((clnp)->cln_entry.prev)
#define cln_data(clnp, type)	(*((type *)&(clnp)->cln_data_u))
#define cln_dptr(clnp)	((void *)((clnp)->cln_attr_ptr))
#define cl_head(clp)	(&(clp)->cl_base)
#define cl_end(clp)	(&(clp)->cl_base)
#define cl_first(clp)	l_to_cln((clp)->cl_base.cln_entry.next)
#define cl_last(clp)	l_to_cln((clp)->cl_base.cln_entry.prev)
#define clist_for_each(node, list)	\
	for ( (node) = cl_head(list) ;	\
	      (node) != cl_end(list) ;	\
	      (node) = cln_next(node) )

struct clist *clist_new_list(struct memmgr *mm, size_t dlen);
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


/* Enumeration used for the various dictionary data types that follow */
enum {
	CAT_KT_STR = 0,
	CAT_KT_BIN = 1,
	CAT_KT_RAW = 2,
	CAT_KT_PTR = 3,
	CAT_KT_NUM = 4
};


#include <cat/hash.h>

/* Application layer hash table functions */
struct htab *	ht_new(struct memmgr *mm, size_t nbkts, int ktype, size_t ksize,
		       size_t dsize);
void		ht_free(struct htab *t);
int		ht_get(struct htab *t, const void *key, void *res);
void *		ht_get_dptr(struct htab *t, const void *key);
int		ht_put(struct htab *t, const void *key, void *data);
int		ht_clr(struct htab *t, const void *key);


#include <cat/avl.h>

struct avl *	avl_new(struct memmgr *mm, int ktype, size_t ksiz, size_t dsiz);
void		avl_free(struct avl *t);
int		avl_get(struct avl *t, const void *key, void *res);
void *		avl_get_dptr(struct avl *t, const void *key);
int		avl_put(struct avl *t, const void *key, void *data);
int		avl_clr(struct avl *t, const void *key);


#include <cat/rbtree.h>

struct rbtree *	rb_new(struct memmgr *mm, int ktype, size_t ksiz, size_t dsiz);
void		rb_free(struct rbtree *t);
int		rb_get(struct rbtree *t, const void *key, void *res);
void *		rb_get_dptr(struct rbtree *t, const void *key);
int		rb_put(struct rbtree *t, const void *key, void *data);
int		rb_clr(struct rbtree *t, const void *key);


#include <cat/splay.h>

struct splay *	st_new(struct memmgr *mm, int ktype, size_t ksiz, size_t dsiz);
void		st_free(struct splay *t);
int		st_get(struct splay *t, const void *key, void *res);
void *		st_get_dptr(struct splay *t, const void *key);
int		st_put(struct splay *t, const void *key, void *data);
int		st_clr(struct splay *t, const void *key);


#include <cat/heap.h>

struct heap * hp_new(struct memmgr *mm, int size, cmp_f cmp);
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
void sbs_set_to(struct safebitset *set, uint index, int val);


#if CAT_HAS_POSIX 
#include <cat/time.h>
struct cat_time * tm_uget(struct cat_time *t);
#endif /* CAT_HAS_POSIX  */


#endif /* __cat_stduse_h */
