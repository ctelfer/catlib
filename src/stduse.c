/*
 * stduse.c -- application level nicities built on the rest of the catlib
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2007-2012  See accompanying license
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


struct clist *clist_new_list(struct memmgr *mm, size_t dlen)
{
	struct clist *list;
	if ( (list = mem_get(mm, sizeof(struct clist))) == NULL )
		return NULL;
	clist_init_list(list, mm, dlen);
	return list;
}


void clist_free_list(struct clist *list)
{
	struct memmgr *mm;
	abort_unless(list);
	mm = list->cl_mm;
	clist_clear_list(list);
	mem_free(mm, list);
}

void clist_init_list(struct clist *list, struct memmgr *mm, size_t dlen)
{
	size_t nsize;

	abort_unless(mm != NULL);
	abort_unless(dlen >= 1);

	nsize = attrib_csize(struct clist_node, cln_data_u, dlen);
	abort_unless(nsize >= sizeof(struct clist_node));

	l_init(&list->cl_base.cln_entry);
	list->cl_base.cln_list = NULL;
	list->cl_base.cln_mm = NULL;
	list->cl_mm = mm;
	list->cl_fill = 0;
	list->cl_node_size = nsize;
	list->cl_data_size = dlen;
}


void clist_clear_list(struct clist *list)
{
	abort_unless(list != NULL);
	while ( !clist_isempty(list) )
		clist_delete(cl_first(list));
}

int clist_isempty(struct clist *list)
{
	abort_unless(list != NULL);
	return l_isempty(&list->cl_base.cln_entry);
}


size_t clist_fill(struct clist *list)
{
	abort_unless(list != NULL);
	return list->cl_fill;
}


struct clist_node *clist_new_node(struct clist *list, void *val)
{
	struct clist_node *node;
	abort_unless(list != NULL && list->cl_mm != NULL);

	if ( (node = mem_get(list->cl_mm, list->cl_node_size)) == NULL )
		return NULL;

	l_init(&node->cln_entry);
	node->cln_list = NULL;
	node->cln_mm = list->cl_mm;

	if ( val != NULL ) {
		memcpy(node->cln_attr_ptr, val, list->cl_data_size);
	} else { 
		memset(node->cln_attr_ptr, 0, list->cl_data_size);
	}

	return node;
}


int clist_is_head(struct clist_node *node)
{
	abort_unless(node);
	return node->cln_mm != NULL;
}


int clist_insert(struct clist *list, struct clist_node *prev,
		 struct clist_node *node)
{
	abort_unless(list != NULL);

	if ( node == NULL || node->cln_list != NULL )
		return 0;

	if ( prev == NULL ) {
		abort_unless(prev->cln_list == list);
		l_ins(&cl_head(list)->cln_entry, &node->cln_entry);
	} else {
		l_ins(&prev->cln_entry, &node->cln_entry);
	}

	list->cl_fill += 1;

	return 1;
}


int clist_remove(struct clist_node *node)
{
	abort_unless(node != NULL);
	if ( node->cln_list != NULL ) {
		l_rem(&node->cln_entry);
		node->cln_list->cl_fill -= 1;
		node->cln_list = NULL;
		return 1;
	} else {
		return 0;
	}
}


static void clist_delete_removed(struct clist_node *node)
{
	struct memmgr *mm;
	mm = node->cln_mm;
	mem_free(mm, node);
}


void clist_delete(struct clist_node *node)
{
	abort_unless(node != NULL && node->cln_mm != NULL);
	if ( node->cln_list != NULL ) {
		l_rem(&node->cln_entry);
		node->cln_list->cl_fill -= 1;
		node->cln_list = NULL;
	}
	clist_delete_removed(node);
}


int clist_enqueue(struct clist *list, void *val)
{
	struct clist_node *node;
	abort_unless(list != NULL);

	if ( (node = clist_new_node(list, val)) == NULL )
		return 0;

	l_ins(&cl_last(list)->cln_entry, &node->cln_entry);
	list->cl_fill += 1;
	node->cln_list = list;
	return 1;
}


int clist_dequeue(struct clist *list, void *val)
{
	struct clist_node *node;
	abort_unless(list != NULL);

	if ( l_isempty(&cl_head(list)->cln_entry) )
		return 0;
	node = cl_first(list);
	clist_remove(node);

	if ( val != NULL )
		memcpy(val, node->cln_attr_ptr, list->cl_data_size);

	clist_delete_removed(node);
	return 1;
}


int clist_push(struct clist *list, void *val)
{
	struct clist_node *node;
	abort_unless(list != NULL);

	if ( (node = clist_new_node(list, val)) == NULL )
		return 0;

	l_ins(&cl_head(list)->cln_entry, &node->cln_entry);
	list->cl_fill += 1;
	node->cln_list = list;
	return 1;
}


int clist_pop(struct clist *list, void *val)
{
	return clist_dequeue(list, val);
}


int clist_top(struct clist *list, void *val)
{
	struct clist_node *node;
	if ( l_isempty(&cl_head(list)->cln_entry) )
		return 0;
	node = cl_first(list);
	if ( val != NULL )
		memcpy(val, node->cln_attr_ptr, list->cl_data_size);
	return 1;
}


void clist_apply(struct clist *list, apply_f f, void *arg)
{
	struct clist_node *cur, *next;

	abort_unless(list != NULL);
	abort_unless(f != NULL);

	for ( cur = cl_head(list) ; cur != cl_end(list) ; cur = next ) {
		next = cln_next(cur);
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


/* Generic data types for container adaptors */

