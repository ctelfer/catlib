/*
 * stduse.c -- application level nicities built on the rest of the catlib
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007-2015  See accompanying license
 * 
 */

#include <cat/cat.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cat/err.h>
#include <cat/stduse.h>
#include <cat/grow.h>
#include <cat/bitset.h>
#include <cat/emalloc.h>

#if CAT_HAS_POSIX
#include <sys/time.h>
#endif /* CAT_HAS_POSIX */

#ifndef va_copy
#ifndef __va_copy
#error "No va_copy() or __va_copy() macros!"
#else /* __va_copy */
#define va_copy(dst, src) __va_copy(dst, src)
#endif /* __va_copy */
#endif /* va_copy */


#define SMAX (~(size_t)0)


/* Memory operations */

static void * std_ealloc(struct memmgr *mm, size_t size) 
{
	void *m;
	abort_unless(mm && size > 0);
	if ( !(m = malloc(size)) )
		errsys("malloc: ");
	return m;
}


static void * std_eresize(struct memmgr *mm, void *old, size_t newsize) 
{
	void *m;
	abort_unless(mm && newsize >= 0);
	if ( !(m = realloc(old, newsize)) )
		errsys("realloc: ");
	return m;
}


static void std_efree(struct memmgr *mm, void *p)
{
	free(p);
}


struct memmgr estdmm = {
	std_ealloc,
	std_eresize,
	std_efree,
	&estdmm
};


union raw_u {
	struct raw    raw;
	cat_align_t   align;
};


struct raw *erawdup(struct raw const * const r)
{
	size_t s;
	struct raw *rnew;
	if ( !r || !r->data || !r->len )
		err("erawdup: invalid raw provided\n");
	s = r->len + sizeof(union raw_u);
	if ( s < sizeof(union raw_u) )
		err("erawdup: integer overflow\n");
	rnew = emalloc(s);
	rnew->len = r->len;
	rnew->data = (byte_t *)((union raw_u *)rnew + 1);
	memcpy(rnew->data, r->data, r->len);
	return rnew;
}


/* List operations */


static struct clist_node *cl_def_node_alloc(struct clist *list)
{
	return malloc(sizeof(struct clist_node));
}


static void cl_def_node_free(struct clist *list, struct clist_node *node)
{
	free(node);
}


static struct clist_attr cl_def_attr = {
	cl_def_node_alloc,
	cl_def_node_free,
	{ 0 }
};


struct clist *cl_new(const struct clist_attr *attr, int abort_on_fail)
{
	struct clist *list;

	if ( attr == NULL )
		attr = &cl_def_attr;

	abort_unless(attr->node_alloc != NULL);
	abort_unless(attr->node_free != NULL);

	list = malloc(sizeof(*list));
	if ( list == NULL ) {
		if ( abort_on_fail )
			err("cl_new: unable to allocate list\n");
		return NULL;
	}

	l_init(&list->base.entry);
	list->base.list = NULL;
	list->fill = 0;
	list->abort_on_fail = abort_on_fail;
	list->node_alloc = attr->node_alloc;
	list->node_free = attr->node_free;
	list->ctx = attr->ctx;

	return list;
}


void cl_free(struct clist *list)
{
	while ( !cl_isempty(list) )
		cl_del(list, cl_first(list));
	free(list);
}


int cl_isempty(struct clist *list)
{
	return list->fill == 0;
}


size_t cl_fill(struct clist *list)
{
	return list->fill;
}


struct clist_node *cl_node_new(struct clist *list, void *val)
{
	struct clist_node *n = (*list->node_alloc)(list);
	if ( n == NULL && list->abort_on_fail )
		err("cl_node_new: unable to allocate node\n");
	return n;
}


int cl_ins(struct clist *list, struct clist_node *prev, void *val)
{
	struct clist_node *node;

	node = (*list->node_alloc)(list);
	if ( node == NULL ) {
		if ( list->abort_on_fail )
			err("cl_ins: unable to allocate NULL\n");
		return -1;
	}

	if ( prev == NULL )
		prev = cl_head(list);
	l_ins(&prev->entry, &node->entry);
	node->list = list;
	node->data = val;
	list->fill += 1;

	return 0;
}


void *cl_del(struct clist *list, struct clist_node *node)
{
	void *p = node->data;
	if ( node->list != NULL ) {
		l_rem(&node->entry);
		node->list->fill -= 1;
		node->list = NULL;
	}
	(*list->node_free)(list, node);
	return p;
}


int cl_enq(struct clist *list, void *val)
{
	return cl_ins(list, cl_last(list), val);
}


void *cl_deq(struct clist *list)
{
	if ( list->fill == 0 )
		return NULL;
	return cl_del(list, cl_first(list));
}


int cl_push(struct clist *list, void *val)
{
	return cl_ins(list, NULL, val);
}


void *cl_pop(struct clist *list)
{
	if ( list->fill == 0 )
		return NULL;
	return cl_del(list, cl_first(list));
}


void *cl_top(struct clist *list)
{
	if ( list->fill == 0 )
		return NULL;
	return cl_first(list)->data;
}


