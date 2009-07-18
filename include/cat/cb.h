/*
 * cat/cb.h -- Event handling and dispatch
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#ifndef __cat_cb_h
#define __cat_cb_h

#include <cat/cat.h>
#include <cat/list.h>


struct callback;
typedef int (*callback_f)(void *arg, struct callback *cb);

struct callback {
	struct list	entry;
	void *		ctx;
	callback_f	func;
};



/* cb_run returns 0 if all invocations return 0.  Otherwise stops and returns */
/* immediately on non-zero return values */

void cb_init(struct callback *cb, callback_f f, void *ctx);
int  cb_call(struct callback *cb, void *arg);
void cb_reg(struct list *cblist, struct callback *cb);
void cb_unreg(struct callback *cb);
int  cb_run(struct list *cblist, void *event);


#endif /* __cat_cb_h */
