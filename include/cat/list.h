/*
 * cat/list.h -- circular doubly linked list
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004 See accompanying license
 *
 */

#ifndef __cat_list
#define __cat_list

#include <cat/cat.h>

#if defined(CAT_USE_INLINE) && CAT_USE_INLINE 
#define DECL static inline
#define CAT_LIST_DO_DECL 1
#else /* CAT_USE_INLINE */
#define DECL
#endif /* CAT_USE_INLINE */

struct list {
	struct list *	next;
	struct list *	prev;
} ;

#define l_head(listp)	(listp)->next
#define l_tail(listp)	(listp)->prev
#define l_end(listp)	(listp)
#define l_next(entp)	(entp)->next

DECL void l_init(struct list *head);
DECL void l_ins(struct list *prev, struct list *elem);
DECL void l_rem(struct list *elem);
DECL int  l_isempty(struct list *list);
DECL int  l_onlist(struct list *elem);

DECL void          l_enq(struct list *list, struct list *elem);
DECL struct list * l_deq(struct list *list);
DECL void          l_push(struct list *list, struct list *elem);
DECL struct list * l_pop(struct list *list);
DECL void          l_apply(struct list *list, apply_f f, void *arg);

#define l_next_obj(ptr, type, member) \
	container((ptr)->member.next, type, member)
#define l_prev_obj(ptr, type, member) \
	container((ptr)->member.prev, type, member)
#define l_for_each(node, list) \
	for ( (node) = l_head(list) ; (node) != l_end(list) ; \
	      (node) = (node)->next )

#if defined(CAT_LIST_DO_DECL) && CAT_LIST_DO_DECL

DECL void l_init(struct list *head)
{
	abort_unless(head);
	head->next = head->prev = head;
}


DECL void l_ins(struct list *prev, struct list *elem)
{
	register struct list *next;

	abort_unless(prev);
	abort_unless(elem);
	abort_unless(prev->next && prev->next->prev == prev);

	next = prev->next;
	elem->next = next;
	elem->prev = prev;
	prev->next = elem;
	next->prev = elem;
}


DECL void l_rem(struct list *elem)
{
	register struct list *next, *prev;

	abort_unless(elem);

	next = elem->next; 
	prev = elem->prev;

	abort_unless(next && next->prev == elem);
	abort_unless(prev && prev->next == elem);

	prev->next = next;
	next->prev = prev;
	elem->next = elem;
	elem->prev = elem;
}


DECL void l_enq(struct list *list, struct list *elem)
{
	l_ins(list->prev, elem);
}


DECL struct list * l_deq(struct list *list)
{
	register struct list *last;

	abort_unless(list);

	last = list->next;
	if ( last != list ) {
		l_rem(last);
		return last;
	} else {
		return NULL;
	}
}


DECL void l_push(struct list *list, struct list *elem)
{
	l_ins(list, elem);
}


DECL struct list *l_pop(struct list *list)
{
	struct list * first;

	abort_unless(list);

	first = list->next;
	if ( first == list ) 
		return NULL;
	l_rem(first);
	return first;
}


DECL int l_isempty(struct list *list)
{
	abort_unless(list);
	return (list->next == list);
}


DECL int l_onlist(struct list *list)
{
	abort_unless(list);
	return (list->next != list);
}


DECL void l_apply(struct list *list, apply_f f, void *arg)
{
	struct list *t, *h;

	abort_unless(list);
	abort_unless(f);

	for ( t = list->next ; t != list ; ) {
		h = t;
		t = t->next;
		f(h, arg);
	}
}


#endif /* CAT_LIST_DO_DECL */


#undef DECL
#undef CAT_LIST_DO_DECL

#endif /* __cat_list */
