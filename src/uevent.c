/*
 * cat/uevent.c -- UNIX event dispatch
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2008, See accompanying license
 *
 */

#include <cat/cat.h>

#if CAT_HAS_POSIX
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <cat/mem.h>
#include <cat/aux.h>
#include <cat/uevent.h>
#include <cat/err.h>
#include <cat/stduse.h>

static int  fdmax(struct avl *a);
static void tdispatch(void *lp, void *muxp);
static void iorun(void *ioep, void *param);
static void run_sig_handlers(struct uemux *mux, sigset_t *ss, int maxsig);

static sigset_t uemux_sset;
static int uemux_maxsig;
static int uemux_initialized = 0;

static int fdmax(struct avl *a)
{
	struct anode *n = avl_getmax(a);
	struct ue_ioevent *io;
	if ( !n ) 
		return -1;
	io = n->data;
	return io->fd;
}


void ue_init(struct uemux *mux, struct memmgr *mm)
{
	abort_unless(mux);

	if ( !uemux_initialized ) {
		uemux_initialized = 1;
		sigemptyset(&uemux_sset);
		uemux_maxsig = -1;
	}

	mux->mm = mm;
	mux->done = 0;
	mux->maxfd = -1;
	mux->fdtab = avl_new(mm, CAT_KT_NUM, 0, 0);
	dl_init(&mux->timers, -1, -1);
	l_init(&mux->iolist);
	FD_ZERO(&mux->rset);
	FD_ZERO(&mux->wset);
	FD_ZERO(&mux->eset);
	mux->sigtab = avl_new(mm, CAT_KT_NUM, 0, 0);
}


static void free_io(void *iop, void *mmp)
{
	ue_io_del(iop);
}


static void free_sigevent(void *sep, void *mmp)
{
	ue_sig_del(sep);
}


void ue_fini(struct uemux *mux)
{
	struct ue_timer *t;

	abort_unless(mux);

	if ( mux->mm )
		l_apply(&mux->iolist, free_io, NULL);
	avl_free(mux->fdtab);
	if ( mux->mm )
		avl_apply(mux->sigtab, free_sigevent, NULL);
	avl_free(mux->sigtab);

	while ( !dl_isempty(&mux->timers) ) {
		t = container(dl_head(&mux->timers), struct ue_timer, entry);
		ue_tm_del(t);
	}
}


void ue_stop(struct uemux *mux)
{
	mux->done = 1;
}


static void tdispatch(void *lp, void *muxp)
{
	struct list *l = lp;
	struct uemux *m = muxp;
	struct ue_timer *t;

	if ( m->done )
		return;
	l_rem(l);
	t = container(container(l, struct dlist, entry), struct ue_timer,entry);
	if ( t->flags & UE_PERIODIC ) {
		dl_init(&t->entry, t->orig / 1000, (t->orig % 1000) * 1000000);
		dl_ins(&m->timers, &t->entry);
	}
	cb_call(&t->cb, NULL);
}


void ue_tm_init(struct ue_timer *t, int flags, ulong ttl, callback_f func, 
		void *ctx)
{
	abort_unless(t);
	abort_unless(func);
	t->flags = flags;
	t->orig  = ttl;
	cb_init(&t->cb, func, ctx);
	dl_init(&t->entry, t->orig / 1000, (t->orig % 1000) * 1000000);
	t->mm = NULL;
}


void ue_tm_reg(struct uemux *mux, struct ue_timer *t)
{
	abort_unless(t);
	abort_unless(mux);
	dl_ins(&mux->timers, &t->entry);
	t->flags |= UE_TREG;
}



void ue_tm_cancel(struct ue_timer *t)
{
	abort_unless(t);
	if ( !(t->flags & UE_TREG) )
		return;
	dl_rem(&t->entry);
	t->flags &= ~UE_TREG;
}


void ue_io_init(struct ue_ioevent *io, int type, int fd, callback_f f, void *a)
{
	cb_init(&io->cb, f, a);
	l_init(&io->fdlist);
	io->fd = fd;
	io->type = type;
	io->mux = NULL;
	io->mm = NULL;
}


void ue_io_reg(struct uemux *mux, struct ue_ioevent *io)
{
	struct ue_ioevent *io2;
	int wasempty = 0;

	abort_unless(mux);
	abort_unless(io);
	abort_unless(io->type == UE_RD || io->type == UE_WR ||
		     io->type == UE_EX);
	abort_unless(io->fd >= 0);

	if ( (io2 = avl_get_dptr(mux->fdtab, &io->fd)) != NULL ) {
		l_ins(&io2->fdlist, &io->fdlist);
	} else {
		wasempty = 1;
		/* TODO: check for failure here */
		avl_put(mux->fdtab, &io->fd, io);
	}

	cb_reg(&mux->iolist, &io->cb);
	switch(io->type) {
	case UE_RD:
		FD_SET(io->fd, &mux->rset);
		break;
	case UE_WR:
		FD_SET(io->fd, &mux->wset);
		break;
	case UE_EX:
		FD_SET(io->fd, &mux->eset);
		break;
	}

	/* check if we have a new high fd */
	if ( io->fd > mux->maxfd ) 
		mux->maxfd = io->fd;

	io->mux = mux;
}


