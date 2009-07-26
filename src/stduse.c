/*
 * stduse.c -- application level nicities built on the rest of the catlib
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007, 2008  See accompanying license
 * 
 */

#include <cat/cat.h>
#include <stdarg.h>

#if CAT_USE_STDLIB
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdio.h>
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */
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


char * estrdup(const char *s)
{
	char *new;
	abort_unless(s);
	if ( !(new = strdup(s)) )
		errsys("estrdup: ");
	return new;
}


union raw_u {
	struct raw    raw;
	cat_align_t   align;
};


struct raw *erawdup(struct raw const * const r)
{
	size_t s;
	struct raw *rnew;
	if ( !r || !r->data || !r->len )
		err("erawdup: invalid raw provided");
	s = r->len + sizeof(union raw_u);
	if ( s < sizeof(union raw_u) )
		err("erawdup: integer overflow");
	rnew = emalloc(s);
	rnew->len = r->len;
	rnew->data = (byte_t *)((union raw_u *)rnew + 1);
	memcpy(rnew->data, r->data, r->len);
	return rnew;
}


/* List operations */


struct list *clist_newlist()
{
	struct list *list = clist_new(size_t);
	clist_data(list, size_t) = 0;
	return list;
}


void clist_freelist(struct list *list)
{
	clist_clearlist(list);
	free(list);
}


void clist_clearlist(struct list *list)
{
	while (!l_isempty(list))
		clist_delete_head(list);
}


int clist_isempty(struct list *list)
{
	return clist_data(list, size_t) == 0;
}


struct list *clist_new_sz(size_t len)
{
	struct list *list = emalloc(len + sizeof(union clist_node_u));
	l_init(list);
	return list;
}


struct list *clist_insert(struct list *list, struct list *prev, 
				struct list *node)
{
	l_ins(prev, node);
	clist_data(list, size_t) += 1;
	return node;
}


struct list *clist_insert_head(struct list *list, struct list *node)
{
	return clist_insert(list, l_head(list), node);
}


struct list *clist_insert_tail(struct list *list, struct list *node)
{
	return clist_insert(list, l_tail(list), node);
}


void clist_delete(struct list *list, struct list *node)
{
	l_rem(node);
	clist_data(list, size_t) -= 1;
	free(node);
}


size_t clist_length(struct list *list)
{
	return clist_data(list, size_t);
}


void clist_delete_head(struct list *list)
{
	if (!l_isempty(list))
		clist_delete(list, l_head(list));
}


struct list *clist_head_node(struct list *list)
{
	if (l_isempty(list))
		return NULL;
	return l_head(list);
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


struct dlist * cdl_new(long sec, long nsec, void *data)
{
	struct cdlist *node;

	node = emalloc(sizeof(*node));
	dl_init(&node->entry, sec, nsec);
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




/* Hash table functions */


struct htab *ht_new(size_t size, int type)
{
	struct htab *t;
	struct hashsys sys = {NULL, NULL, NULL};

	abort_unless(size > 0);
	abort_unless(type >= CAT_DT_STR && type <= CAT_DT_RAWCPY);

	switch(type) {
		case CAT_DT_STR:
		sys.hash  = ht_shash;
		sys.cmp   = cmp_str;
		break;

		case CAT_DT_RAW:
		case CAT_DT_RAWCPY:
		sys.hash  = ht_rhash;
		sys.cmp   = cmp_raw;
		break;

		case CAT_DT_PTR:
		case CAT_DT_NUM:
		sys.hash  = ht_phash;
		sys.cmp   = cmp_ptr;
		break;
	}
	/* XXX shouldn't go here, but ok since the hash functions don't */
	/* require any context */
	sys.hctx  = (void *)type;

