/*
 * cat/list.h -- circular doubly linked list
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2017 -- See accompanying license
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

/* 
 * This structure is both a list head/tail structure and nodes to
 * embed in other structures to put on a list.
 */
struct list {
	struct list *next;
	struct list *prev;
};

/* Declare a list an initialize it as empty */
#define LIST_INITALIZER(name)	{ &(name), &(name) }
#define LIST_DECLARE(name)	struct list name = LIST_INITIALIZER(name);

/* ----- Accessor macros ----- */
#define l_head(listp)	(listp)->next
#define l_tail(listp)	(listp)->prev
#define l_end(listp)	(listp)
#define l_next(entp)	(entp)->next
#define l_prev(entp)	(entp)->prev


/* ----- basic operations ----- */

/* Initialize a fresh list. */
DECL void l_init(struct list *list);

/*
 * Insert an elem into a list after prev.  Prev can be an element in the list
 * or the list head itself to insert at the beginning of the list.
 */
DECL void l_ins(struct list *prev, struct list *elem);

/* Remove an element from its list */
DECL void l_rem(struct list *elem);

/* Return a non-zero value if list is empty, otherwise return 0. */
DECL int l_isempty(struct list *list);

/* Return a non-zero value if an element is on a list otherwise return 0. */
DECL int l_onlist(struct list *elem);

/* Return the number of entries on a list.  (by linear count) */
DECL ulong l_length(struct list *list);


/* ----- Stack and queue functions ----- */

/* enqueue an element to the end of a list */
DECL void l_enq(struct list *list, struct list *elem);

/* 
 * Dequeue an element from the front of a list and return a pointer to it.
 * Return NULL if the list is empty.
 */
DECL struct list * l_deq(struct list *list);

/* push an element to the front of a list */
DECL void l_push(struct list *list, struct list *elem);

/* 
 * Pop an element from the front of a list and return a pointer to it.
 * Return NULL if the list is empty.
 */
DECL struct list * l_pop(struct list *list);

/* Apply 'f' to each element of a list passing 'arg' to 'f' as state */
DECL void l_apply(struct list *list, apply_f f, void *arg);


/* ----- Functions to move whole chunks from one list to another ----- */

/* 
 * Move the elements of 'src' to 'dst'.  dst must start empty and src will
 * be empty after the operation.
 */
DECL void l_move(struct list *dst, struct list *src);

/*
 * Move the nodes in a list from first to last into a new list dst.  dst
 * must be initially empty.  There is no way to specify to cut an empty
 * list other than to not call the function.
 */
DECL void l_cut(struct list *dst, struct list *first, struct list *last);

/*
 * Append the elements of list2 to list1.  list2 will be empty after
 * the operation.
 */
DECL void l_append(struct list *list1, struct list *list2);

/*
 * Insert the elements of list2 into a given list after prev which
 * like l_ins() may be an element in the list or the list head to
 * splice the contents to the beginning of the list.  list2 will be
 * empty after the operation.
 */
DECL void l_splice(struct list *prev, struct list *list2);

/* ----- merge sort ----- */

/* 
 * Merge the sorted list1 and list2 elements into list1 in sorted order.
 * Use cmp to compare two elements for sorting.  As always, cmp(x,y) must
 * return (< 0) if x < y, (== 0) if x == y or (> 0) if x > y.
 */
DECL void l_merge(struct list *list1, struct list *list2, cmp_f cmp);

/*
 * Run merge sort to sort a list of elements.  cmp must behave as specified
 * in l_merge() above.
 */
DECL void l_sort(struct list *list, cmp_f cmp);

/*
 * Return the next object in a list of objects with a list node where:
 *  - ptr is the pointer to the current object
 *  - type is the type of the object
 *  - member is the name of the member of type that is a list node
 */
#define l_next_obj(ptr, type, member) \
	container((ptr)->member.next, type, member)

/*
 * Return the previous object in a list of objects with a list node where:
 *  - ptr is the pointer to the current object
 *  - type is the type of the object
 *  - member is the name of the member of type that is a list node
 */
#define l_prev_obj(ptr, type, member) \
	container((ptr)->member.prev, type, member)

/*
 * Iterate forwards over a list setting node to each one in turn.
 */
#define l_for_each(node, list) \
	for ( (node) = l_head(list) ; (node) != l_end(list) ; \
	      (node) = (node)->next )

/*
 * Iterate forwards over each node in a list, but use an xtra to hold
 * list position in case the list gets altered during a list iteration.
 */
#define l_for_each_safe(node, xtra, list) \
	for ( (node) = l_head(list) ; \
	      (xtra) = (node)->next, (node) != l_end(list) ; \
	      (node) = (xtra) )

/*
 * Iterate backwards over a list setting node to each one in turn.
 */
#define l_for_each_rev(node, list) \
	for ( (node) = l_tail(list) ; (node) != l_end(list) ; \
	      (node) = (node)->prev )

/*
 * Iterate backwards over each node in a list, but use an xtra to hold
 * list position in case the list gets altered during a list iteration.
 */
#define l_for_each_rev_safe(node, xtra, list) \
	for ( (node) = l_tail(list) ; \
	      (xtra) = (node)->prev, (node) != l_end(list) ; \
	      (node) = (xtra) )


/* ----- Implementation ----- */
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


DECL void l_move(struct list *dst, struct list *src)
{
	abort_unless(dst);
	abort_unless(src);

	dst->next = src->next;
	dst->prev = src->prev;
	src->next->prev = dst;
	src->prev->next = dst;

	src->next = src->prev = src;
}


DECL void l_cut(struct list *dst, struct list *first, struct list *last)
{
	struct list *prev, *after;
	abort_unless(first);
	abort_unless(last);
	abort_unless(dst && l_isempty(dst));

	prev = first->prev;
	after = last->next;

	dst->next = first;
	dst->prev = last;
	first->prev = dst;
	last->next = dst;

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
	l_move(l1, &mlist);
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
				l_move(&arr[i], &arr[i-1]);
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

	l_move(l, &arr[last_non_empty]);
}



#endif /* CAT_LIST_DO_DECL */


#undef DECL
#undef CAT_LIST_DO_DECL


#endif /* __cat_list */