void ue_io_cancel(struct ue_ioevent *io)
{
	struct uemux *mux;
	struct list *l;
	struct ue_ioevent *io2;

	abort_unless(io);

	if ( !(mux = io->mux) )
		return;
	io->mux = NULL;
	cb_unreg(&io->cb);

	for ( l = l_head(&io->fdlist) ; l != l_end(&io->fdlist) ; l = l->next ){
		io2 = container(l, struct ue_ioevent, fdlist);
		if ( io2->type == io->type )
			break;
	}
	if ( l == l_end(&io->fdlist) ) {
		switch(io->type) {
		case UE_RD: FD_CLR(io->fd, &mux->rset); break;
		case UE_WR: FD_CLR(io->fd, &mux->wset); break;
		case UE_EX: FD_CLR(io->fd, &mux->eset); break;
		}
	}

	io2 = avl_get_dptr(mux->fdtab, &io->fd);
	abort_unless(io2);
	if ( l_isempty(&io2->fdlist) ) {
		avl_clr(mux->fdtab, &io->fd);
		if ( io->fd == mux->maxfd )
			mux->maxfd = fdmax(mux->fdtab);
	} else {
		if ( io2 == io ) {
			/* TODO: check the return code for this */
			avl_put(mux->fdtab, &io->fd, 
				container(io->fdlist.next, struct ue_ioevent, fdlist));
		}
		l_rem(&io->fdlist);
	}
}


void ue_sig_init(struct ue_sigevent *se, int signum, callback_f f, void *x)
{
	l_init(&se->cb.entry);
	se->mm = NULL;
	se->cb.func = f;
	se->cb.ctx = x;
	se->signum = signum;
	se->mux = NULL;
}


static void ue_handler(int signum)
{
	sigaddset(&uemux_sset, signum);
	if ( signum > uemux_maxsig )
		uemux_maxsig = signum;
}


void ue_sig_reg(struct uemux *mux, struct ue_sigevent *se)
{
	int wasempty = 0;
	struct ue_sigevent *se2;
	sigset_t block, oset;
	struct sigaction sa;

	se->mux = mux;
	l_init(&se->cb.entry);
	if ( (se2 = avl_get_dptr(mux->sigtab, &se->signum)) == NULL ) {
		/* ADD the signal to the list to watch */
		sigfillset(&block);
		sigprocmask(SIG_BLOCK, &block, &oset);
		sa.sa_handler = ue_handler;
		sigfillset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		/* TODO: check if this fails */
		if ( sigaction(se->signum, &sa, NULL) < 0 )
			errsys("sigaction: ");
		sigprocmask(SIG_BLOCK, &oset, NULL);
		wasempty = 1;
		/* TODO: check if this fails */
		avl_put(mux->sigtab, &se->signum, se);
	} else {
		l_ins(&se2->cb.entry, &se2->cb.entry);
	}
}


void ue_sig_cancel(struct ue_sigevent *se)
{
	struct ue_sigevent *se2;
	struct uemux *mux;

	abort_unless(se);
	if ( !(mux = se->mux) )
		return;
	se->mux = NULL;

	se2 = avl_get_dptr(mux->sigtab, &se->signum);
	abort_unless(se2);
	if ( l_isempty(&se2->cb.entry) ) {
		avl_clr(mux->sigtab, &se->signum);
		/* TODO could add code to remove handler */
		/* would need to save the old sigaction entry */
	} else {
		if ( se2 == se ) {
			struct callback *cb = container(se->cb.entry.next, 
							struct callback, entry);
			avl_put(mux->sigtab, &se->signum,
				container(cb, struct ue_sigevent, cb));
		}
		l_rem(&se->cb.entry);
	}
}


void ue_sig_clear(void)
{
	sigset_t block, oset;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, &oset);
	sigemptyset(&uemux_sset);
	uemux_maxsig = -1;
	sigprocmask(SIG_BLOCK, &oset, NULL);
}


static void run_sig_handlers(struct uemux *mux, sigset_t *ss, int maxsig)
{
	int i;
	struct list *first, *cur;
	struct callback *cb;
	struct ue_sigevent *se;

	for ( i = 0; i < maxsig ; ++i ) {
		if ( mux->done )
			return;
		if ( !sigismember(ss, i) )
			continue;

		if ( (se = avl_get_dptr(mux->sigtab, &i)) == NULL )
			continue;
		abort_unless(se);
		first = &se->cb.entry;
		do { 
			cur = first;
			cb = container(cur, struct callback, entry);
			/* TODO, fix this int2ptr */
			cb_call(cb, int2ptr(i));
			cur = cur->next;
		} while ( cur != first );
	}
}


struct ue_iorun_prm {
	struct uemux *mux;
	fd_set *	rset;
	fd_set *	wset;
	fd_set *	eset;
};