void cl_apply(struct clist *list, apply_f f, void *arg)
{
	struct clist_node *cur, *next;

	abort_unless(list != NULL);
	abort_unless(f != NULL);

	for ( cur = cl_head(list) ; cur != cl_end(list) ; cur = next ) {
		next = cl_next(cur);
		(*f)(cur, arg);
	}
}


/* String functions */


char * str_copy_a(const char *src)
{
	size_t len;
	char *newstr;

	len = strlen(src) + 1;
	abort_unless(len > 0);
	newstr = emalloc(len);
	return memmove(newstr, src, len);
}


char * str_cat_a(const char *first, const char *second)
{
	size_t len1, len2, tlen;
	char *newstr;

	len1 = strlen(first);
	len2 = strlen(second);
	abort_unless((size_t)~0 - 1 >= len1);
	abort_unless((size_t)~0 - 1 - len1 >= len2);
	tlen = len1 + len2 + 1;

	newstr = emalloc(tlen);
	memmove(newstr, first, len1);
	return memmove(newstr + len1, second, len2 + 1);
}


char * str_fmt_a(const char *fmt, ...)
{
	va_list ap;
	char *newstr;

	va_start(ap, fmt);
	newstr = str_vfmt_a(fmt, ap);
	va_end(ap);

	return newstr;
}


char * str_vfmt_a(const char *fmt, va_list ap) 
{
	int len;
	char *buf;
	va_list oap;

	abort_unless(fmt);

	va_copy(oap, ap);
	len = str_vfmt(NULL, 0, fmt, ap);

	if ( len < 0 )
		return NULL;
	abort_unless((size_t)~0 - 1 >= len);
	buf = emalloc(len + 1);
	len = str_vfmt(buf, len+1, fmt, oap);
	abort_unless(len >= 0);

	return buf;
}


static int is_space(const char *s, const char *wschars)
{
	abort_unless(s && wschars);
	/* TODO make UTF8-compliant */
	while ( *wschars != '\0' )
		if ( *wschars++ == *s )
			return 1;
	return 0;
}


char * str_tok_a(char **start, const char *wschars)
{
	char *s, *e, *rs;
	size_t tlen;

	if ( !wschars )
		wschars = " \t\r\n";
	if ( !start || !*start )
		return NULL;

	s = *start;
	while ( *s != '\0' && is_space(s, wschars) )
		s++;

	if ( *s == '\0' ) {
		*start = s;
		return NULL;
	}

	e = s + 1;
	while ( *e != '\0' && !is_space(e, wschars) )
		e++;

	tlen = e - s;
	rs = emalloc(tlen+1);
	memmove(rs, s, tlen);
	rs[tlen] = '\0';
	*start = e;

	return rs;
}




/* Delta list functions */


void * cdl_data(struct dlist *nodep)
{

	abort_unless(nodep);
	return container(nodep, struct cdlist, entry)->data;
}


void cdl_set(struct dlist *nodep, void *data)
{
	abort_unless(nodep);
	container(nodep, struct cdlist, entry)->data = data;
}


struct dlist * cdl_new(cat_time_t t, void *data)
{
	struct cdlist *node;

	node = emalloc(sizeof(*node));
	dl_init(&node->entry, t);
	node->data = data;

	return &node->entry;
}


void * cdl_free(struct dlist * nodep)
{
	void *old = NULL;
	struct cdlist *node;

	if ( nodep ) {
		node = container(nodep, struct cdlist, entry);
		old = node->data;
		free(node);
	}

	return old;
}




/* Used by all xxx_apply() functions to store the real apply function */
/* when wrapping with a new apply function to get the data pointer. */
struct apply_ctx {
	void *ctx;
	apply_f f;
};




/* Hash table functions */

static struct chnode *cht_node_alloc_skey(struct chtab *t, void *key)
{
	char *kcpy;
	struct chnode *chn;
	kcpy = strdup(key);
	if ( kcpy == NULL )
		return NULL;
	chn = malloc(sizeof(*chn));
	if ( chn == NULL ) {
		free(kcpy);
		return NULL;
	}
	ht_ninit(&chn->node, kcpy);
	return chn;
}


static void cht_node_free_skey(struct chtab *t, struct chnode *chn)
{
	abort_unless(chn != NULL);
	abort_unless(chn->node.key != NULL);
	free(chn->node.key);
	free(chn);
}


struct chtab_attr cht_std_attr_skey = {
	&cmp_str,
	&ht_shash,
	0,
	&cht_node_alloc_skey,
	&cht_node_free_skey,
	NULL,
};


static struct chnode *cht_node_alloc_rkey(struct chtab *t, void *key)
{
	struct chnode *chn;
	struct raw *rkey = key;
	struct raw *rnode;

	abort_unless(rkey != NULL);
	chn = malloc(CAT_ALIGN_SIZE(sizeof(*chn)) +
		     CAT_ALIGN_SIZE(sizeof(*rnode)) +
		     rkey->len);
	if ( chn == NULL )
		return NULL;

