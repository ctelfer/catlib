/*
 * stdclio.h -- stdio interface matched to emit API.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_stdclio_h
#define __cat_stdclio_h
#include <cat/cat.h>
#include <cat/emit.h>
#include <cat/inport.h>
#include <stdio.h>

struct file_emitter {
	struct emitter	fe_emitter;
	FILE *		fe_file;
};

void file_emitter_init(struct file_emitter *fe, FILE *file);
int file_emitter_open(struct file_emitter *fe, const char *fname, int append);
int file_emitter_close(struct file_emitter *fe);
/* normally we wouldn't expose this.  I do it here so we can wrap it in err.c. */
int file_emit_func(struct emitter *em, const void *buf, size_t len);


#if CAT_HAS_POSIX
/* File Descriptor Emitter - emit to a Posix file descriptor */
struct fd_emitter {
	struct emitter		fde_emitter;
	int			fde_fd;
};

void fd_emitter_init(struct fd_emitter *fde, int fd);
#endif /* CAT_HAS_POSIX */


struct dynstr_emitter {
	struct emitter		ds_emitter;
	struct raw		ds_str;
	size_t			ds_fill;
	size_t			ds_max;
};

/* initialize an emitter that resizes its string buffer.  */
void dynstr_emitter_init(struct dynstr_emitter *dse, size_t max);

/* reset the string output, but don't release memory */
void dynstr_emitter_reset(struct dynstr_emitter *dse);

/* release memory and clear so subsequent calls with will not send to string */
void dynstr_emitter_clear(struct dynstr_emitter *dse);



struct file_inport {
	struct inport		in;
	FILE *			file;
};

void file_inport_init(struct file_inport *fin, FILE *fp);


#if CAT_HAS_POSIX
struct fd_inport {
	struct inport		in;
	int			fd;
};

void fd_inport_init(struct fd_inport *fdin, int fd);
#endif /* CAT_HAS_POSIX */


#endif /* __cat_stdclio_h */