struct dictiface;
typedef int (*std_keycopy_f)(byte_t *nodep, const void *key, 
			     struct dictiface *di, void **keyp);
typedef void (*std_keyfree_f)(byte_t *nodep, struct dictiface *di);

struct dictiface {
	struct memmgr *		mm;
	std_keycopy_f		keycpy;
	std_keyfree_f		keyfree;
	int			rawwrap;
	size_t			nodesize;
	size_t			keysize;
	size_t			keyoff;
	size_t			datasize;	
	size_t			dataoff;
};


static int std_str_kcopy(byte_t *p, const void *k, struct dictiface *di, 
			 void ** kp)
{
	size_t slen;
	void *newk;
	char **cpp;

	slen = strlen(k) + 1;
	abort_unless(slen > 0);
	newk = mem_get(di->mm, slen);
	if ( newk == NULL )
		return -1;
	memcpy(newk, k, slen);
	cpp = (char **)(p + di->keyoff);
	*cpp = newk;
	*kp = newk;
	return 0;
}


static void std_str_kfree(byte_t *p, struct dictiface *di)
{
	char **cpp = (char **)(p + di->keyoff);
	mem_free(di->mm, *cpp);
}


static int std_ptr_kcopy(byte_t *p, const void *k, struct dictiface *di, 
			 void ** kp)
{
	*kp = (void *)k;
	return 0;
}


static int std_bin_kcopy(byte_t *p, const void *k, struct dictiface *di,
			 void ** kp)
{
	/* The wrapped struct raw is used to pass the length to cmp_bin() */
	struct raw *r = (struct raw *)(p + di->keyoff);
	struct raw const *rik = k;
	void *k2 = (p + di->keyoff + CAT_ALIGN_SIZE(sizeof(struct raw)));
	r->data = k2;
	r->len = di->keysize;
	abort_unless(r->len == rik->len);
	memcpy(k2, rik->data, di->keysize);
	*kp = r;
	return 0;
}


static int std_raw_kcopy(byte_t *p, const void *k, struct dictiface *di,
			 void ** kp)
{
	struct raw const *r1 = k;
	struct raw *r2 = (struct raw *)(p + di->keyoff);
	void *k2;

	abort_unless(r1 != NULL);
	k2 = mem_get(di->mm, r1->len);
	if ( k2 == NULL )
		return -1;
	memcpy(k2, r1->data, r1->len);
	r2->data = k2;
	r2->len = r1->len;
	*kp = r2;
	return 0;
}


static void std_raw_kfree(byte_t *p, struct dictiface *di)
{
	struct raw *r = (struct raw *)(p + di->keyoff);
	mem_free(di->mm, r->data);
}


