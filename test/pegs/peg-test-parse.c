#include <cat/peg.h>
#include <cat/buffer.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <cat/cpg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *readfile(const char *filename, uint *fs)
{
	FILE *fp;
	long fsize;
	char *buf;
	long nread;

	fp = fopen(filename, "r");
	if ( fp == NULL )
		errsys("opening file %s: ", filename);
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	rewind(fp);
	buf = emalloc(fsize + 1);
	nread = fread(buf, 1, fsize, fp);
	if ( fsize != nread )
		errsys("error reading file: ");
	*fs = fsize;
	return buf;
}


int lgetc(void *p)
{
	return fgetc(p);
}


int action(int n, struct raw *r, void *aux)
{
	struct cpg_state *cs = aux;
	char buf[256];
	size_t len = r->len;
	int i;
	for ( i = 0; i < cs->depth; ++i )
		fputc(' ', stderr);
	if ( len >= sizeof(buf) )
		len = sizeof(buf) - 1;
	memcpy(buf, r->data, len);
	buf[len] = '\0';
	fprintf(stderr, "Primary %d matched on \"%s\"\n", n, buf);
	return 0;
}


int main(int argc, char *argv[])
{
	FILE *pfile;
	char *buf;
	char estr[256];
	struct peg_grammar_parser pgp;
	struct peg_grammar peg;
	struct cpg_state cs;
	uint fsize;
	int i;
	int rv;

	if ( argc < 3 )
		err("usage: %s peg-filename parse-filename\n", argv[0]);
	buf = readfile(argv[1], &fsize);

	if ( peg_parse(&pgp, &peg, buf, fsize) < 0 )
		err("%s\n", peg_err_string(&pgp, estr, sizeof(estr)));
	if ( pgp.len != pgp.input_len )
		err("PEG parser only parsed to position %u of %u / line %u\n",
		    pgp.len, pgp.input_len, pgp.nlines);
	printf("Successfully parsed %s\n", argv[1]);

	for ( i = 0; i < peg.max_nodes; ++i ) {
		if ( peg.nodes[i].pn_type == PEG_PRIMARY ) {
			peg.nodes[i].pn_action_cb = &action;
			peg.nodes[i].pp_action = PEG_ACT_CALLBACK;
		}
	}

	cpg_init(&cs, &peg, lgetc);
	cs.debug = 1;

	pfile = fopen(argv[2], "r");
	if ( pfile == NULL )
		errsys("Error opening %s\n", argv[2]);

	rv = cpg_parse(&cs, pfile, &cs);

	if ( rv >= 0 )
		printf("%s parsed successfully\n", argv[2]);
	else
		printf("%s failed to parse successfully\n", argv[2]);

	return 0;
}


