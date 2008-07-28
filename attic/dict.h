/*
 * cat/dict.h -- Abstract dictionary interface 
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2005 See accompanying license
 *
 */

#ifndef __cat_dict_h
#define __cat_dict_h

#include <cat/cat.h>
#include <cat/aux.h>

/* dictionary types */
#include <cat/list.h>
#include <cat/avl.h>
#include <cat/hash.h>
#include <cat/rbtree.h>

#define CAT_ST_STR	0
#define CAT_ST_RAW	1
#define CAT_ST_PTR	2
#define CAT_ST_NUM	3
#define CAT_ST_RAWCPY	4
#define CAT_ST_OTHER	5

#define CAT_DT_LIST		0
#define CAT_DT_HASHTABLE	1
#define CAT_DT_AVLTREE		2
#define CAT_DT_RBTREE		3

struct dictaux {
	cmp_f			cmp;
	hash_f			hash;
	void *			hctx;
	void			(*dinit)(void *d, void *
	void			(*ninit)
};

struct dict {
	int			storetype;
	int			dicttype;
	void *			(*nnew)(void *k, void *d, void *ctx);
	void *			(*nfree)(void *n, void *ctx);
	void *			nctx;
	struct dictaux *	aux;
};

struct dict * d_new(int dtype, int stype, struct dictsys *ds);
void          d_free(struct dict *d);
void *        d_get(struct dict *d, void *key);
void *        d_put(struct dict *d, void *key, void *data);
void *        d_clr(struct dict *d, void *key);

/* helpers */
struct rbnode *rb_snalloc(void *key, void *data, void *ctx);
void           rb_snfree(struct rbnode *n, void *ctx);

#endif /* __cat_dict_h */
