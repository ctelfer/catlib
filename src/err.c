/*
 *  err.c -- Error handling and debugging
 *
 *  Christopher Adam Telfer 
 * 
 *  Copyright 2005, 2006, 2007 -- See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/err.h>
#include <cat/emit.h>
#include <cat/stdclio.h>
#include <cat/str.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#if CAT_HAS_POSIX
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#else /* CAT_HAS_POSIX */
#define LOG_ERR		3
#define LOG_INFO 	6
#endif /* CAT_HAS_POSIX */


static int  def_log_emit_func(struct emitter *em, const void *buf, size_t len);
static void def_log_close_func(struct emitter *em);
static void eout(int, int, char *, va_list);


static struct file_emitter def_log_emitter = { 
	{ EMIT_OK, def_log_emit_func },
	NULL,
};

static log_close_f log_close_func = def_log_close_func;
static struct emitter *log_emitter = (struct emitter *)&def_log_emitter;
static int log_threshold = 0;	/* The mode for debugging */


static int def_log_emit_func(struct emitter *em, const void *buf, size_t len)
{
	static int initialized = 0;
	if ( !initialized ) {
		def_log_emitter.fe_file = stderr;
		initialized = 1;
	}
	return file_emit_func(em, buf, len);
}


static void def_log_close_func(struct emitter *em)
{
	struct file_emitter *fe = (struct file_emitter *)em;
	fclose(fe->fe_file);
}


/* This is for fatal errors.  Assume we quit */
void err(char *fmt, ...) 
{ 
	va_list ap; 

	va_start(ap, fmt);
	eout(LOG_ERR, 0, fmt, ap); 
	va_end(ap);
#if defined(CAT_DIE_DUMP) && CAT_DIE_DUMP
	abort();
#else /* CAT_DIE_DUMP */
	exit(1); 
#endif /* CAT_DIE_DUMP */
} 


void errsys(char *fmt, ...) 
{ 
	va_list ap; 

	va_start(ap, fmt);
	eout(LOG_ERR, 1, fmt, ap); 
	va_end(ap);
#if defined(CAT_DIE_DUMP) && CAT_DIE_DUMP
	abort();
#else /* CAT_DIE_DUMP */
	exit(1); 
#endif /* CAT_DIE_DUMP */
} 


void logrec(int level, char *fmt, ...) 
{ 
	va_list ap; 

	va_start(ap, fmt);
	if ( level >= log_threshold ) 
		eout(LOG_INFO, 0, fmt, ap); 
	va_end(ap);
} 


void logsys(int level, char *fmt, ...) 
{ 
	va_list ap; 

	va_start(ap, fmt);
	if ( level >= log_threshold ) 
		eout(LOG_INFO, 1, fmt, ap); 
	va_end(ap);
} 


void set_logger(struct emitter *em, log_close_f new_close_func)
{ 
	abort_unless(em);

	if ( log_close_func != NULL )
		(*log_close_func)(log_emitter);

	log_emitter = em;
	log_close_func = new_close_func;
} 


void setlogthresh(int thresh)
{
	log_threshold = thresh;
}


static void eout(int level, int sys, char *fmt, va_list ap) 
{ 
	char buf[256];
	int len;

#if CAT_HAS_POSIX
	int savee = 0;
	if ( sys ) 
		savee = errno;
#endif /* CAT_HAS_POSIX */

	len = str_vfmt(buf, sizeof(buf)-1, fmt, ap);

#if CAT_HAS_POSIX
	if ( sys && len >= 0 && len < sizeof(buf) )
		len = str_fmt(buf + len, sizeof(buf) - len, "%s\n",
			      strerror(savee)); 
#endif /* CAT_HAS_POSIX */

	if ( len < 0 ) 
		str_copy(buf, "eout() -> error with string formatting\n",
			 sizeof(buf)); 
	emit_string(log_emitter, buf);

#if CAT_HAS_POSIX
	if ( sys ) 
		errno = savee;
#endif /* CAT_HAS_POSIX */
}