	rnode = (struct raw *)((byte_t *)chn + CAT_ALIGN_SIZE(sizeof(*chn)));
	rnode->len = rkey->len;
	if ( rkey->len > 0 ) {
		rnode->data = (byte_t *)rnode + CAT_ALIGN_SIZE(sizeof(*rnode));
		memmove(rnode->data, rkey->data, rkey->len);
	} else {
		rnode->data = NULL;
	}
	ht_ninit(&chn->node, rnode);
	return chn;
}


static void cht_node_free_rkey(struct chtab *t, struct chnode *chn)
{
	free(chn);
}


struct chtab_attr cht_std_attr_rkey = {
	&cmp_raw,
	&ht_rhash,
	0,
	&cht_node_alloc_rkey,
	&cht_node_free_rkey,
	NULL,
};


static struct chnode *cht_node_alloc_pkey(struct chtab *t, void *key)
{
	struct chnode *chn;
	chn = malloc(sizeof(*chn));
	if ( chn == NULL )
		return NULL;
	ht_ninit(&chn->node, key);
	return chn;
}


static void cht_node_free_pkey(struct chtab *t, struct chnode *chn)
{
	free(chn);
}


struct chtab_attr cht_std_attr_pkey = {
	&cmp_ptr,
	&ht_phash,
	0,
	&cht_node_alloc_pkey,
	&cht_node_free_pkey,
	NULL,
};


struct chtab_attr cht_std_attr_bkey = {
	NULL,		/* Must be supplied by user */
	NULL,		/* Must be supplied by user */
	0,		/* Must be supplied by user */
	&cht_node_alloc_pkey,
	&cht_node_free_pkey,
	NULL,
};


struct chtab *cht_new(size_t nbkts, struct chtab_attr *attr, void *hctx,
		      int abort_on_fail)
{
	size_t n;
	size_t tsize;
	size_t hctx_size;
	struct chtab *t;
	void *new_hctx;
	struct hnode **buckets;

	if ( attr == NULL )
		attr = &cht_std_attr_skey;

	abort_unless(attr->kcmp != NULL);
	abort_unless(attr->hash != NULL);
	abort_unless(attr->node_alloc != NULL);
	abort_unless(attr->node_free != NULL);

	tsize = CAT_ALIGN_SIZE(sizeof(struct chtab));
	hctx_size = CAT_ALIGN_SIZE(attr->hctx_size);
	abort_unless(hctx_size >= attr->hctx_size);
	abort_unless(SMAX - hctx_size >= tsize);
	n = hctx_size + tsize;
	abort_unless((SMAX - n) / sizeof(struct hnode *) >= nbkts);
	n += sizeof(struct hnode *) * nbkts;

	t = emalloc(n);
	if ( t == NULL ) {
		if ( abort_on_fail )
			err("cht_new: unable to allocate table\n");
		return NULL;
	}

	new_hctx = (byte_t *)t + tsize;
	buckets = (struct hnode **)((byte_t *)new_hctx + hctx_size);

	if (hctx_size != 0)
		memmove(new_hctx, hctx, attr->hctx_size);
	else
		new_hctx = NULL;

	ht_init(&t->table, buckets, nbkts, attr->kcmp, attr->hash, new_hctx);
	t->abort_on_fail = abort_on_fail;
	t->node_alloc = attr->node_alloc;
	t->node_free = attr->node_free;
	t->ctx = attr->ctx;

	return t;
}


void cht_free(struct chtab *t)
{
	unsigned i;
	struct chnode *chn;

	abort_unless(t != NULL);

	for ( i = 0; i < t->table.nbkts ; ++i ) {
		while ( t->table.bkts[i] != NULL ) {
			chn = container(t->table.bkts[i], struct chnode, node);
			ht_rem(&chn->node);
			(*t->node_free)(t, chn);
		}
	}
	free(t);
}


void *cht_get(struct chtab *t, void *key)
{
	struct hnode *hn;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	hn = ht_lkup(&t->table, key, NULL);
	if ( hn != NULL )
		return container(hn, struct chnode, node)->data;
	return NULL;
}


int cht_put(struct chtab *t, void *key, void *data)
{
	struct hnode *hn;
	struct chnode *chn;
	uint h;

	abort_unless(t != NULL);
	abort_unless(key != NULL);
	abort_unless(data != NULL);

	hn = ht_lkup(&t->table, key, &h);
	if ( hn != NULL ) {
		chn = container(hn, struct chnode, node);
		chn->data = data;
		return 1;
	}

	chn = (*t->node_alloc)(t, key);
	if ( chn == NULL ) {
		if ( t->abort_on_fail )
			err("cht_put: unable to allocate node\n");
		return -1;
	}

	chn->data = data;
	ht_ins(&t->table, &chn->node, h);
	return 0;
}


void *cht_del(struct chtab *t, void *key)
{
	struct hnode *hn;
	struct chnode *chn;
	void *data = NULL;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	hn = ht_lkup(&t->table, key, NULL);
	if ( hn != NULL ) {
		ht_rem(hn);
		chn = container(hn, struct chnode, node);
		data = chn->data;
		(*t->node_free)(t, chn);
	}
	return data;
}


