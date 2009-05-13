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


#include <cat/mem.h>

extern struct memmgr stdmem;
extern struct memmgr estdmem;

char *estrdup(const char *s);
struct raw *erawdup(struct raw const * const r);



#include <cat/list.h>

union clist_node_u { 
  struct list	entry;
  cat_align_t	align;
};

/* basic data access */
#define clist_data(node, type) (*clist_dptr(node, type))
#define clist_dptr(node, type) ((type *)((union clist_node_u *)node + 1))
#define clist_ptr2node(dptr) ((struct list *)((union clist_node_u *)dptr - 1))
#define clist_new(type) clist_new_sz(sizeof(type))

/* basic manipulation */
struct list *clist_new_sz(size_t len); /* create a new node */
struct list *clist_newlist();
void clist_init(struct list *list);
void clist_freelist(struct list *list);
void clist_clearlist(struct list *list);
int clist_isempty(struct list *list);
struct list *clist_insert(struct list *l, struct list *p, struct list *n);
struct list *clist_insert_head(struct list *list, struct list *node);
struct list *clist_insert_tail(struct list *list, struct list *node);
void clist_delete(struct list *l, struct list *n);
void clist_delete_head(struct list *l);
struct list *clist_head_node(struct list *list);

/* queue operations */
#define clist_enq(list, type, val)                                             \
  do { 								       \
  (clist_data(clist_insert_tail((list),clist_new(type)),type) = (val));  \
  } while (0)
#define clist_deq(list) clist_delete_head(list)
#define clist_qnext_node(list) clist_head_node(list)
#define clist_qnext(list, type) clist_data(clist_qnext_node(list), type)

/* stack operations */
#define clist_push(list, type, val)                                            \
  do { 								       \
  (clist_data(clist_insert_head((list), clist_new(type)),type) = (val)); \
  } while (0)
#define clist_pop(list) clist_delete_head(list)
#define clist_top_node(list) clist_head_node(list)
#define clist_top(list, type) clist_data(clist_top_node(list), type)


#include <cat/str.h>

char * str_copy_a(const char *src);
char * str_cat_a(const char *first, const char *second);
char * str_fmt_a(const char *fmt, ...);
char * str_vfmt_a(const char *fmt, va_list ap);
char * str_tok_a(char **start, const char *wschars);


#include <cat/dlist.h>

struct dlist * cdl_new(long sec, long nsec, void *data);
void *	       cdl_free(struct dlist *node);



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
struct hnode *	ht_snalloc(void *k, void *d, unsigned h, void *c);
void		ht_snfree(struct hnode *n, void *ctx);
void *		ht_get(struct htab *t, void *key);
void		ht_put(struct htab *t, void *key, void *data);
void		ht_clr(struct htab *t, void *key);
struct hnode *	ht_nnew(struct hashsys *sys, void *key, void *data, 
                        unsigned hash);
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
  unsigned int	nbits;
  unsigned int	len;
  bitset_t *	set;
};

struct safebitset *sbs_new(unsigned nbits);
void sbs_free(struct safebitset *set);
void sbs_zero(struct safebitset *set);
void sbs_fill(struct safebitset *set);
unsigned int sbs_copy(struct safebitset *dst, struct safebitset *src);
int  sbs_test(struct safebitset *set, unsigned int index);
void sbs_set(struct safebitset *set, unsigned int index);
void sbs_clr(struct safebitset *set, unsigned int index);
void sbs_set_to(struct safebitset *set, unsigned int index, int val);


#if CAT_HAS_POSIX 
#include <cat/time.h>
struct cat_time * tm_uget(struct cat_time *t);
#endif /* CAT_HAS_POSIX  */


#endif /* __cat_stduse_h */