static void iorun(void *ioep, void *param)
{
	struct ue_ioevent *io =
		container(container(ioep, struct callback, entry), 
			  struct ue_ioevent, cb);
	struct ue_iorun_prm *iorp = param;
	fd_set *set = NULL;

	if ( iorp->mux->done )
		return;

	switch(io->type) {
		case UE_RD: set = iorp->rset; break;
		case UE_WR: set = iorp->wset; break;
		case UE_EX: set = iorp->eset; break;
	}

	if ( FD_ISSET(io->fd, set) )
		cb_call(&io->cb, int2ptr(io->fd));
}


void ue_next(struct uemux *mux)
{
	int i, maxsig;
	fd_set rset, wset, eset;
	struct timeval cur, delta, tv, *tvp;
	struct ue_iorun_prm iorp;
	struct list l;
	struct cat_time ct;
	sigset_t block, oset, fired;

	abort_unless(mux);
	if ( mux->done )
		return;

	tvp = NULL;
	rset = mux->rset;
	wset = mux->wset;
	eset = mux->eset;

	dl_first(&mux->timers, &ct);
	if ( tm_isset(&ct) ) { 
		delta.tv_sec  = ct.sec;
		delta.tv_usec = ct.nsec / 1000;
		gettimeofday(&cur, NULL);
		tvp = &delta;
	}

	i = select(mux->maxfd + 1, &rset, &wset, &eset, tvp);
	if ( i < 0 && errno != EINTR ) {
		errsys("ue_next (select): ");
	}

	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, &oset);
	maxsig = uemux_maxsig;
	uemux_maxsig = -1;
	if ( maxsig >= 0 )
		fired = uemux_sset;
	sigprocmask(SIG_SETMASK, &oset, NULL);
	if ( maxsig >= 0 )
		run_sig_handlers(mux, &fired, maxsig);

	if ( tvp ) { 
		gettimeofday(&tv, NULL);
		delta.tv_sec = tv.tv_sec - cur.tv_sec;
		delta.tv_usec = tv.tv_usec - cur.tv_usec;
		if (delta.tv_usec < 0) {
			delta.tv_sec -= 1;
			delta.tv_usec += 1000000;
		}
		tm_lset(&ct, delta.tv_sec, delta.tv_usec * 1000);
		l_init(&l);
		dl_adv(&mux->timers, &ct, &l);
		l_apply(&l, tdispatch, mux);
	}

	iorp.mux  = mux;
	iorp.rset = &rset;
	iorp.wset = &wset;
	iorp.eset = &eset;
	l_apply(&mux->iolist, iorun, &iorp);
}


void ue_run(struct uemux *mux)
{
	abort_unless(mux);
	while ( (!dl_isempty(&mux->timers) || (mux->maxfd >= 0)) && !mux->done )
		ue_next(mux);
}


static int done(void *arg, struct callback *cb)
{
	int *ip = cb->ctx;
	*ip = 1;
	return 0;
}


void ue_runfor(struct uemux *mux, ulong msec)
{
	int timeout = 0;
	struct ue_timer timer;

	abort_unless(mux);

	ue_tm_init(&timer, UE_TIMEOUT, msec, done, &timeout);
	ue_tm_reg(mux, &timer);

	while ( !timeout && !mux->done )
		ue_next(mux);
}


struct ue_ioevent * ue_io_new(struct uemux *m, int type, int fd, callback_f f, 
		              void *ctx)
{
	struct ue_ioevent *io;
	abort_unless(m);
	abort_unless(m->mm);

	if ( (io = mem_get(m->mm, (sizeof(*io)))) == NULL )
		return NULL;
	ue_io_init(io, type, fd, f, ctx);
	io->mm = m->mm;
	ue_io_reg(m, io);

	return io;
}


void ue_io_del(struct ue_ioevent *io)
{
	abort_unless(io);
	ue_io_cancel(io);
	if ( io->mm )
		mem_free(io->mm, io);
}


struct ue_timer * ue_tm_new(struct uemux *m, int flags, ulong ttl, callback_f f,
			    void *ctx)
{
	struct ue_timer *t;
	abort_unless(m && m->mm);

	if ( (t = mem_get(m->mm, sizeof(*t))) == NULL )
		return NULL;
	ue_tm_init(t, flags, ttl, f, ctx);
	t->mm = m->mm;
	ue_tm_reg(m, t);

	return t;
}


void ue_tm_del(struct ue_timer *t)
{
	abort_unless(t);
	ue_tm_cancel(t);
	if ( t->mm )
		mem_free(t->mm, t);
}


struct ue_sigevent * ue_sig_new(struct uemux *m, int signum, callback_f f,
				void *ctx)
{
	struct ue_sigevent *se;
	abort_unless(m && m->mm);

	if ( (se = mem_get(m->mm, sizeof(*se))) == NULL )
		return NULL;
	ue_sig_init(se, signum, f, ctx);
	se->mm = m->mm;
	ue_sig_reg(m, se);

	return se;
}


void ue_sig_del(struct ue_sigevent *se)
{
	abort_unless(se);
	ue_sig_cancel(se);
	if ( se->mm )
		mem_free(se->mm, se);
}

#endif /* CAT_HAS_POSIX */