static void cht_apply_wrap(void *p, void *ctx)
{
	struct chnode *chn = p;
	struct apply_ctx *ac = ctx;
	(*ac->f)(chn->data, ac->ctx);
}


void cht_apply(struct chtab *t, apply_f f, void *ctx)
{
	struct apply_ctx ac;
	ac.ctx = t->ctx;
	ac.f = f;
	ht_apply(&t->table, &cht_apply_wrap, &ac);
}




/* AVL Trees */

static struct canode *cavl_node_alloc_skey(struct cavltree *t, void *key)
{
	char *kcpy;
	struct canode *can;
	kcpy = strdup(key);
	if ( kcpy == NULL )
		return NULL;
	can = malloc(sizeof(*can));
	if ( can == NULL ) {
		free(kcpy);
		return NULL;
	}
	avl_ninit(&can->node, kcpy);
	return can;
}


static void cavl_node_free_skey(struct cavltree *t, struct canode *can)
{
	abort_unless(can != NULL);
	abort_unless(can->node.key != NULL);
	free(can->node.key);
	free(can);
}


struct cavltree_attr cavl_std_attr_skey = {
	&cmp_str,
	&cavl_node_alloc_skey,
	&cavl_node_free_skey,
	0,
};


static struct canode *cavl_node_alloc_rkey(struct cavltree *t, void *key)
{
	struct canode *can;
	struct raw *rkey = key;
	struct raw *rnode;

	abort_unless(rkey != NULL);
	can = malloc(CAT_ALIGN_SIZE(sizeof(*can)) +
		     CAT_ALIGN_SIZE(sizeof(*rnode)) +
		     rkey->len);
	if ( can == NULL )
		return NULL;

	rnode = (struct raw *)((byte_t *)can + CAT_ALIGN_SIZE(sizeof(*can)));
	rnode->len = rkey->len;
	if ( rkey->len > 0 ) {
		rnode->data = (byte_t *)rnode + CAT_ALIGN_SIZE(sizeof(*rnode));
		memmove(rnode->data, rkey->data, rkey->len);
	} else {
		rnode->data = NULL;
	}
	avl_ninit(&can->node, rnode);
	return can;
}


static void cavl_node_free_rkey(struct cavltree *t, struct canode *can)
{
	free(can);
}


struct cavltree_attr cavl_std_attr_rkey = {
	&cmp_raw,
	&cavl_node_alloc_rkey,
	&cavl_node_free_rkey,
	0,
};


static struct canode *cavl_node_alloc_pkey(struct cavltree *t, void *key)
{
	struct canode *can;
	can = malloc(sizeof(*can));
	if ( can == NULL )
		return NULL;
	avl_ninit(&can->node, key);
	return can;
}


static void cavl_node_free_pkey(struct cavltree *t, struct canode *can)
{
	free(can);
}


struct cavltree_attr cavl_std_attr_pkey = {
	&cmp_ptr,
	&cavl_node_alloc_pkey,
	&cavl_node_free_pkey,
	0,
};


struct cavltree_attr cavl_std_attr_bkey = {
	NULL, 			/* Must be supplied by user */
	&cavl_node_alloc_pkey,
	&cavl_node_free_pkey,
	0,
};


struct cavltree *cavl_new(struct cavltree_attr *attr, int abort_on_fail)
{
	struct cavltree *t;

	if ( attr == NULL )
		attr = &cavl_std_attr_skey;

	abort_unless(attr->kcmp != NULL);
	abort_unless(attr->node_alloc != NULL);
	abort_unless(attr->node_free != NULL);

	t = malloc(sizeof(*t));
	if ( t == NULL ) {
		if ( abort_on_fail )
			err("cavl_new: unable to allocate tree\n");
		return NULL;
	}

	avl_init(&t->tree, attr->kcmp);
	t->abort_on_fail = abort_on_fail;
	t->node_alloc = attr->node_alloc;
	t->node_free = attr->node_free;
	t->ctx = attr->ctx;

	return t;
}


void cavl_free(struct cavltree *t)
{
	struct anode *an;
	struct canode *can;

	abort_unless(t != NULL);

	while ( (an = avl_getroot(&t->tree)) != NULL ) {
		avl_rem(an);
		can = container(an, struct canode, node);
		(*t->node_free)(t, can);
	}
	free(t);
}


void *cavl_get(struct cavltree *t, void *key)
{
	struct anode *an;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	an = avl_lkup(&t->tree, key, NULL);
	if ( an != NULL )
		return container(an, struct canode, node)->data;
	return NULL;
}


int cavl_put(struct cavltree *t, void *key, void *data)
{
	int dir;
	struct anode *an;
	struct canode *can;

	abort_unless(t != NULL);
	abort_unless(key != NULL);


	an = avl_lkup(&t->tree, key, &dir);
	if ( dir == CA_N ) {
		can = container(an, struct canode, node);
		can->data = data;
		return 1;
	}

	can = (*t->node_alloc)(t, key);
	if ( can == NULL ) {
		if ( t->abort_on_fail )
			err("cavl_put: unable to allocate node\n");
		return -1;
	}

	can->data = data;
	avl_ins(&t->tree, &can->node, an, dir);
	return 0;
}


