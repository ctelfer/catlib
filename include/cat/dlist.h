/*
 * cat/dlist.h -- Delta list header file for catlib2
 *
 * by Christopher Adam Telfer
 * 
 * Copyright 2003-2012 -- See accompanying license
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
	cat_time_t		ttl;
};

DECL void           dl_init(struct dlist *node, long sec, long nsec);
DECL void           dl_ins(struct dlist *list, struct dlist *node);
DECL void	    dl_first(struct dlist *list, cat_time_t *next);
DECL struct dlist * dl_deq(struct dlist *list);
DECL void           dl_adv(struct dlist *list, cat_time_t amt, 
		           struct list *out);
DECL void           dl_rem(struct dlist *elem);

#define l_to_dl(le)	container(le, struct dlist, entry)
#define dl_head(list)	l_to_dl(((list)->entry.next))
#define dl_end(list)	l_to_dl((&(list)->entry))
#define dl_next(node)	l_to_dl(((struct dlist *)(list)->entry.next))
#define dl_isempty(node) l_isempty(&(node)->entry)

#if defined(CAT_DLIST_DO_DECL) && CAT_DLIST_DO_DECL


DECL void dl_init(struct dlist *node, long sec, long nsec)
{
	abort_unless(node);
	l_init(&node->entry);
	node->ttl = tm_lset(sec, nsec);
	if ( tm_cmp(node->ttl, tm_zero) < 0 )
		node->ttl = tm_lset(-1, -1);
}


DECL void dl_ins(struct dlist *dlist, struct dlist *node)
{
	cat_time_t ttl;
	struct list *t, *list;
	struct dlist *dl;

	abort_unless(dlist);
	abort_unless(node);

	list = &dlist->entry;

	ttl = node->ttl;
	for ( t = l_head(list) ; t != l_end(list) ; t = t->next ) { 
		dl = container(t, struct dlist, entry);
		if ( tm_cmp(ttl, dl->ttl) < 0 ) 
			break;
		else
			ttl = tm_sub(ttl, dl->ttl);
	}

	if ( t != l_end(list) )
		dl->ttl = tm_sub(dl->ttl, ttl);
	l_ins(t->prev, &node->entry);
	node->ttl = ttl;
}


DECL void dl_first(struct dlist *dlist, cat_time_t *tm)
{
	abort_unless(dlist);
	abort_unless(tm);

	if ( l_isempty(&dlist->entry) )
		*tm = tm_lset(-1, -1);
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


DECL void dl_adv(struct dlist *dlist, cat_time_t delta, 
		 struct list *out)
{
	struct dlist *dl;
	struct list *t, *head, *list;

	abort_unless(dlist);
	abort_unless(out);

	list = &dlist->entry;

	if ( l_isempty(list) )
		return;

	for ( t = l_head(list) ; t != l_end(list) ; t = t->next ) { 
		dl = container(t, struct dlist, entry);
		if ( tm_cmp(dl->ttl, delta) > 0 ) 
			break;
		else
			delta = tm_sub(delta, dl->ttl);
	}

	if ( t != l_end(list) )
		dl->ttl = tm_sub(dl->ttl, delta);
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
		next->ttl = tm_add(next->ttl, elem->ttl);
		l_rem(&elem->entry);
	}
}


#endif /* CAT_DLIST_DO_DECL */


#undef DECL
#undef CAT_DLIST_DO_DECL

#endif /* __CAT_DLIST_H */
