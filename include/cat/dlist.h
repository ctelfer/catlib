/*
 * cat/dlist.h -- Delta list header file for catlib2
 *
 * by Christopher Adam Telfer
 * 
 * Copyright 2003, See accompanying license
 *
 */
#ifndef __CAT_DLIST_H
#define __CAT_DLIST_H

#include <cat/cat.h>
#include <cat/list.h>
#include <cat/time.h>

#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define CAT_DLIST_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#endif /* CAT_USE_INLINE */

struct dlist { 
	struct list		entry;
	struct cat_time		ttl;
};

struct cdlist { 
	struct dlist		entry;
	void *			data;
};


DECL void           dl_init(struct dlist *node, long sec, long nsec);
DECL void           dl_ins(struct dlist *list, struct dlist *node);
DECL void	    dl_first(struct dlist *list, struct cat_time *next);
DECL struct dlist * dl_deq(struct dlist *list);
DECL void           dl_adv(struct dlist *list, struct cat_time *amt, 
		           struct list *out);
DECL void           dl_rem(struct dlist *elem);

#define dl_head(list)	((list)->entry.next)
#define dl_end(list)	(&(list)->entry)
#define dl_next(node)	((struct dlist *)(list)->entry.next)
#define dl_isempty(node) l_isempty(&(node)->entry)

DECL void *         cdl_data(struct dlist *node);
DECL void           cdl_set(struct dlist *node, void *data);

#if defined(CAT_DLIST_DO_DECL) && CAT_DLIST_DO_DECL


DECL void dl_init(struct dlist *node, long sec, long nsec)
{
	abort_unless(node);
	l_init(&node->entry);
	if (sec < 0 || nsec < 0)
		tm_clr(&node->ttl);
	else
		tm_lset(&node->ttl, sec, nsec);
}


DECL void dl_ins(struct dlist *dlist, struct dlist *node)
{
	struct cat_time ttl;
	struct list *t, *list;
	struct dlist *dl;

	abort_unless(dlist);
	abort_unless(node);

	list = &dlist->entry;

	ttl = node->ttl;
	for ( t = l_head(list) ; t != l_end(list) ; t = t->next ) { 
		dl = container(t, struct dlist, entry);
		if ( tm_cmp(&ttl, &dl->ttl) < 0 ) 
			break;
		else
			tm_sub(&ttl, &dl->ttl);
	}

	if ( t != l_end(list) )
		tm_sub(&dl->ttl, &ttl);
	l_ins(t->prev, &node->entry);
	node->ttl = ttl;
}


DECL void dl_first(struct dlist *dlist, struct cat_time *tm)
{
	abort_unless(dlist);
	abort_unless(tm);

	if ( l_isempty(&dlist->entry) )
		tm_clr(tm);
	else
		*tm = container(l_head(&dlist->entry), 
				struct dlist, entry)->ttl;
}


DECL struct dlist * dl_deq(struct dlist *dlist)
{
	struct list *list, *node;

	abort_unless(dlist);

	list = &dlist->entry;
	if ( l_isempty(list) ) 
		return NULL;
	else {
		node = l_head(list);
		l_rem(node);
		return container(node, struct dlist, entry);
	}
}


DECL void dl_adv(struct dlist *dlist, struct cat_time *deltap, 
		 struct list *out)
{
	struct dlist *dl;
	struct list *t, *head, *list;
	struct cat_time delta;

	abort_unless(dlist);
	abort_unless(tm_isset(deltap));
	abort_unless(out);
	delta = *deltap;

	list = &dlist->entry;

	if ( l_isempty(list) )
		return;

	for ( t = l_head(list) ; t != l_end(list) ; t = t->next ) { 
		dl = container(t, struct dlist, entry);
		if ( tm_cmp(&dl->ttl, &delta) > 0 ) 
			break;
		else
			tm_sub(&delta, &dl->ttl);
	}

	if ( t != l_end(list) )
		tm_sub(&dl->ttl, &delta);
	head = l_head(list);
	t = t->prev;

	if ( t->next == head ) 
		return;

	list->next = t->next;
	t->next->prev = list;

	head->prev = out;
	out->next  = head;
	t->next    = out;
	out->prev  = t;
}


DECL void dl_rem(struct dlist *elem)
{
	struct dlist *next;

	if ( l_onlist(&elem->entry) ) {
		next = container(elem->entry.next, struct dlist, entry);
		if ( tm_isset(&next->ttl) )
			tm_add(&next->ttl, &elem->ttl);
		l_rem(&elem->entry);
	}
}


DECL void * cdl_data(struct dlist *nodep)
{

	abort_unless(nodep);
	return container(nodep, struct cdlist, entry)->data;
}


DECL void cdl_set(struct dlist *nodep, void *data)
{
	abort_unless(nodep);
	container(nodep, struct cdlist, entry)->data = data;
}

#endif /* CAT_DLIST_DO_DECL */


#undef DECL
#undef CAT_DLIST_DO_DECL

#endif /* __CAT_DLIST_H */