void *cavl_del(struct cavltree *t, void *key)
{
	struct anode *an;
	struct canode *can;
	void *data = NULL;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	an = avl_lkup(&t->tree, key, NULL);
	if ( an != NULL ) {
		can = container(an, struct canode, node);
		data = can->data;
		avl_rem(an);
		(*t->node_free)(t, can);
	}
	return data;
}


static void cavl_apply_wrap(void *p, void *ctx)
{
	struct canode *can = p;
	struct apply_ctx *ac = ctx;
	(*ac->f)(can->data, ac->ctx);
}


void cavl_apply(struct cavltree *t, apply_f f, void *ctx)
{
	struct apply_ctx ac;
	ac.ctx = t->ctx;
	ac.f = f;
	avl_apply(&t->tree, &cavl_apply_wrap, &ac);
}




/* Red-Black Trees */

static struct crbnode *crb_node_alloc_skey(struct crbtree *t, void *key)
{
	char *kcpy;
	struct crbnode *crn;
	kcpy = strdup(key);
	if ( kcpy == NULL )
		return NULL;
	crn = malloc(sizeof(*crn));
	if ( crn == NULL ) {
		free(kcpy);
		return NULL;
	}
	rb_ninit(&crn->node, kcpy);
	return crn;
}


static void crb_node_free_skey(struct crbtree *t, struct crbnode *crn)
{
	abort_unless(crn != NULL);
	abort_unless(crn->node.key != NULL);
	free(crn->node.key);
	free(crn);
}


struct crbtree_attr crb_std_attr_skey = {
	&cmp_str,
	&crb_node_alloc_skey,
	&crb_node_free_skey,
	0,
};


static struct crbnode *crb_node_alloc_rkey(struct crbtree *t, void *key)
{
	struct crbnode *crn;
	struct raw *rkey = key;
	struct raw *rnode;

	abort_unless(rkey != NULL);
	crn = malloc(CAT_ALIGN_SIZE(sizeof(*crn)) +
		     CAT_ALIGN_SIZE(sizeof(struct raw)) +
		     rkey->len);
	if ( crn == NULL )
		return NULL;

	rnode = (struct raw *)((byte_t *)crn + CAT_ALIGN_SIZE(sizeof(*crn)));
	rnode->len = rkey->len;
	if ( rkey->len > 0 ) {
		rnode->data = (byte_t *)rnode + CAT_ALIGN_SIZE(sizeof(*rnode));
		memmove(rnode->data, rkey->data, rkey->len);
	} else {
		rnode->data = NULL;
	}
	rb_ninit(&crn->node, rnode);
	return crn;
}


static void crb_node_free_rkey(struct crbtree *t, struct crbnode *crn)
{
	free(crn);
}


struct crbtree_attr crb_std_attr_rkey = {
	&cmp_raw,
	&crb_node_alloc_rkey,
	&crb_node_free_rkey,
	0,
};


static struct crbnode *crb_node_alloc_pkey(struct crbtree *t, void *key)
{
	struct crbnode *crn;
	crn = malloc(sizeof(*crn));
	if ( crn == NULL )
		return NULL;
	rb_ninit(&crn->node, key);
	return crn;
}


static void crb_node_free_pkey(struct crbtree *t, struct crbnode *crn)
{
	free(crn);
}


struct crbtree_attr crb_std_attr_pkey = {
	&cmp_ptr,
	&crb_node_alloc_pkey,
	&crb_node_free_pkey,
	0,
};


struct crbtree_attr crb_std_attr_bkey = {
	NULL, 			/* Must be supplied by user */
	&crb_node_alloc_pkey,
	&crb_node_free_pkey,
	0,
};


struct crbtree *crb_new(struct crbtree_attr *attr, int abort_on_fail)
{
	struct crbtree *t;

	if ( attr == NULL )
		attr = &crb_std_attr_skey;

	abort_unless(attr->kcmp != NULL);
	abort_unless(attr->node_alloc != NULL);
	abort_unless(attr->node_free != NULL);

	t = malloc(sizeof(*t));
	if ( t == NULL ) {
		if ( abort_on_fail )
			err("crb_new: unable to allocate tree\n");
		return NULL;
	}

	rb_init(&t->tree, attr->kcmp);
	t->abort_on_fail = abort_on_fail;
	t->node_alloc = attr->node_alloc;
	t->node_free = attr->node_free;
	t->ctx = attr->ctx;

	return t;
}


void crb_free(struct crbtree *t)
{
	struct rbnode *rn;
	struct crbnode *crn;

	abort_unless(t != NULL);

	while ( (rn = rb_getroot(&t->tree)) != NULL ) {
		rb_rem(rn);
		crn = container(rn, struct crbnode, node);
		(*t->node_free)(t, crn);
	}
	free(t);
}


