/*
 * stdclio.c -- standard IO implementation of emtter API.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/cat.h>
#include <cat/stdclio.h>
#include <stdio.h>


int file_emit_func(struct emitter *em, const void *buf, size_t len)
{
	struct file_emitter *fe = (struct file_emitter *)em;

	if ( len == 0 )
		return 0;

	fwrite(buf, 1, len, fe->fe_file);
	if ( ferror(fe->fe_file) ) {
		fe->fe_emitter.emit_state = EMIT_ERR;
		return -1;
	}

	return 0;
}


void file_emitter_init(struct file_emitter *fe, FILE *file)
{
	struct emitter *e;
	abort_unless(fe && file);

	e = &fe->fe_emitter;
	e->emit_state = EMIT_OK;
	e->emit_func = file_emit_func;
	fe->fe_file = file;
}


int file_emitter_open(struct file_emitter *fe, const char *fname, int append)
{
	FILE *fp;
	abort_unless(fe);
	if ( (fp = fopen(fname, (append ? "a" : "r"))) == NULL )
		return -1;
	file_emitter_init(fe, fp);
	return 0;
}


int file_emitter_close(struct file_emitter *fe)
{
	abort_unless(fe && fe->fe_file);
	return fclose(fe->fe_file);
}


static int finp_read(struct inport *in, void *buf, int len)
{
	struct file_inport *fin = (struct file_inport *)in;
	size_t nr;

	abort_unless(fin->file);

	if ( ferror(fin->file) )
		return -1;
	if ( feof(fin->file) )
		return 0;

	nr = fread(buf, len, 1, fin->file);
	if ( nr < len ) {
		if ( ferror(fin->file) )
			return -1;
		len = nr;
	}

	return nr;
}


static int finp_close(struct inport *in)
{
	struct file_inport *fin = (struct file_inport *)in;
	int rv;

	if ( fin->file == NULL )
		return 0;

	rv = fclose(fin->file);
	if ( rv == 0 )
		fin->file = NULL;

	return rv;
}



void finp_init(struct file_inport *fin, FILE *fp)
{
	abort_unless(fin && fp);
	fin->in.read = &finp_read;
	fin->in.close = &finp_close;
	fin->file = fp;
}


#if CAT_HAS_POSIX
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static int fd_emit_func(struct emitter *ectx, const void *buf, size_t len)
{
	size_t nwritten = 0;
	ssize_t rv;
	int fd = ((struct fd_emitter *)ectx)->fde_fd;
	const uchar *data = buf;

	while ( nwritten < len ) {
		rv = write(fd, data + nwritten, len - nwritten);
		if ( rv == -1 ) {
			if ( errno == EINTR )
				continue;
			ectx->emit_state = EMIT_ERR;
			return -1;
		} else {
			nwritten += (size_t)rv;
		}
	} 
	return 0;
}


void fd_emitter_init(struct fd_emitter *fde, int fd)
{
	struct emitter *e;
	abort_unless(fde);
	abort_unless(fd >= 0);

	e = &fde->fde_emitter;
	e->emit_state = EMIT_OK;
	e->emit_func  = fd_emit_func;
	fde->fde_fd   = fd;
}


static int fdinp_read(struct inport *in, void *buf, int len)
{
	struct fd_inport *fdin = (struct fd_inport *)in;
	abort_unless(fdin->fd >= 0);
	return read(fdin->fd, buf, len);
}


static int fdinp_close(struct inport *in)
{
	struct fd_inport *fdin = (struct fd_inport *)in;
	int rv;

	if ( fdin->fd < 0 )
		return 0;

	rv = close(fdin->fd);
	if ( rv == 0 )
		fdin->fd = -1;

	return rv;
}
	


void fd_inport_init(struct fd_inport *fdin, int fd)
{
	abort_unless(fdin && fd >= 0);
	fdin->in.read = &fdinp_read;
	fdin->in.close = &fdinp_close;
	fdin->fd = fd;
}

#endif /* CAT_HAS_POSIX */