static void init_dictiface(struct dictiface *di, struct memmgr *mm, size_t nlen, 
			   int ktype, size_t klen, size_t dlen)
{
	abort_unless(nlen > 0);

	di->rawwrap = 0;
	nlen = CAT_ALIGN_SIZE(nlen);
	di->mm = mm;
	switch (ktype) {
	case CAT_KT_STR:
		di->keycpy = &std_str_kcopy;
		di->keyfree = &std_str_kfree;
		di->keysize = klen = CAT_ALIGN_SIZE(sizeof(char *));
		di->keyoff = nlen;
		break;

	case CAT_KT_BIN:
		abort_unless(klen > 0);
		di->keycpy = &std_bin_kcopy;
		di->keyfree = NULL;
		/* NOTE: we use keysize for the binary blob length */
		di->keysize = klen;
		klen = CAT_ALIGN_SIZE(sizeof(struct raw)) + di->keysize;
		di->keyoff = nlen;
		di->rawwrap = 1;
		break;

	case CAT_KT_RAW:
		klen = CAT_ALIGN_SIZE(sizeof(struct raw));
		di->keycpy = &std_raw_kcopy;
		di->keyfree = &std_raw_kfree;
		di->keysize = 0;
		di->keyoff = nlen;
		break;

	case CAT_KT_PTR:
		di->keycpy = &std_ptr_kcopy;
		di->keyfree = NULL;
		di->keysize = 0;
		di->keyoff = 0;
		break;

	case CAT_KT_NUM:
		di->keycpy = &std_bin_kcopy;
		di->rawwrap = 1;
		di->keyfree = NULL;
		di->keysize = sizeof(int);
		klen = CAT_ALIGN_SIZE(sizeof(struct raw)) + sizeof(int);
		di->keyoff = nlen;
		break;
	}

	abort_unless(nlen + CAT_ALIGN_SIZE(klen) > nlen);
	nlen += CAT_ALIGN_SIZE(klen);
	abort_unless(nlen + dlen >= nlen);
	di->nodesize = nlen + dlen;

	di->dataoff = klen + nlen;
}


static void *di_new_node(struct dictiface *di, void const * key, void **nk, 
			 void **data)
{
	byte_t *node, *p;
	struct memmgr *mm;

	abort_unless(key != NULL);
	abort_unless(data != NULL);
	abort_unless(*data != NULL);
	abort_unless(di != NULL);
	mm = di->mm;
	abort_unless(mm != NULL);

	if ( (node = mem_get(mm, di->nodesize)) == NULL )
		return NULL;

	if ( (*di->keycpy)(node, key, di, nk) < 0 ) {
		mem_free(mm, node);
		return NULL;
	}

	if ( di->datasize != 0 ) {
		p = node + di->dataoff;
		memcpy(p, data, di->datasize);
		*data = p;
	}

	return node;
}


static void di_free_node(struct dictiface *di, void *nodep)
{
	byte_t *node = nodep;
	struct memmgr *mm;

	abort_unless(nodep != NULL);
	abort_unless(di != NULL);
	mm = di->mm;
	abort_unless(mm != NULL);

	if ( di->keyfree != NULL )
		(*di->keyfree)(nodep, di);
	mem_free(mm, node);
}


static const void *di_get_key(struct dictiface *di, struct raw *r, const void *k)
{
	if ( !di->rawwrap ) {
		return k;
	} else {
		r->data = (void *)k;
		r->len = di->keysize;
		return r;
	}
}


/* Hash table functions */

struct std_htab {
	struct htab		table;
	struct dictiface	iface;
	struct hnode *		buckets[1];
};


static uint std_ht_bin_hash(const void *k, void *ctx)
{
	size_t *len = ctx;
	struct raw r;
	r.data = (void *)k;
	r.len = *len;
	return ht_rhash(&r, NULL);
}


static uint std_ht_int_hash(const void *kp, void *ctx)
{
	ulong v;
	abort_unless(kp != NULL);
	v = *(int *)kp;
	v ^= (v >> 24) ^ (v << 24) ^ ((v >> 8) & 0xFF00) ^ 
	     ((v << 8) & 0xFF0000);
	v = v + (v << 1);
	return (uint)v;
}