void *crb_get(struct crbtree *t, void *key)
{
	struct rbnode *rn;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	rn = rb_lkup(&t->tree, key, NULL);
	if ( rn != NULL )
		return container(rn, struct crbnode, node)->data;
	return NULL;
}


int crb_put(struct crbtree *t, void *key, void *data)
{
	int dir;
	struct rbnode *rn;
	struct crbnode *crn;

	abort_unless(t != NULL);
	abort_unless(key != NULL);


	rn = rb_lkup(&t->tree, key, &dir);
	if ( dir == CRB_N ) {
		crn = container(rn, struct crbnode, node);
		crn->data = data;
		return 1;
	}

	crn = (*t->node_alloc)(t, key);
	if ( crn == NULL ) {
		if ( t->abort_on_fail )
			err("crb_put: unable to allocate node\n");
		return -1;
	}

	crn->data = data;
	rb_ins(&t->tree, &crn->node, rn, dir);
	return 0;
}


void *crb_del(struct crbtree *t, void *key)
{
	struct rbnode *rn;
	struct crbnode *crn;
	void *data = NULL;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	rn = rb_lkup(&t->tree, key, NULL);
	if ( rn != NULL ) {
		crn = container(rn, struct crbnode, node);
		data = crn->data;
		rb_rem(rn);
		(*t->node_free)(t, crn);
	}
	return data;
}


static void crb_apply_wrap(void *p, void *ctx)
{
	struct crbnode *crn = p;
	struct apply_ctx *ac = ctx;
	(*ac->f)(crn->data, ac->ctx);
}


void crb_apply(struct crbtree *t, apply_f f, void *ctx)
{
	struct apply_ctx ac;
	ac.ctx = t->ctx;
	ac.f = f;
	rb_apply(&t->tree, &crb_apply_wrap, &ac);
}




/* Splay Trees */

static struct cstnode *cst_node_alloc_skey(struct cstree *t, void *key)
{
	char *kcpy;
	struct cstnode *csn;
	kcpy = strdup(key);
	if ( kcpy == NULL )
		return NULL;
	csn = malloc(sizeof(*csn));
	if ( csn == NULL ) {
		free(kcpy);
		return NULL;
	}
	st_ninit(&csn->node, kcpy);
	return csn;
}


static void cst_node_free_skey(struct cstree *t, struct cstnode *csn)
{
	abort_unless(csn != NULL);
	abort_unless(csn->node.key != NULL);
	free(csn->node.key);
	free(csn);
}


struct cstree_attr cst_std_attr_skey = {
	&cmp_str,
	&cst_node_alloc_skey,
	&cst_node_free_skey,
	0,
};


static struct cstnode *cst_node_alloc_rkey(struct cstree *t, void *key)
{
	struct cstnode *csn;
	struct raw *rkey = key;
	struct raw *rnode;

	abort_unless(rkey != NULL);
	csn = malloc(CAT_ALIGN_SIZE(sizeof(*csn)) +
		     CAT_ALIGN_SIZE(sizeof(struct raw)) +
		     rkey->len);
	if ( csn == NULL )
		return NULL;

	rnode = (struct raw *)((byte_t *)csn + CAT_ALIGN_SIZE(sizeof(*csn)));
	rnode->len = rkey->len;
	if ( rkey->len > 0 ) {
		rnode->data = (byte_t *)rnode + CAT_ALIGN_SIZE(sizeof(*rnode));
		memmove(rnode->data, rkey->data, rkey->len);
	} else {
		rnode->data = NULL;
	}
	st_ninit(&csn->node, rnode);
	return csn;
}


static void cst_node_free_rkey(struct cstree *t, struct cstnode *csn)
{
	free(csn);
}


struct cstree_attr cst_std_attr_rkey = {
	&cmp_raw,
	&cst_node_alloc_rkey,
	&cst_node_free_rkey,
	0,
};


static struct cstnode *cst_node_alloc_pkey(struct cstree *t, void *key)
{
	struct cstnode *csn;
	csn = malloc(sizeof(*csn));
	if ( csn == NULL )
		return NULL;
	st_ninit(&csn->node, key);
	return csn;
}


static void cst_node_free_pkey(struct cstree *t, struct cstnode *csn)
{
	free(csn);
}


struct cstree_attr cst_std_attr_pkey = {
	&cmp_ptr,
	&cst_node_alloc_pkey,
	&cst_node_free_pkey,
	0,
};


struct cstree_attr cst_std_attr_bkey = {
	NULL,			/* Must be supplied by user */
	&cst_node_alloc_pkey,
	&cst_node_free_pkey,
	0,
};


struct cstree *cst_new(struct cstree_attr *attr, int abort_on_fail)
{
	struct cstree *t;

	if ( attr == NULL )
		attr = &cst_std_attr_skey;

	abort_unless(attr->kcmp != NULL);
	abort_unless(attr->node_alloc != NULL);
	abort_unless(attr->node_free != NULL);

	t = malloc(sizeof(*t));
	if ( t == NULL ) {
		if ( abort_on_fail )
			err("cst_new: unable to allocate tree\n");
		return NULL;
	}

