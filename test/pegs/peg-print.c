#include <cat/peg.h>
#include <cat/buffer.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <stdio.h>
#include <stdlib.h>


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


int main(int argc, char *argv[])
{
	char *buf;
	char estr[256];
	struct peg_grammar_parser pgp;
	struct peg_grammar peg;
	uint fsize;

	if ( argc < 2 )
		err("usage: %s filename\n", argv[0]);
	buf = readfile(argv[1], &fsize);

	if ( peg_parse(&pgp, &peg, buf, fsize, 0) < 0 )
		err("%s\n", peg_err_string(&pgp, estr, sizeof(estr)));

	printf("Successfully parsed %s\n", argv[1]);
	printf("Parsed Grammar\n");
	peg_print(&peg, stdout);

	return 0;
}


