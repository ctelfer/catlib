/*
 * cb.c -- Event dispatch
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/cb.h>


void cb_init(struct callback *cb, callback_f f, void *ctx)
{
	abort_unless(cb);
	abort_unless(f);

	l_init(&cb->entry);
	cb->ctx = ctx;
	cb->func = f;
}


int cb_call(struct callback *cb, void *arg)
{
	abort_unless(cb && cb->func);
	return (*cb->func)(arg, cb);
}


int cb_run(struct list *l, void *arg)
{
	struct list *node;
	struct callback *cb;
	int r;

	abort_unless(l);

	node = l_head(l);
	while ( node != l_end(l) ) {
		cb = container(node, struct callback, entry);
		node = node->next;
		if ( (r = (*cb->func)(arg, cb)) ) 
			return r;
	}

	return 0;
}




void cb_reg(struct list *l, struct callback *cb)
{
	abort_unless(l);
	abort_unless(cb);
	l_ins(l, &cb->entry);
}




void cb_unreg(struct callback *cb)
{
	abort_unless(cb);
	l_rem(&cb->entry);
}