	if ( (((size_t)~0) - sizeof(*t)) / sizeof(struct list) < size )
		err("size overflows integer size max");
	t = emalloc(sizeof(*t) + size * sizeof(struct list));
	ht_init(t, (struct list *)(t + 1), size, &sys);
	return t;
}


void ht_free(struct htab *t)
{
	unsigned i;
	struct list *l;
	struct hnode *node;
	struct hashsys *hs;

	abort_unless(t != NULL);

	l = t->tab;
	hs = &t->sys;

	for ( i = t->size ; i > 0 ; --i, ++l ) 
		while ( ! l_isempty(l) ) {
			node = (struct hnode *)l->next;
			ht_rem(node);
			ht_snfree(node, hs->hctx);
		}

	free(t);
}


struct hnode *ht_snalloc(void *k, void *d, unsigned h, void *c)
{
	struct hnode *node = NULL;
	int type;
	unsigned l, l2;
	void *nk = NULL;
	struct raw *r1, *r2;
	void free(void *);

	type = (int)c;
	switch(type) {

	case CAT_DT_STR:
		abort_unless(k != NULL);

		l = strlen(k);
		abort_unless(l + 1 + sizeof(struct hnode) >= l);
		l += 1;
		node = emalloc(sizeof(struct hnode) + l);
		nk = (node + 1);
		memmove(nk, k, l);
		break;

	case CAT_DT_RAW:
		abort_unless(k != NULL);

		r1 = k;
		abort_unless(r1->len != 0);
		abort_unless(r1->data != NULL);
		abort_unless(r1->len + sizeof(struct hnode)+
			     sizeof(struct raw) >= r1->len);
		node = emalloc(sizeof(struct hnode) + sizeof(struct raw) + 
			       r1->len);
		nk = r2 = (struct raw *)(node + 1);
		r2->len  = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, r1->len);
		break;

	case CAT_DT_RAWCPY:
		abort_unless(k != NULL);
		abort_unless(d != NULL);

		r1 = k;
		r2 = d;

		abort_unless(r1->len != 0);
		abort_unless(r1->data != NULL);
		abort_unless(r2->len != 0);
		abort_unless(r2->data != NULL);

		l = r1->len;
		if (r1->len % sizeof(cat_align_t))
			l += sizeof(cat_align_t) - 
				r1->len % sizeof(cat_align_t);
		l2 = r2->len;
		if (r2->len % sizeof(cat_align_t))
			l2 += sizeof(cat_align_t) - 
				r2->len % sizeof(cat_align_t);

		abort_unless(l >= r1->len);
		abort_unless(l2 >= r2->len);
		abort_unless(l + l2 >= l);
		abort_unless(l + l2 + sizeof(struct hnode) +
			     sizeof(struct raw) * 2 >= l + l2);
				
		node = emalloc(sizeof(struct hnode) + sizeof(struct raw) * 2 + 
			       l + l2);
		nk = r2 = (struct raw *)(node + 1);
		r2->len  = r1->len;
		r2->data = (char *)(r2 + 1);
		memcpy(r2->data, r1->data, r1->len);
		r1 = d;
		d = r2 = (struct raw *)(r2->data + l);
		r2->len = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, r1->len);
		break;

	case CAT_DT_NUM:
	case CAT_DT_PTR:
		node = emalloc(sizeof(struct hnode));
		nk = k;
		break;

	default:  /* should never happen */
		err("ht_snalloc: unknown type %d", type);
	}

	ht_ninit(node, nk, d, h);
	return node;
}


void ht_snfree(struct hnode *node, void *ctx)
{
	free(node);
}


void * ht_get(struct htab *t, void *key)
{
	struct list *l, *list;
	unsigned h;
	struct hashsys *hs;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	hs = &t->sys;
	h = (*hs->hash)(key, hs->hctx);
	if ( t->po2mask )
		list = t->tab + (h & t->po2mask);
	else
		list = t->tab + (h % t->size);

	for ( l = list->next ; l != list ; l = l->next )
		if ( ! (*hs->cmp)(((struct hnode *)l)->key, key) )
			return ((struct hnode *)l)->data;

	return NULL;
}


void ht_put(struct htab *t, void *key, void *data)
{
	struct hnode *prev, *node;
	struct hashsys *hs;
	unsigned h;

	abort_unless(t != NULL);
	abort_unless(key != NULL);
	hs = &t->sys;
	h = (*hs->hash)(key, hs->hctx);
	node = ht_snalloc(key, data, h, hs->hctx);
	abort_unless(node != NULL);
	prev = ht_ins(t, node);
	if ( prev )
		ht_snfree(prev, hs->hctx);
}