	st_init(&t->tree, attr->kcmp);
	t->abort_on_fail = abort_on_fail;
	t->node_alloc = attr->node_alloc;
	t->node_free = attr->node_free;
	t->ctx = attr->ctx;

	return t;
}


void cst_free(struct cstree *t)
{
	struct stnode *sn;
	struct cstnode *csn;

	abort_unless(t != NULL);

	while ( (sn = st_getroot(&t->tree)) != NULL ) {
		st_rem(sn);
		csn = container(sn, struct cstnode, node);
		(*t->node_free)(t, csn);
	}
	free(t);
}


void *cst_get(struct cstree *t, void *key)
{
	struct stnode *sn;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	sn = st_lkup(&t->tree, key);
	if ( sn != NULL )
		return container(sn, struct cstnode, node)->data;
	return NULL;
}


int cst_put(struct cstree *t, void *key, void *data)
{
	struct stnode *sn;
	struct cstnode *csn;

	abort_unless(t != NULL);
	abort_unless(key != NULL);


	sn = st_lkup(&t->tree, key);
	if ( sn != NULL ) {
		csn = container(sn, struct cstnode, node);
		csn->data = data;
		return 1;
	}

	csn = (*t->node_alloc)(t, key);
	if ( csn == NULL ) {
		if ( t->abort_on_fail )
			err("cst_put: unable to allocate node\n");
		return -1;
	}

	csn->data = data;
	st_ins(&t->tree, &csn->node);
	return 0;
}


void *cst_del(struct cstree *t, void *key)
{
	struct stnode *sn;
	struct cstnode *csn;
	void *data = NULL;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	sn = st_lkup(&t->tree, key);
	if ( sn != NULL ) {
		csn = container(sn, struct cstnode, node);
		data = csn->data;
		st_rem(sn);
		(*t->node_free)(t, csn);
	}
	return data;
}


static void cst_apply_wrap(void *p, void *ctx)
{
	struct cstnode *csn = p;
	struct apply_ctx *ac = ctx;
	(*ac->f)(csn->data, ac->ctx);
}


void cst_apply(struct cstree *t, apply_f f, void *ctx)
{
	struct apply_ctx ac;
	ac.ctx = t->ctx;
	ac.f = f;
	st_apply(&t->tree, &cst_apply_wrap, &ac);
}




/* Heap operations */


struct heap *hp_new(size_t size, cmp_f cmp)
{
	struct heap *hp;
	void **elem = NULL;

	abort_unless(cmp);
	abort_unless(SMAX / sizeof(void *) >= size);

	hp = malloc(sizeof(struct heap));
	if ( hp == NULL )
		return NULL;

	if ( size > 0 ) {
		elem = malloc(size * sizeof(void*));
		if ( elem == NULL ) {
			free(hp);
			return NULL;
		}
	}
	hp_init(hp, elem, size, 0, cmp, &stdmm);

	return hp;
}


void hp_free(struct heap *hp)
{
	abort_unless(hp != NULL);
	mem_free(&stdmm, hp->elem);
	free(hp);
}




/* Ring operations */


#define CKRING(r) do {							\
	abort_unless(r); 						\
	abort_unless(((r)->alloc > 0) && ((r)->start < (r)->alloc));	\
	abort_unless((r)->len <= (r)->alloc);				\
	} while (0)


char * ring_alloc(struct ring *r, size_t len)
{
	size_t osiz, last, toend;

	CKRING(r);

	if (len > CAT_MAXGROW - r->len)
		err("ring_alloc: request for %ld bytes too much\n", len);
	last = ring_last(r);
	if ( r->alloc - r->len >= len ) {
		if (len > r->alloc - last) {
			memmove(r->data, r->data + r->start, r->len);
			r->start = 0;
		}
		return (char *)r->data + r->len;
	}

	osiz = r->alloc;
	/* XXX extra (void *) cast is to shut up the compiler */
	if ( grow(&r->data, &r->alloc, r->len + len) < 0 )
		err("ring_alloc: out of memory\n");

	if ( last < r->start ) {
		toend = osiz - r->start;
		memmove(r->data + r->start + (r->alloc - osiz), 
			r->data + r->start, toend);
		r->start += r->alloc - osiz;
	} else if ( len > r->alloc - last ) {
		memmove(r->data, r->data + r->start, r->len);
		r->start = 0;
		last = r->len;
	}

	return (char *)r->data + last;
}


int ring_fmt(struct ring *r, const char *fmt, ...)
{
	size_t a;
	int rv;
	va_list ap;
	char *d;

	abort_unless(r);

	a = ring_avail(r);
	while ( 1 ) {
		va_start(ap, fmt);
		d = ring_alloc(r, a);
		if ( !d ) 
			return -1;
		rv = vsnprintf(d, a, fmt, ap);
		va_end(ap);

		if ( rv > a )
			a = rv;
		else if ( rv < 0 )
			a <<= 1;
		else
			break;
	}

	r->len += rv;
	return rv;
}

#undef CKRING