struct htab *ht_new(struct memmgr *mm, size_t size, int ktype, size_t klen, 
		    size_t dlen)
{
	struct std_htab *sh;
	struct dictiface *di;
	size_t n;

	abort_unless(mm != NULL);
	abort_unless(size > 0);
	abort_unless(size < (((size_t)~0 - sizeof(*sh)) / sizeof(struct hnode *)));
	abort_unless(ktype >= CAT_KT_STR && ktype <= CAT_KT_NUM);

	n = offsetof(struct std_htab, buckets) + sizeof(struct hnode *) * size;
	if ( (sh = mem_get(mm, n)) == NULL )
		return NULL;
	di = &sh->iface;

	init_dictiface(di, mm, sizeof(struct hnode), ktype, klen, dlen);
	switch (ktype) {
	case CAT_KT_STR:
		ht_init(&sh->table, sh->buckets, size, cmp_str, ht_shash, NULL);
		break;

	case CAT_KT_BIN:
		ht_init(&sh->table, sh->buckets, size, cmp_raw, std_ht_bin_hash,
			&di->datasize);
		break;

	case CAT_KT_RAW:
		ht_init(&sh->table, sh->buckets, size, cmp_raw, ht_rhash, NULL);
		break;

	case CAT_KT_PTR:
		ht_init(&sh->table, sh->buckets, size, cmp_ptr, ht_phash, NULL);
		break;

	case CAT_KT_NUM:
		ht_init(&sh->table, sh->buckets, size, cmp_raw, std_ht_int_hash,
			NULL);
		break;
	}

	return &sh->table;
}


void ht_free(struct htab *t)
{
	struct std_htab *sh;
	unsigned i;
	struct hnode *node;
	struct memmgr *mm;

	abort_unless(t != NULL);
	sh = container(t, struct std_htab, table);
	mm = sh->iface.mm;

	for ( i = 0; i < t->nbkts ; ++i ) {
		while ( t->bkts[i] ) {
			node = t->bkts[i];
			ht_rem(node);
			mem_free(mm, node);
		}
	}

	mem_free(mm, sh);
}


int ht_get(struct htab *t, const void *key, void *res)
{
	struct std_htab *sh;
	struct hnode *node;
	struct raw r;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	sh = container(t, struct std_htab, table);
	abort_unless(sh->iface.datasize > 0);
	key = di_get_key(&sh->iface, &r, key);

	if ( (node = ht_lkup(t, key, NULL)) != NULL ) {
		if ( res != NULL )
			memcpy(res, node->data, sh->iface.datasize);
		return 1;
	} else {
		return 0;
	}
}


void *ht_get_dptr(struct htab *t, const void *key)
{
	struct std_htab *sh;
	struct hnode *node;
	struct raw r;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	sh = container(t, struct std_htab, table);
	key = di_get_key(&sh->iface, &r, key);

	if ( (node = ht_lkup(t, key, NULL)) != NULL )
		return node->data;
	else
		return NULL;
}


int ht_put(struct htab *t, const void *key, void *data)
{
	struct std_htab *sh;
	struct hnode *node;
	struct raw r;
	unsigned h;
	void *nk;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	sh = container(t, struct std_htab, table);
	key = di_get_key(&sh->iface, &r, key);

	if ( (node = ht_lkup(t, key, &h)) != NULL ) {
		ht_rem(node);
		di_free_node(&sh->iface, node);
	}

	if ( (node = di_new_node(&sh->iface, key, &nk, &data)) != NULL ) {
		ht_ninit(node, nk, data);
		ht_ins(t, node, h);
		return 1;
	} else {
		return 0;
	}
}


int ht_clr(struct htab *t, const void *key)
{
	struct std_htab *sh;
	struct hnode *node;
	struct raw r;

	abort_unless(t != NULL);
	abort_unless(key != NULL);

	sh = container(t, struct std_htab, table);
	key = di_get_key(&sh->iface, &r, key);

	if ( (node = ht_lkup(t, key, NULL)) != NULL ) {
		ht_rem(node);
		di_free_node(&sh->iface, node);
		return 1;
	} else {
		return 0;
	}
}



