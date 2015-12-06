#include <cat/peg.h>
#include <cat/buffer.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <stdio.h>
#include <stdlib.h>


char *readfile(const char *filename)
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
	return buf;
}


int main(int argc, char *argv[])
{
	char *buf;
	struct peg_grammar peg;

	if ( argc < 2 )
		err("usage: %s filename\n", argv[0]);
	buf = readfile(argv[1]);

	if ( peg_parse(&peg, buf) < 0 ) {
		err("Error parsing %s at line %u/byte %u: %s\n",
		    argv[1], peg.eloc.line, peg.eloc.pos,
		    peg_err_message(peg.err));
	}

	printf("Successfully parsed %s\n", argv[1]);
	printf("Parsed Grammar\n");
	peg_print(&peg, stdout);

	return 0;
}