void ht_clr(struct htab *t, void *key)
{
	struct hashsys *hs;
	struct hnode *node;

	abort_unless(t != NULL);
	abort_unless(key != NULL);
	hs = &t->sys;
	node = ht_lkup(t, key, NULL);
	if ( node ) {
		ht_rem(node);
		ht_snfree(node, hs->hctx);
	}
}


struct hnode * ht_nnew(struct hashsys *hs, void *key, void *data,
					unsigned hash)
{
	abort_unless(hs != NULL);
	abort_unless(key != NULL);
	return ht_snalloc(key, data, hash, hs->hctx);
}


void ht_nfree(struct hashsys *hs, struct hnode *node)
{
	abort_unless(hs != NULL);
	if ( ! node )
		return;
	ht_snfree(node, hs->hctx);
}




/* AVL Trees */


struct avl *avl_new(int type)
{
	struct xavl *xa;
	cmp_f cmp;

	abort_unless(type >= CAT_DT_STR && type <= CAT_DT_RAWCPY);
	switch (type) {
	case CAT_DT_STR:
		cmp = cmp_str;
		break;

	case CAT_DT_RAW:
	case CAT_DT_RAWCPY:
		cmp = cmp_raw;
		break;

	case CAT_DT_NUM:
	case CAT_DT_PTR:
		cmp = cmp_ptr;
		break;

	default:
		return NULL;
	}

	xa = emalloc(sizeof(*xa));
	xa->ctx = (void *)type;
	avl_init(&xa->avl, cmp);
	return &xa->avl;
}


void avl_free(struct avl *avl)
{
	struct anode *trav, *par, *root;
	struct xavl *xa = (struct xavl *)avl;

	abort_unless(avl);
	root = &avl->root;
	if ( (trav = root->p[CA_P]) ) {
		while ( trav != root ) {
			if ( trav->p[CA_L] ) 
				trav = trav->p[CA_L];
			else if ( trav->p[CA_R] )
				trav = trav->p[CA_R];
			else {
				par = trav->p[CA_P];
				par->p[trav->pdir] = NULL;
				trav->tree = NULL;
				avl_snfree(trav, xa->ctx);
				trav = par;
			}
		}
	}
	free(avl);
}


struct anode *avl_snalloc(void *k, void *d, void *c)
{
	struct anode *node = NULL;
	void *nk = NULL;
	size_t l, l2;
	int type = (int)c;
	struct raw *r1, *r2;

	switch (type) {
	case CAT_DT_STR:
		abort_unless(k);
		l = strlen(k);
		abort_unless(l + 1 + sizeof(struct anode) >= l);
		l += 1;
		node = emalloc(sizeof(struct anode) + l);
		nk = (node + 1);
		memmove(nk, k, l);
		break;

	case CAT_DT_RAW:
		abort_unless(k);
		r1 = k;
		abort_unless(r1->len > 0);
		abort_unless(r1->data != NULL);
		abort_unless(r1->len + sizeof(struct raw) + 
			     sizeof(struct anode) >= r1->len);
		node = emalloc(sizeof(struct anode) + sizeof(struct raw) + 
			       r1->len);
		r2 = (struct raw *)(node + 1);
		r2->data = nk = r2 + 1;
		r2->len = r1->len;
		memmove(nk, r1->data, r2->len);
		break;

	case CAT_DT_RAWCPY:
		abort_unless(k);
		abort_unless(d);
		r1 = k;  
		r2 = d;
		abort_unless(r1->len);
		abort_unless(r1->data);
		abort_unless(r2->len);
		abort_unless(r2->data);

		l = r1->len;   
		if (r1->len % sizeof(cat_align_t))
			l += sizeof(cat_align_t) - 
				r1->len % sizeof(cat_align_t);
		l2 = r2->len;                                     
		if (r2->len % sizeof(cat_align_t))
			l2 += sizeof(cat_align_t) - 
				r2->len % sizeof(cat_align_t);

		abort_unless(l >= r1->len);
		abort_unless(l2 >= r2->len);
		abort_unless(l + l2 >= l);
		abort_unless(l + l2 + sizeof(struct anode) + 
			     sizeof(struct raw) * 2 >= l + l2);
		node = emalloc(sizeof(struct anode) + sizeof(struct raw)*2 + 
			       l + l2);    
		nk = r2 = (struct raw *)(node + 1);
		r2->len  = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, r1->len);
		r1 = d;
		d = r2 = (struct raw *)(r2->data + l);
		r2->len = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, r1->len);
		break;


	case CAT_DT_PTR:
	case CAT_DT_NUM:
		nk = k;
		node = emalloc(sizeof(struct anode));
		break;

	default:
		err("avl_snalloc:  corrupt sys type (%d)\n", type);
	}

	avl_ninit(node, nk, d);
	return node;
}