/* AVL Trees */


struct std_avl {
	struct avl		tree;
	struct dictiface	iface;
};


struct avl *avl_new(struct memmgr *mm, int ktype, size_t klen, size_t dlen)
{
	struct std_avl *sat;
	struct dictiface *di;
	cmp_f cmp = NULL;

	abort_unless(mm != NULL);
	abort_unless(ktype >= CAT_KT_STR && ktype <= CAT_KT_NUM);

	if ( (sat = mem_get(mm, sizeof(*sat))) == NULL )
		return NULL;
	di = &sat->iface;

	init_dictiface(di, mm, sizeof(struct anode), ktype, klen, dlen);
	switch (ktype) {
	case CAT_KT_STR:
		cmp = cmp_str;
		break;

	case CAT_KT_BIN:
	case CAT_KT_RAW:
	case CAT_KT_NUM:
		cmp = cmp_raw;
		break;

	case CAT_KT_PTR:
		cmp = cmp_ptr;
		break;
	}

	avl_init(&sat->tree, cmp);
	return &sat->tree;
}


void avl_free(struct avl *avl)
{
	struct std_avl *sat;
	struct anode *node;
	struct memmgr *mm;

	abort_unless(avl != NULL);

	sat = container(avl, struct std_avl, tree);
	mm = sat->iface.mm;

	while ( (node = avl_getroot(avl)) != NULL ) {
		avl_rem(node);
		mem_free(mm, node);
	}
	mem_free(mm, sat);
}


int avl_get(struct avl *avl, const void *key, void *res)
{
	struct std_avl *sat;
	struct anode *node;
	struct raw r;

	abort_unless(avl != NULL);
	abort_unless(key != NULL);

	sat = container(avl, struct std_avl, tree);
	abort_unless(sat->iface.datasize > 0);
	key = di_get_key(&sat->iface, &r, key);

	if ( (node = avl_lkup(avl, key, NULL)) != NULL ) {
		if ( res != NULL )
			memcpy(res, node->data, sat->iface.datasize);
		return 1;
	} else {
		return 0;
	}
}


void *avl_get_dptr(struct avl *avl, const void *key)
{
	struct std_avl *sat;
	struct anode *node;
	struct raw r;

	abort_unless(avl != NULL);
	abort_unless(key != NULL);

	sat = container(avl, struct std_avl, tree);
	key = di_get_key(&sat->iface, &r, key);

	if ( (node = avl_lkup(avl, key, NULL)) != NULL )
		return node->data;
	else
		return NULL;
}


int avl_put(struct avl *avl, const void *key, void *data)
{
	struct std_avl *sat;
	struct anode *node;
	struct raw r;
	void *nk;

	abort_unless(avl != NULL);
	abort_unless(key != NULL);

	sat = container(avl, struct std_avl, tree);
	key = di_get_key(&sat->iface, &r, key);

	if ( (node = avl_lkup(avl, key, NULL)) != NULL ) {
		avl_rem(node);
		di_free_node(&sat->iface, node);
	}

	if ( (node = di_new_node(&sat->iface, key, &nk, &data)) != NULL ) {
		avl_ninit(node, nk, data);
		avl_ins(avl, node, NULL, CA_N);
		return 1;
	} else {
		return 0;
	}
}


int avl_clr(struct avl *avl, const void *key)
{
	struct std_avl *sat;
	struct anode *node;
	struct raw r;

	abort_unless(avl != NULL);
	abort_unless(key != NULL);

	sat = container(avl, struct std_avl, tree);
	key = di_get_key(&sat->iface, &r, key);

	if ( (node = avl_lkup(avl, key, NULL)) != NULL ) {
		avl_rem(node);
		di_free_node(&sat->iface, node);
		return 1;
	} else {
		return 0;
	}
}


/* Red-Black Trees */

struct std_rbtree {
	struct rbtree		tree;
	struct dictiface	iface;
};


struct rbtree *rb_new(struct memmgr *mm, int ktype, size_t klen, size_t dlen)
{
	struct std_rbtree *srb;
	struct dictiface *di;
	cmp_f cmp = NULL;

