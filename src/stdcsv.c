/*
 * stdcsv.c -- nicer way to interface with CSV files built on csv.c
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/cat.h>

#if CAT_HAS_POSIX

#include <cat/stdcsv.h>
#include <cat/stduse.h>
#include <cat/grow.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int local_getchar(void *fp)
{
	int c;
	FILE *f = fp;
	errno = 0;
	c = fgetc(f);
	if ( c == EOF ) {
		if ( ferror(f) )
			return CSV_GETC_ERR;
		else
			return CSV_GETC_EOF;
	}
	return c;
}


int csv_fopen(struct csv_state *csv, const char *filename)
{
	FILE *f;
	if ( (f = fopen(filename, "r")) == NULL )
		return CSV_ERR;
	csv_init(csv, local_getchar, f);
	return CSV_OK;
}


int csv_fclose(struct csv_state *csv)
{
	if ( fclose(csv->csv_gcctx) < 0 )
		return CSV_ERR;
	return CSV_OK;
}


int csv_read_field(struct csv_state *csv, char **res)
{
	char *s = NULL;
	size_t slen = 32, soff = 0, t;
	int code;

	if ( (s = mem_get(&stdmm, slen)) == NULL )
		return CSV_ERR;
	do {
		abort_unless(slen + 2 > slen);
		if ( soff > slen - 2 ) {
			byte_t *bp = (byte_t*)s;
			if ( mm_grow(&stdmm, &bp, &slen, slen + 2) < 0) {
				code = CSV_ERR;
				break;
			}

			s = (char *)bp;
		}
		code = csv_next(csv, &s[soff], slen - soff, &t);
		soff += t;
	} while  ( code == CSV_CNT );

	if ( code != CSV_ERR )
		*res = s;
	else
		free(s);

	return code;
}


int csv_read_rec(struct csv_state *csv, struct csv_record *cr)
{
	ulong nf, i;
	ulong rsz = 8;
	size_t size;
	int code = CSV_OK;
	char *field, **fields;
	byte_t *bp;

	abort_unless(csv != NULL);
	abort_unless(cr != NULL);

	fields = mem_get(&stdmm, sizeof(char *) * rsz);
	if ( fields == NULL )
		return CSV_ERR;

	for ( nf = 0 ; (code != CSV_REC) && (code != CSV_EOF) ; ++nf ) {
		abort_unless(nf <= (ulong)~0);
		code = csv_read_field(csv, &field);
		if ( code == CSV_ERR )
			goto err;
		else if ( code != CSV_EOF )
			break;
		if ( nf == rsz ) {
			bp = (byte_t *)fields;
			size = rsz * sizeof(char *);
			if ( mm_grow(&stdmm, &bp, &size, size + sizeof(char *)) < 0) {
				code = CSV_ERR;
				goto err;
			}
			rsz = size / sizeof(char *);
			fields = (char **)bp;
		}
		fields[nf++] = field;
	}

	if ( code == CSV_EOF ) {
		abort_unless(nf == 0);
		return CSV_EOF;
	}

	cr->cr_nfields = nf;
	cr->cr_fields = fields;

	return CSV_REC;

err:
	for ( i = 0; i < nf; ++i )
		free(fields[i]);
	free(fields);
	return CSV_ERR;
}


void csv_free_rec(struct csv_record *cr)
{
	int i;
	for ( i = 0 ; i < cr->cr_nfields ; ++i )
		free(cr->cr_fields[i]);
	free(cr->cr_fields);
	memset(cr, 0, sizeof(*cr));
}

#endif /* CAT_HAS_POSIX */