void avl_snfree(struct anode *node, void *ctx)
{
	free(node);
}


void * avl_get(struct avl *t, void *key)
{
	struct anode *p;

	abort_unless(t);
	p = avl_lkup(t, key, NULL);
	if ( p )
		return p->data;
	else
		return NULL;
}


void avl_put(struct avl *t, void *key, void *data)
{
	struct anode *prev, *node;
	struct xavl *xa = (struct xavl *)t;

	abort_unless(t);
	node = avl_snalloc(key, data, xa->ctx);
	prev = avl_ins(t, node, NULL, CA_N);
	if ( prev )
		avl_snfree(prev, xa->ctx);
}


void avl_clr(struct avl *t, void *key)
{
	struct anode *p;
	int dir;
	struct xavl *xa = (struct xavl *)t;

	abort_unless(t);
	p = avl_lkup(t, key, &dir);
	if ( dir == CA_N ) {
		avl_rem(p);
		avl_snfree(p, xa->ctx);
	}
}


struct anode * avl_nnew(struct avl *t, void *key, void *data)
{
	struct xavl *xa = (struct xavl *)t;
	abort_unless(t);
	return avl_snalloc(key, data, xa->ctx);
}


void avl_nfree(struct avl *t, struct anode *node)
{
	struct xavl *xa = (struct xavl *)t;
	abort_unless(t);
	if ( ! node )
		return;
	avl_snfree(node, xa->ctx);
}




/* Red-Black Trees */


struct rbtree *rb_new(int type)
{
	struct xrbtree *xrbt;
	cmp_f cmp;

	abort_unless(type >= CAT_DT_STR && type <= CAT_DT_RAWCPY);
	switch (type) {

	case CAT_DT_STR:
		cmp = cmp_str;
		break;

	case CAT_DT_RAW:
	case CAT_DT_RAWCPY:
		cmp = cmp_raw;
		break;

	case CAT_DT_NUM:
	case CAT_DT_PTR:
		cmp = cmp_ptr;
		break;

	default:
		return NULL;

	}
	xrbt = emalloc(sizeof(struct xrbtree));
	xrbt->ctx = (void *)type;
	rb_init(&xrbt->rbt, cmp);
	return &xrbt->rbt;
}


void rb_free(struct rbtree *rbt)
{
	struct rbnode *trav, *par, *root;
	struct xrbtree *xrbt = (struct xrbtree *)rbt;

	abort_unless(rbt);
	root = &rbt->root;
	if ( (trav = root->p[CRB_P]) ) {
		while ( trav != root ) {
			if ( trav->p[CRB_L] ) 
				trav = trav->p[CRB_L];
			else if ( trav->p[CRB_R] )
				trav = trav->p[CRB_R];
			else {
				par = trav->p[CRB_P];
				par->p[(int)trav->pdir] = NULL;
				trav->tree = NULL;
				rb_snfree(trav, xrbt->ctx);
				trav = par;
			}
		}
	}
	free(rbt);
}


struct rbnode *rb_snalloc(void *k, void *d, void *c)
{
	struct rbnode *node = NULL;
	void *nk = NULL;
	size_t l, l2;
	int type = (int)c;
	struct raw *r1, *r2;