	abort_unless(mm != NULL);
	abort_unless(ktype >= CAT_KT_STR && ktype <= CAT_KT_NUM);

	if ( (srb = mem_get(mm, sizeof(*srb))) == NULL )
		return NULL;
	di = &srb->iface;

	init_dictiface(di, mm, sizeof(struct rbnode), ktype, klen, dlen);
	switch (ktype) {
	case CAT_KT_STR:
		cmp = cmp_str;
		break;

	case CAT_KT_BIN:
	case CAT_KT_RAW:
	case CAT_KT_NUM:
		cmp = cmp_raw;
		break;

	case CAT_KT_PTR:
		cmp = cmp_ptr;
		break;
	}

	rb_init(&srb->tree, cmp);
	return &srb->tree;
}


void rb_free(struct rbtree *rbt)
{
	struct std_rbtree *srb;
	struct rbnode *node;
	struct memmgr *mm;

	abort_unless(rbt != NULL);

	srb = container(rbt, struct std_rbtree, tree);
	mm = srb->iface.mm;

	while ( (node = rb_getroot(rbt)) != NULL ) { 
		rb_rem(node);
		mem_free(mm, node);
	}
	mem_free(mm, srb);
}


int rb_get(struct rbtree *rbt, const void *key, void *res)
{
	struct std_rbtree *srb;
	struct rbnode *node;
	struct raw r;

	abort_unless(rbt != NULL);
	abort_unless(key != NULL);

	srb = container(rbt, struct std_rbtree, tree);
	abort_unless(srb->iface.datasize > 0);
	key = di_get_key(&srb->iface, &r, key);

	if ( (node = rb_lkup(rbt, key, NULL)) != NULL ) {
		if ( res != NULL )
			memcpy(res, node->data, srb->iface.datasize);
		return 1;
	} else {
		return 0;
	}
}


void *rb_get_dptr(struct rbtree *rbt, const void *key)
{
	struct std_rbtree *srb;
	struct rbnode *node;
	struct raw r;

	abort_unless(rbt != NULL);
	abort_unless(key != NULL);

	srb = container(rbt, struct std_rbtree, tree);
	key = di_get_key(&srb->iface, &r, key);

	if ( (node = rb_lkup(rbt, key, NULL)) != NULL )
		return node->data;
	else
		return NULL;
}


int rb_put(struct rbtree *rbt, const void *key, void *data)
{
	struct std_rbtree *srb;
	struct rbnode *node;
	struct raw r;
	void *nk;

	abort_unless(rbt != NULL);
	abort_unless(key != NULL);

	srb = container(rbt, struct std_rbtree, tree);
	key = di_get_key(&srb->iface, &r, key);

	if ( (node = rb_lkup(rbt, key, NULL)) != NULL ) {
		rb_rem(node);
		di_free_node(&srb->iface, node);
	}

	if ( (node = di_new_node(&srb->iface, key, &nk, &data)) != NULL ) {
		rb_ninit(node, nk, data);
		rb_ins(rbt, node, NULL, CRB_N);
		return 1;
	} else { 
		return 0; 
	}
}


int rb_clr(struct rbtree *rbt, const void *key)
{
	struct std_rbtree *srb;
	struct rbnode *node;
	struct raw r;

	abort_unless(rbt != NULL);
	abort_unless(key != NULL);

	srb = container(rbt, struct std_rbtree, tree);
	key = di_get_key(&srb->iface, &r, key);

	if ( (node = rb_lkup(rbt, key, NULL)) != NULL ) {
		rb_rem(node);
		di_free_node(&srb->iface, node);
		return 1;
	} else {
		return 0;
	}
}



/* Splay Trees */

struct std_splay {
	struct splay		tree;
	struct dictiface	iface;
};


struct splay *st_new(struct memmgr *mm, int ktype, size_t klen, size_t dlen)
{
	struct std_splay *sst;
	struct dictiface *di;
	cmp_f cmp = NULL;

	abort_unless(mm != NULL);
	abort_unless(ktype >= CAT_KT_STR && ktype <= CAT_KT_NUM);