/* Matching functions */


struct kmppat * kmp_pnew(struct memmgr *mm, struct raw *pat)
{
	struct std_kmppat *kmp;

	abort_unless(pat && pat->data && pat->len);
	abort_unless(mm != NULL);

	kmp = mem_get(mm, sizeof(*kmp) + pat->len * sizeof(ulong));
	if ( kmp != NULL ) {
		kmp_pinit(&kmp->pattern, pat, (ulong *)(kmp + 1));
		kmp->mm = mm;
		return &kmp->pattern;
	} else {
		return NULL;
	}
}


void kmp_free(struct kmppat *kmp)
{
	struct std_kmppat *base;
	struct memmgr *mm;
	abort_unless(kmp != NULL);
	base = container(kmp, struct std_kmppat, pattern);
	mm = base->mm;
	abort_unless(mm != NULL);
	mem_free(mm, base);
}


struct bmpat * bm_pnew(struct memmgr *mm, struct raw *pat)
{
	struct bmpat *bmp;
	ulong *scratch;

	abort_unless(pat && pat->data && pat->len);
	scratch = mem_get(mm, pat->len * sizeof(ulong));
	if ( scratch == NULL ) 
		return NULL;
	bmp = mem_get(mm, sizeof(*bmp) + pat->len * sizeof(ulong));
	if ( bmp != NULL )
		bm_pinit(bmp, pat, (ulong *)(bmp + 1), scratch);
	free(scratch);
	return bmp;
}


void bm_free(struct bmpat *bmp)
{
	struct std_bmpat *base;
	struct memmgr *mm;
	abort_unless(bmp != NULL);
	base = container(bmp, struct std_bmpat, pattern);
	mm = base->mm;
	abort_unless(mm != NULL);
	mem_free(mm, base);
}


struct sfxtree *sfx_new(struct memmgr *mm, struct raw *str)
{
	struct sfxtree *sfx;
	if ( (sfx = mem_get(mm, sizeof(*sfx))) != NULL ) {
		if ( sfx_init(sfx, str, mm) < 0 ) {
			sfx_free(sfx);
			sfx = NULL;
		}
	}
	return sfx;
}


void sfx_free(struct sfxtree *sfx)
{
	sfx_clear(sfx);
	free(sfx);
}




/* Safe Bit Set operations */

struct safebitset *sbs_new(struct memmgr *mm, uint nbits)
{
	struct safebitset *set;
	unsigned len = BITSET_LEN(nbits);

	abort_unless(mm != NULL);

	if ( (set = mem_get(mm, sizeof(struct safebitset))) == NULL )
		return NULL;
	set->mm = mm;
	set->nbits = nbits;
	set->len = len;
	if ( len == 0 )
		len = 1;
	if ( (set->set = mem_get(mm, sizeof(bitset_t) * len)) == NULL ) {
		mem_free(mm, set);
		return NULL;
	}
	sbs_zero(set);

	return set;
}


void sbs_free(struct safebitset *set)
{
	struct memmgr *mm;

	abort_unless(set);
	abort_unless(set->set);
	abort_unless(set->mm);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	mm = set->mm;
	mem_free(mm, set->set);
	mem_free(mm, set);
}


void sbs_zero(struct safebitset *set)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	bset_zero(set->set, set->nbits);
}


void sbs_fill(struct safebitset *set)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	bset_fill(set->set, set->nbits);
}


uint sbs_copy(struct safebitset *dst, struct safebitset *src)
{
	uint nbits;

	abort_unless(dst);
	abort_unless(dst->set);
	abort_unless(BITSET_LEN(dst->nbits) == dst->len);
	abort_unless(src);
	abort_unless(src->set);
	abort_unless(BITSET_LEN(src->nbits) == src->len);

	nbits = dst->nbits;
	if ( nbits > src->nbits )
		nbits = src->nbits;
	bset_copy(dst->set, src->set, nbits);
	return nbits;
}


int sbs_test(struct safebitset *set, uint index)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	if ( index > set->nbits )
		err("sbs_test: index out of bounds (%u > %u)\n", index,
		    set->nbits);
	return bset_test(set->set, index);
}


void sbs_set(struct safebitset *set, uint index)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	if ( index > set->nbits )
		err("sbs_set: index out of bounds (%u > %u)\n", index, 
		    set->nbits);
	bset_set(set->set, index);
}


void sbs_clr(struct safebitset *set, uint index)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	if ( index > set->nbits )
		err("sbs_clr: index out of bounds (%u > %u)\n", index, 
		    set->nbits);
	bset_clr(set->set, index);
}


void sbs_flip(struct safebitset *set, uint index)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);

	if ( index > set->nbits )
		err("sbs_clr: index out of bounds (%u > %u)\n", index,
		    set->nbits);
	bset_flip(set->set, index);
}


void sbs_set_to(struct safebitset *set, uint index, int val)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);
	if ( index > set->nbits )
		err("sbs_set_to: index out of bounds (%u > %u)\n",
		    index, set->nbits);
	bset_set_to(set->set, index, val);
}


