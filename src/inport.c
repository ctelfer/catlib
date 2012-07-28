/*
 * inport.c -- generic API for an input stream.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 See accompanying license
 *
 */

#include <cat/inport.h>
#include <limits.h>
#include <string.h>

/* ------ Core API ------ */
int inp_read(struct inport *in, void *buf, int len)
{
	if ( in == NULL || in->read == NULL || buf == NULL || len <= 0 )
		return -1;
	return (*in->read)(in, buf, len);
}


int inp_getc(struct inport *in)
{
	char ch;
	int rv;
	rv = inp_read(in, &ch, 1);
	if ( rv <= 0 )
		return -1;
	return ch;
}


/* ------ Constant String Inport ------ */
static int csinp_read(struct inport *in, void *buf, int len)
{
	struct cstr_inport *csi = (struct cstr_inport *)in;

	abort_unless(csi->str != NULL);
	abort_unless(len > 0);

	if ( csi->off >= csi->slen )
		return 0;

	if ( csi->slen - csi->off > len )
		len = csi->slen - csi->off;

	memcpy(buf, csi->str + csi->off, len);

	return len;
}


void csinp_init(struct cstr_inport *csi, const char *s)
{
	abort_unless(csi);

	csi->in.read = csinp_read;
	csi->str = s;
	csi->slen = (s == NULL) ? 0 : strlen(s);
	csi->off = 0;
}


void csinp_init_len(struct cstr_inport *csi, const char *s, size_t len)
{
	abort_unless(csi);
	abort_unless((s != NULL) || (len > 0));

	csi->in.read = csinp_read;
	csi->str = s;
	csi->slen = len;
	csi->off = 0;
}


void csinp_reset(struct cstr_inport *csi)
{
	abort_unless(csi);
	csi->off = 0;
}


void csinp_clear(struct cstr_inport *csi)
{
	csinp_init(csi, NULL);
}


/* ------ Null Inport ------ */
void null_inport_init(struct inport *in)
{
}


struct inport null_inport = { NULL };

