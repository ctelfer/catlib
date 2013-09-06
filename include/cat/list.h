/*
 * cat/list.h -- circular doubly linked list
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
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
	struct list *next;
	struct list *prev;
};

#define LIST_INITALIZER(name)	{ &(name), &(name) }
#define LIST_DECLARE(name)	struct list name = LIST_INITIALIZER(name);

#define l_head(listp)	(listp)->next
#define l_tail(listp)	(listp)->prev
#define l_end(listp)	(listp)
#define l_next(entp)	(entp)->next
#define l_prev(entp)	(entp)->prev

/* basic operations */
DECL void l_init(struct list *head);
DECL void l_ins(struct list *prev, struct list *elem);
DECL void l_rem(struct list *elem);
DECL int  l_isempty(struct list *list);
DECL int  l_onlist(struct list *elem);
DECL ulong l_length(struct list *list);

/* stack and queue functions */
DECL void          l_enq(struct list *list, struct list *elem);
DECL struct list * l_deq(struct list *list);
DECL void          l_push(struct list *list, struct list *elem);
DECL struct list * l_pop(struct list *list);
DECL void          l_apply(struct list *list, apply_f f, void *arg);

/* functions to move whole chunks from one list to another */
DECL void l_move(struct list *lsrc, struct list *ldst);
DECL void l_cut(struct list *lsrc, struct list *prev, struct list *last, 
		struct list *ldst);
DECL void l_append(struct list *list1, struct list *list2);
DECL void l_splice(struct list *prev, struct list *list2);

/* merge sort */
DECL void l_merge(struct list *l1, struct list *l2, cmp_f cmp);
DECL void l_sort(struct list *l, cmp_f cmp);

#define l_next_obj(ptr, type, member) \
	container((ptr)->member.next, type, member)

#define l_prev_obj(ptr, type, member) \
	container((ptr)->member.prev, type, member)

#define l_for_each(node, list) \
	for ( (node) = l_head(list) ; (node) != l_end(list) ; \
	      (node) = (node)->next )

#define l_for_each_safe(node, xtra, list) \
	for ( (node) = l_head(list) ; \
	      (xtra) = (node)->next, (node) != l_end(list) ; \
	      (node) = (xtra) )

#define l_for_each_rev(node, list) \
	for ( (node) = l_tail(list) ; (node) != l_end(list) ; \
	      (node) = (node)->prev )

#define l_for_each_rev_safe(node, xtra, list) \
	for ( (node) = l_tail(list) ; \
	      (xtra) = (node)->prev, (node) != l_end(list) ; \
	      (node) = (xtra) )

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


DECL ulong l_length(struct list *list)
{
	struct list *t;
	unsigned long n = 0;

	abort_unless(list && list->next);
	for ( t = list->next ; t != list ; t = t->next )
		++n;

	return n;
}


DECL void l_apply(struct list *list, apply_f f, void *arg)
{
	struct list *t, *next;

	abort_unless(list);
	abort_unless(f);

	for ( t = list->next ; t != list ; t = next ) {
		next = t->next;
		f(t, arg);
	}
}


DECL void l_move(struct list *src, struct list *dst)
{
	abort_unless(src);
	abort_unless(dst);

	dst->next = src->next;
	dst->prev = src->prev;
	src->next->prev = dst;
	src->prev->next = dst;

	src->next = src->prev = src;
}


DECL void l_cut(struct list *lsrc, struct list *prev, struct list *last, 
		struct list *ldst)
{
	struct list *start, *after;
	abort_unless(prev);
	abort_unless(last && last != lsrc);
	abort_unless(ldst && l_isempty(ldst));

	if ( prev == last )
		return;
	start = prev->next;
	after = last->next;

	ldst->next = start;
	ldst->prev = last;
	start->prev = ldst;
	last->next = ldst;

	prev->next = after;
	after->prev = prev;
}


DECL void l_append(struct list *list1, struct list *list2)
{
	abort_unless(list1);
	l_splice(l_tail(list1), list2);
}


DECL void l_splice(struct list *prev, struct list *list2)
{
	struct list *after, *l2s, *l2e;

	abort_unless(list2);
	abort_unless(prev);

	if ( l_isempty(list2) )
		return;

	after = prev->next;
	l2s = list2->next;
	l2e = list2->prev;

	l2s->prev = prev;
	prev->next = l2s;
	l2e->next = after;
	after->prev = l2e;

	list2->next = list2->prev = list2;
}


DECL void l_merge(struct list *l1, struct list *l2, cmp_f cmp)
{
	struct list mlist, *t, *rest;
	l_init(&mlist);
	while ( !l_isempty(l1) && !l_isempty(l2) ) {
		if ( (*cmp)(l_head(l1), l_head(l2)) < 0 )
			t = l_head(l1);
		else
			t = l_head(l2);
		l_rem(t);
		l_ins(l_tail(&mlist), t);
	}
	rest = l_isempty(l1) ? l2 : l1;
	while ( !l_isempty(rest) ) {
		t = l_head(rest);
		l_rem(t);
		l_ins(l_tail(&mlist), t);
	}
	l_move(&mlist, l1);
}


/* non-recursive merge sort implementation */
DECL void l_sort(struct list *l, cmp_f cmp)
{
	/* Make this the log base-2 of the largest list we expect to sort */
	struct list arr[48];
	struct list *node;
	int i, wasempty = 0, last_non_empty = 0;

	abort_unless(l);
	abort_unless(cmp);

	for ( i = 0; i < array_length(arr); ++i )
		l_init(&arr[i]);

	while ( !l_isempty(l) ) {
		l_rem(node = l_head(l));
		wasempty = l_isempty(&arr[0]);
		if ( wasempty ) {
			l_ins(&arr[0], node);
		} else {
			if ( (*cmp)(node, l_head(&arr[0])) < 0 )
				l_ins(&arr[0], node);
			else
				l_ins(l_head(&arr[0]), node);

		}
		for ( i = 1 ; i < array_length(arr) && !wasempty ; ++i ) {
			wasempty = l_isempty(&arr[i]);
			if ( wasempty ) {
				l_move(&arr[i-1], &arr[i]);
			} else {
				l_merge(&arr[i], &arr[i-1], cmp);
			}
		}
	}

	for ( i = 1 ; i < array_length(arr); ++i ) {
		if ( !l_isempty(&arr[i]) ) {
			l_merge(&arr[i], &arr[last_non_empty], cmp);
			last_non_empty = i;
		}
	}

	l_move(&arr[last_non_empty], l);
}



#endif /* CAT_LIST_DO_DECL */


#undef DECL
#undef CAT_LIST_DO_DECL


#endif /* __cat_list */