	switch (type) {
	case CAT_DT_STR:
		abort_unless(k);
		l = strlen(k);
		abort_unless(l + 1 + sizeof(struct rbnode) >= l);
		l += 1;
		node = emalloc(sizeof(struct rbnode) + l);
		nk = (node + 1);
		memmove(nk, k, l);
		break;

	case CAT_DT_RAW:
		abort_unless(k);
		r1 = k;
		abort_unless(r1->len > 0);
		abort_unless(r1->data != NULL);
		abort_unless(r1->len + sizeof(struct rbnode) +
			     sizeof(struct raw) >= r1->len);
		node = emalloc(sizeof(struct rbnode) + sizeof(struct raw) + 
			       r1->len);
		r2 = (struct raw *)(node + 1);
		r2->data = nk = r2 + 1;
		r2->len = r1->len;
		memmove(nk, r1->data, r2->len);
		break;

	case CAT_DT_RAWCPY:                      
		abort_unless(k);
		abort_unless(d);
		r1 = k;
		r2 = d;
		abort_unless(r1->len);
		abort_unless(r1->data);
		abort_unless(r2->len); 
		abort_unless(r2->data);

		l = r1->len;
		if (r1->len % sizeof(cat_align_t))
			l += sizeof(cat_align_t) - 
				r1->len % sizeof(cat_align_t);   
		l2 = r2->len;                                              
		if (r2->len % sizeof(cat_align_t))
			l2 += sizeof(cat_align_t) - 
				r2->len % sizeof(cat_align_t);  

		abort_unless(l >= r1->len);
		abort_unless(l2 >= r2->len);
		abort_unless(l + l2 >= l);
		abort_unless(l + l2 + sizeof(struct rbnode) + 
			     sizeof(struct raw)*2 >= l + l2);
		node = emalloc(sizeof(struct rbnode)+sizeof(struct raw)*2 + l +
			       l2);
		nk = r2 = (struct raw *)(node + 1);
		r2->len  = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, r1->len);
		r1 = d;
		d = r2 = (struct raw *)(r2->data + l);
		r2->len = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, r1->len);
		break;

	case CAT_DT_PTR:
	case CAT_DT_NUM:
		nk = k;
		node = emalloc(sizeof(struct rbnode));
		break;

	default:
		err("rb_snalloc:  corrupt sys type %d", type);
	}

	rb_ninit(node, nk, d);
	return node;
}


void rb_snfree(struct rbnode *node, void *ctx)
{
	free(node);
}


void * rb_get(struct rbtree *t, void *key)
{
	struct rbnode *p;
	int dir;

	abort_unless(t);
	p = rb_lkup(t, key, &dir);
	if ( dir == CRB_N )
		return p->data;
	else
		return NULL;
}


void rb_put(struct rbtree *t, void *key, void *data)
{
	struct rbnode *prev, *node;
	struct xrbtree *xrbt = (struct xrbtree *)t;

	abort_unless(t);
	node = rb_snalloc(key, data, xrbt->ctx);
	prev = rb_ins(t, node, NULL, CRB_N);
	if ( prev )
		rb_snfree(prev, xrbt->ctx);
}


void rb_clr(struct rbtree *t, void *key)
{
	struct rbnode *p;
	int dir;
	struct xrbtree *xrbt = (struct xrbtree *)t;

	abort_unless(t);
	p = rb_lkup(t, key, &dir);
	if ( dir == CRB_N ) {
		rb_rem(p);
		rb_snfree(p, xrbt->ctx);
	} 
}


struct rbnode * rb_nnew(struct rbtree *rbt, void *key, void *data)
{
	struct xrbtree *xrbt = (struct xrbtree *)rbt;
	abort_unless(rbt);
	return rb_snalloc(key, data, xrbt->ctx);
}


void rb_nfree(struct rbtree *rbt, struct rbnode *node)
{
	struct xrbtree *xrbt = (struct xrbtree *)rbt;
	abort_unless(rbt);
	if ( ! node )
		return;
	rb_snfree(node, xrbt->ctx);
}




/* Splay Trees */


struct splay *st_new(int type)
{
	struct xsplay *xs;
	cmp_f cmp;

	abort_unless(type >= CAT_DT_STR && type <= CAT_DT_NUM);
	switch (type) {
	case CAT_DT_STR:
		cmp = cmp_str;
		break;
	
	case CAT_DT_RAW:
	case CAT_DT_RAWCPY:
		cmp = cmp_raw;
		break;
	
	case CAT_DT_NUM:
	case CAT_DT_PTR:
		cmp = cmp_ptr;
		break;
	default:
		return NULL;
	}

