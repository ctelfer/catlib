#ifndef __cat_uevent_h
#define __cat_uevent_h

#include <cat/cat.h>

#if CAT_HAS_POSIX
#include <cat/mem.h>
#include <cat/list.h>
#include <cat/dlist.h>
#include <cat/avl.h>
#include <cat/cb.h>
#include <sys/select.h>


struct ue_ioevent {
	struct callback	cb;
	struct list	fdlist;
	int		fd;
	int		type;
	struct uemux *	mux;
};

#define UE_RD		1
#define UE_WR		2
#define UE_EX		3


struct ue_timer {
	struct dlist	entry;
	int 		flags;
	unsigned long	orig;
	struct callback	cb;
};

#define UE_TIMEOUT	0
#define UE_PERIODIC	1
#define UE_TREG		0x80


struct ue_sigevent {
	struct callback	cb;
	int		signum;
	struct uemux *	mux;
};


struct uemux {
	struct dlist	timers;
	int 		maxfd;
	struct avl *	fdtab;
	struct list	iolist;
	fd_set		rset;
	fd_set		wset;
	fd_set		eset;
	struct avl *	sigtab;
};


/* mux initialization, finalization and execution */
void ue_init(struct uemux *mux);
void ue_fini(struct uemux *mux, struct memsys *msys);
void ue_next(struct uemux *mux);
void ue_run(struct uemux *mux);
void ue_runfor(struct uemux *mux, unsigned long msec);

/* timer events */
void ue_tm_init(struct ue_timer *t, int type, unsigned long ttl, callback_f tof,
		void *arg);
void ue_tm_reg(struct uemux *mux, struct ue_timer *t);
void ue_tm_cancel(struct ue_timer *t);

/* i/o events */
void ue_io_init(struct ue_ioevent *io, int type, int fd, callback_f f, void *x);
void ue_io_reg(struct uemux *mux, struct ue_ioevent *io);
void ue_io_cancel(struct ue_ioevent *io);

/* Signal events */
void ue_sig_init(struct ue_sigevent *io, int signum, callback_f f, void *x);
void ue_sig_reg(struct uemux *mux, struct ue_sigevent *se);
void ue_sig_cancel(struct ue_sigevent *se);
void ue_sig_clear(void);

/* dynamic memory allocation versions */
struct ue_ioevent * ue_io_new(struct uemux *m, int type, int fd, callback_f f, 
                              void *ctx);
struct ue_timer * ue_tm_new(struct uemux *m, int type, unsigned long ttl, 
			    callback_f f, void *ctx);
struct ue_sigevent * ue_sig_new(struct uemux *m, int signum, callback_f f, 
			        void *ctx);
void ue_tm_del(void *);
void ue_io_del(void *);
void ue_sig_del(void *);

#endif /* CAT_HAS_POSIX */

#endif /* __cat_uevent_h */
