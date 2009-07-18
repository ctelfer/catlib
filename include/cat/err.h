/*
 * cat/err.h -- error reporting and debugging
 * 
 * Christopher Adam Telfer
 * 
 * Copyright 1999, 2000, 2001, 2002, 2003 -- See accompanying license
 * 
 */ 

#ifndef __CAT_ERR_H
#define __CAT_ERR_H

#include <cat/cat.h>
#include <cat/emit.h>

#if CAT_USE_STDIO
#include <stdio.h>
#else /* CAT_USE_STDIO */
#include <cat/catstdio.h>
#endif /* CAT_USE_STDIO */

typedef void (*log_close_f)(struct emitter *);

void err(char *fmt, ...);
void errsys(char *fmt, ...);
void logrec(int level, char *fmt, ...);
void logsys(int level, char *fmt, ...);
void set_logger(struct emitter *em, log_close_f log_close_func);
void setlogthresh(int thresh);


#define ERRCK(x)							\
do {									\
	if ( (x) < 0 )                                             	\
		err("Error at %s:%d:\n\t%s\n", __FILE__, __LINE__, #x);	\
} while (0);


#if CAT_DEBUG_LEVEL >= 1

#define CDBG \
	logrec(1, "DEBUG %s:%d\n", __FILE__, __LINE__);

#define CDBG1(f,v1) \
	logrec(1, "DEBUG %s:%d: " f "\n", __FILE__, __LINE__, v1);

#define CDBG2(f,v1,v2) \
	logrec(1, , "DEBUG %s:%d: " f "\n", __FILE__, __LINE__, v1, v2);

#define CDBG3(f,v1,v2,v3) \
	logrec(1, "DEBUG %s:%d: " f "\n", __FILE__, __LINE__, v1, v2, v3);

#define CDBG4(f,v1,v2,v3,v4)	\
	logrec(1, "DEBUG %s:%d: " f "\n", __FILE__, __LINE__, v1, v2, v3, v4);

#define CDBG5(f,v1,v2,v3,v4,v5)	\
	logrec(1, "DEBUG %s:%d: "  f "\n", __FILE__, __LINE__, \
	       v1, v2, v3, v4, v5);

#define CDBGE(f,e) \
	logrec(1, "DEBUG %s:%d: %s = " f " \n", __FILE__, __LINE__, #e , (e));

#else

#define CDBG
#define CDBG1(f,v1)
#define CDBG2(f,v1,v2)
#define CDBG3(f,v1,v2,v3)
#define CDBG4(f,v1,v2,v3,v4)
#define CDBG5(f,v1,v2,v3,v4,v5)
#define CDBGE(f,e)

#endif

#endif /* __CAT_ERR_H */