	xs = emalloc(sizeof(*xs));
	xs->ctx = (void *)type;
	st_init(&xs->tree, cmp);
	return &xs->tree;
}

void st_free(struct splay *t)
{
	struct stnode *trav, *par, *root;
	struct xsplay *xs = (struct xsplay *)t;

	abort_unless(t);
	root = &t->root;
	if ( (trav = root->st_par) ) {
		while ( trav != root ) {
			if ( trav->st_left )
				trav = trav->st_left;
			else if ( trav->st_right )
				trav = trav->st_right;
			else {
				par = trav->st_par;
				par->p[(int)trav->pdir] = NULL;
				trav->tree = NULL;
				st_snfree(trav, xs->ctx);
				trav = par;
			}
		}
	}
	free(t);
}


struct stnode *st_snalloc(void *k, void *d, void *c)
{
	struct stnode *node = NULL;
	void *nk = NULL;
	size_t l, l2, tlen;
	int type = (int)c;
	struct raw *r1, *r2;

	switch (type) {
	case CAT_DT_STR:
		abort_unless(k);
		l = strlen(k);
		tlen = l + 1 + sizeof(struct stnode);
		abort_unless(tlen >= l);
		l += 1;
		node = emalloc(tlen);
		nk = (node + 1);
		memmove(nk, k, l);
		break;

	case CAT_DT_RAW:
		abort_unless(k);
		r1 = k;
		abort_unless(r1->len > 0);
		tlen = r1->len + sizeof(struct raw) + sizeof(struct stnode);
		abort_unless(tlen >= r1->len);
		node = emalloc(tlen);
		r2 = (struct raw *)(node + 1);
		r2->data = nk = r2 + 1;
		r2->len = r1->len;
		memmove(nk, r1->data, r2->len);
		break;

	case CAT_DT_RAWCPY:
		abort_unless(k);
		abort_unless(d);
		r1 = k;
		r2 = d;
		abort_unless(r1->len > 0);
		abort_unless(r1->data != NULL);
		abort_unless(r2->len > 0);
		abort_unless(r2->data != NULL);

		l = r1->len;
		if ( r1->len % sizeof(cat_align_t) )
			l += sizeof(cat_align_t) - 
				r1->len % sizeof(cat_align_t);
		l2 = r2->len;
		if ( r2->len % sizeof(cat_align_t) )
			l += sizeof(cat_align_t) - 
				r2->len % sizeof(cat_align_t);

		abort_unless(l >= r1->len);
		abort_unless(l2 >= r2->len);
		tlen = l + l2;
		abort_unless(tlen >= l);
		tlen += sizeof(struct stnode) + sizeof(struct raw) * 2;
		abort_unless(tlen >= l + l2);

		node = emalloc(tlen);
		nk = r2 = (struct raw *)(node + 1);
		r2->len = l;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, l);
		r1 = d;
		d = r2 = (struct raw *)(r2->data + l);
		r2->len = r1->len;
		r2->data = (char *)(r2 + 1);
		memmove(r2->data, r1->data, l2);
		break;

	case CAT_DT_NUM:
	case CAT_DT_PTR:
		nk = k;
		node = emalloc(sizeof(struct stnode));
		break;

	default:
		err("st_snalloc:  corrupt sys type (%d)\n", type);
	}

	st_ninit(node, nk, d);
	return node;
}


void st_snfree(struct stnode *node, void *ctx)
{
	free(node);
}


void * st_get(struct splay *t, void *key)
{
	struct stnode *p;

	abort_unless(t);
	p = st_lkup(t, key);
	if ( p )
		return p->data;
	else
		return NULL;
}


void st_put(struct splay *t, void *key, void *data)
{
	struct stnode *prev, *node;
	struct xsplay *xs = (struct xsplay *)t;

	abort_unless(t);
	node = st_snalloc(key, data, xs->ctx);
	prev = st_ins(t, node);
	if ( prev )
		st_snfree(prev, xs->ctx);
}


void st_clr(struct splay *t, void *key)
{
	struct stnode *prev;
	struct xsplay *xs = (struct xsplay *)t;
	abort_unless(t);
	prev = st_lkup(t, key);
	if ( prev != NULL ) {
		st_rem(prev);
		st_snfree(prev, xs->ctx);
	}
}