	if ( (sst = mem_get(mm, sizeof(*sst))) == NULL )
		return NULL;
	di = &sst->iface;

	init_dictiface(di, mm, sizeof(struct stnode), ktype, klen, dlen);
	switch (ktype) {
	case CAT_KT_STR:
		cmp = cmp_str;
		break;
	
	case CAT_KT_BIN:
	case CAT_KT_RAW:
	case CAT_KT_NUM:
		cmp = cmp_raw;
		break;
	
	case CAT_KT_PTR:
		cmp = cmp_ptr;
		break;
	}

	st_init(&sst->tree, cmp);
	return &sst->tree;
}


void st_free(struct splay *st)
{
	struct std_splay *sst;
	struct stnode *node;
	struct memmgr *mm;

	abort_unless(st != NULL);

	sst = container(st, struct std_splay, tree);
	mm = sst->iface.mm;

	while ( (node = st_getroot(st)) != NULL ) {
		st_rem(node);
		mem_free(mm, node);
	}
	mem_free(mm, sst);
}


int st_get(struct splay *st, const void *key, void *res)
{
	struct std_splay *sst;
	struct stnode *node;
	struct raw r;

	abort_unless(st != NULL);
	abort_unless(key != NULL);

	sst = container(st, struct std_splay, tree);
	abort_unless(sst->iface.datasize > 0);
	key = di_get_key(&sst->iface, &r, key);

	if ( (node = st_lkup(st, key)) != NULL ) {
		if ( res != NULL )
			memcpy(res, node->data, sst->iface.datasize);
		return 1;
	} else {
		return 0;
	}
}


void *st_get_dptr(struct splay *st, const void *key)
{
	struct std_splay *sst;
	struct stnode *node;
	struct raw r;

	abort_unless(st != NULL);
	abort_unless(key != NULL);

	sst = container(st, struct std_splay, tree);
	key = di_get_key(&sst->iface, &r, key);

	if ( (node = st_lkup(st, key)) != NULL )
		return node->data;
	else
		return NULL;
}


int st_put(struct splay *st, const void *key, void *data)
{
	struct std_splay *sst;
	struct stnode *node;
	struct raw r;
	void *nk;

	abort_unless(st != NULL);
	abort_unless(key != NULL);

	sst = container(st, struct std_splay, tree);
	abort_unless(sst->iface.datasize == 0);
	key = di_get_key(&sst->iface, &r, key);

	if ( (node = st_lkup(st, key)) != NULL ) {
		st_rem(node);
		di_free_node(&sst->iface, node);
	}

	if ( (node = di_new_node(&sst->iface, key, &nk, &data)) != NULL ) {
		st_ninit(node, nk, data);
		st_ins(st, node);
		return 1;
	} else {
		return 0;
	}
}


int st_clr(struct splay *st, const void *key)
{
	struct std_splay *sst;
	struct stnode *node;
	struct raw r;

	abort_unless(st != NULL);
	abort_unless(key != NULL);

	sst = container(st, struct std_splay, tree);
	key = di_get_key(&sst->iface, &r, key);

	if ( (node = st_lkup(st, key)) != NULL ) {
		st_rem(node);
		di_free_node(&sst->iface, node);
		return 1;
	} else {
		return 0;
	}
}



/* Heap operations */


struct heap * hp_new(struct memmgr *mm, int size, cmp_f cmp)
{
	struct heap *hp;
	void **elem = NULL;

	abort_unless(size >= 0 && cmp);

	if ( (hp = mem_get(mm, sizeof(struct heap))) == NULL )
		return NULL;

	if ( size > 0 ) {
		elem = mem_get(mm, size * sizeof(void*));
		if ( elem == NULL ) {
			mem_free(mm, hp);
			return NULL;
		}
	}
	hp_init(hp, elem, size, 0, cmp, mm);

	return hp;
}


void hp_free(struct heap *hp)
{
	struct memmgr *mm;
	abort_unless(hp != NULL && hp->mm != NULL);
	mm = hp->mm;
	mem_free(mm, hp->elem);
	mem_free(mm, hp);
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


