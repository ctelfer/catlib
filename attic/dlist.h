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

#ifdef CAT_DLIST_DO_DECL
#define SAVE_INLINE CAT_USE_INLINE
#undef CAT_USE_INLINE
#define CAT_USE_INLINE 1
#endif /* CAT_DLIST_DO_DECL */

#include <cat/cat.h>
#include <cat/list.h>

#ifdef SAVE_INLINE
#undef CAT_USE_INLINE
#define CAT_USE_INLINE SAVE_INLINE
#undef SAVE_INLINE
#endif /* SAVE_INLINE */


#if defined(CAT_USE_INLINE) && CAT_USE_INLINE
#define DECL static inline
#define CAT_DLIST_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#endif /* CAT_USE_INLINE */


#define CAT_DL_EMPTY	0xffffffff
#define CAT_DL_DLIST	CAT_DL_EMPTY


struct dlist { 
	struct list	entry;
	unsigned long	ttl;
} ;


DECL void           dl_init(struct dlist *node, unsigned long ttl);
DECL void           dl_ins(struct dlist *list, struct dlist *node);
DECL unsigned long  dl_first(struct dlist *list);
DECL struct dlist * dl_deq(struct dlist *list);
DECL void           dl_adv(struct dlist *list, unsigned long amt, 
                           struct list *out);
DECL void           dl_rem(struct dlist *elem);

#define dl_head(list)	((list)->entry.next)
#define dl_end(list)	(&(list)->entry)
#define dl_next(node)	((struct dlist *)(list)->entry.next)


#if defined(CAT_DLIST_DO_DECL) && CAT_DLIST_DO_DECL


DECL void dl_init(struct dlist *node, unsigned long ttl)
{
	Assert(node);
	l_init(&node->entry);
	node->ttl  = ttl;
}




DECL void dl_ins(struct dlist *dlist, struct dlist *node)
{
	unsigned long ttl;
	struct list *t, *list;
	struct dlist *dl;

	Assert(dlist);
	Assert(node);

	list = &dlist->entry;

	ttl = node->ttl;
	for ( t = l_head(list) ; t != l_end(list) ; t = t->next ) { 
		dl = base(t, struct dlist, entry);
		if ( ttl < dl->ttl ) 
			break;
		else
			ttl -= dl->ttl;
	}

	if ( t != l_end(list) )
		dl->ttl -= ttl;
	l_ins(t->prev, &node->entry);
	node->ttl = ttl;
}




DECL unsigned long dl_first(struct dlist *dlist)
{
	Assert(dlist);

	if ( l_isempty(&dlist->entry) )
		return CAT_DL_EMPTY;
	else
		return base(l_head(&dlist->entry), struct dlist, entry)->ttl;
}




DECL struct dlist * dl_deq(struct dlist *dlist)
{
	struct list *list, *node;

	Assert(dlist);

	list = &dlist->entry;

	if ( l_isempty(list) ) 
		return NULL;
	else {
		node = l_head(list);
		l_rem(node);
		return base(node, struct dlist, entry);
	}
}




DECL void dl_adv(struct dlist *dlist, unsigned long delta, struct list *out)
{
	struct dlist *dl;
	struct list *t, *head, *list;

	Assert(dlist);
	Assert(out);

	list = &dlist->entry;

	if ( l_isempty(list) )
		return;

	for ( t = l_head(list) ; t != l_end(list) ; t = t->next ) { 
		dl = base(t, struct dlist, entry);
		if ( dl->ttl > delta ) 
			break;
		else
			delta -= dl->ttl;
	}

	if ( t != l_end(list) )
		dl->ttl -= delta;
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

	next = base(elem->entry.next, struct dlist, entry);
	if ( next->ttl != CAT_DL_DLIST )
		next->ttl += elem->ttl;
	l_rem(&elem->entry);
}



#endif /* CAT_DLIST_DO_DECL */


#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB

struct dclist { 
	struct dlist	entry;
	void *		data;
};

DECL struct dlist * dcl_new(unsigned long ttl, void *data);
DECL void *         dcl_free(struct dlist *node);
DECL void *         dcl_data(struct dlist *node);
DECL void           dcl_setd(struct dlist *node, void *data);



#if defined(CAT_DLIST_DO_DECL) && CAT_DLIST_DO_DECL

#include <stdlib.h>
#include <cat/err.h>

DECL struct dlist * dcl_new(unsigned long ttl, void *data)
{
	struct dclist *node;

	node = malloc(sizeof(*node));
	if ( ! node )
		errsys("dcl_new: ");
	dl_init(&node->entry, ttl);
	node->data = data;

	return &node->entry;
}




DECL void * dcl_free(struct dlist * nodep)
{
	void *old = NULL;
	struct dclist *node;

	if ( nodep ) {
		node = base(nodep, struct dclist, entry);
		old = node->data;
		free(node);
	}

	return old;
}




DECL void * dcl_data(struct dlist *nodep)
{

	Assert(nodep);
	return base(nodep, struct dclist, entry)->data;
}




DECL void dcl_setd(struct dlist *nodep, void *data)
{
	Assert(nodep);
	base(nodep, struct dclist, entry)->data = data;
}

#endif /* CAT_DLIST_DO_DECL */

#endif /* CAT_USE_STDLIB */

#undef DECL
#undef CAT_DLIST_DO_DECL

#endif /* __CAT_DLIST_H */