struct stnode * st_nnew(struct splay *t, void *key, void *data)
{
	struct xsplay *xs = (struct xsplay *)t;
	abort_unless(t);
	return st_snalloc(key, data, xs->ctx);
}


void st_nfree(struct splay *t, struct stnode *node)
{
	struct xsplay *xs = (struct xsplay *)t;
	abort_unless(t);
	if ( ! node )
		return;
	st_snfree(node, xs->ctx);
}




/* Heap operations */


struct heap * hp_new(int size, cmp_f cmp)
{
	struct heap *hp;
	void **elem = NULL;

	abort_unless(size >= 0 && cmp);

	hp = emalloc(sizeof(struct heap));
	if ( size )
		elem = emalloc(size * sizeof(void*));
	hp_init(hp, elem, size, 0, cmp, &estdmm);

	return hp;
}


void hp_free(struct heap *hp)
{
	free(hp->elem);
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




/* Callback functions */


struct callback * cb_new(struct list *e, callback_f f, void *ctx)
{
	struct callback *cb;

	abort_unless(e);

	cb = emalloc(sizeof(*cb));
	cb_init(cb, f, ctx);
	cb_reg(e, cb);

	return cb;
}


void cb_clr(struct callback *cb)
{
	abort_unless(cb);
	cb_unreg(cb);
	free(cb);
}




/* Matching functions */


struct kmppat * kmp_pnew(struct raw *pat)
{
	struct kmppat *kmp;

	abort_unless(pat && pat->data && pat->len);
	kmp = emalloc(sizeof(*kmp) + pat->len * sizeof(ulong));
	kmp_pinit(kmp, pat, (ulong *)(kmp + 1));
	return kmp;
}


struct bmpat * bm_pnew(struct raw *pat)
{
	struct bmpat *bmp;
	ulong *scratch;

	abort_unless(pat && pat->data && pat->len);
	bmp = emalloc(sizeof(*bmp) + pat->len * sizeof(ulong));
	scratch = emalloc(pat->len * sizeof(ulong));
	bm_pinit(bmp, pat, (ulong *)(bmp + 1), scratch);
	free(scratch);
	return bmp;
}


struct sfxtree *sfx_new(struct raw *str)
{
	struct sfxtree *sfx;
	sfx = emalloc(sizeof(*sfx));
	sfx_init(sfx, str, &estdmm);
	return sfx;
}


void sfx_free(struct sfxtree *sfx)
{
	sfx_clear(sfx);
	free(sfx);
}




/* Safe Bit Set operations */

struct safebitset *sbs_new(unsigned nbits)
{
	struct safebitset *set;
	unsigned len = BITSET_LEN(nbits);

	set = emalloc(sizeof(struct safebitset));
	set->nbits = nbits;
	set->len = len;
	set->set = emalloc(sizeof(bitset_t) * (len ? len : 1));
	sbs_zero(set);
	return set;
}


void sbs_free(struct safebitset *set)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);
	free(set->set);
	free(set);
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
		err("sbs_test: index out of bounds (%u > %u)\n",
				index, set->nbits);
	return bset_test(set->set, index);
}


void sbs_set(struct safebitset *set, uint index)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);
	if ( index > set->nbits )
		err("sbs_set: index out of bounds (%u > %u)\n",
				index, set->nbits);
	bset_set(set->set, index);
}


void sbs_clr(struct safebitset *set, uint index)
{
	abort_unless(set);
	abort_unless(set->set);
	abort_unless(BITSET_LEN(set->nbits) == set->len);
	if ( index > set->nbits )
		err("sbs_clr: index out of bounds (%u > %u)\n",
				index, set->nbits );
	bset_clr(set->set, index);
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



#if CAT_HAS_POSIX

/* Get Posix system time */


struct cat_time *tm_uget(struct cat_time *t)
{
	struct timeval cur;

	abort_unless(t);

	gettimeofday(&cur, NULL);
	t->sec  = cur.tv_sec;
	t->nsec = cur.tv_usec * 1000;

	return t;
}


#endif /* CAT_HAS_POSIX */
