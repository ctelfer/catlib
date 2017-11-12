#include <cat/peg.h>
#include <cat/buffer.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <cat/cpg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int stack[256];
int tos = 0;


void push(int val)
{
	if ( tos >= array_length(stack) ) err("Stack overflow\n");
	stack[tos++] = val;
}


int pop(void)
{
	if ( tos <= 0 ) err("Stack underflow\n");
	return stack[--tos];
}


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


int push_num(int n, struct raw *r, void *aux)
{
	struct cpg_state *cs = aux;
	char buf[64];
	if ( r->len >= sizeof(buf) ) err("number is too long");
	memcpy(buf, r->data, r->len);
	buf[r->len] = '\0';
	push(atoi(buf));
	return 0;
}


int do_negate(int n, struct raw *r, void *aux)
{
	push(-pop());
}


int do_divide(int n, struct raw *r, void *aux)
{
	int b = pop();
	push(pop() / b);
}


int do_multiply(int n, struct raw *r, void *aux)
{
	push(pop() * pop());
}


int do_minus(int n, struct raw *r, void *aux)
{
	int b = pop();
	push(pop() - b);
}


int do_plus(int n, struct raw *r, void *aux)
{
	push(pop() + pop());
}


int output(int n, struct raw *r, void *aux)
{
	printf("result = %d\n", pop());
	fflush(stdout);
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

	if ( argc < 2 )
		err("usage: %s calc-file [expression-file]\n", argv[0]);
	buf = readfile(argv[1], &fsize);

	if ( peg_parse(&pgp, &peg, buf, fsize, 0) < 0 )
		err("%s\n", peg_err_string(&pgp, estr, sizeof(estr)));
	if ( pgp.len != pgp.input_len )
		err("PEG parser only parsed to position %u of %u / line %u\n",
		    pgp.len, pgp.input_len, pgp.nlines);
	printf("Successfully parsed %s\n", argv[1]);

	abort_unless(peg_action_set(&peg, "push_num", &push_num) == 1);
	abort_unless(peg_action_set(&peg, "do_negate", &do_negate) == 1);
	abort_unless(peg_action_set(&peg, "do_divide", &do_divide) == 1);
	abort_unless(peg_action_set(&peg, "do_multiply", &do_multiply) == 1);
	abort_unless(peg_action_set(&peg, "do_minus", &do_minus) == 1);
	abort_unless(peg_action_set(&peg, "do_plus", &do_plus) == 1);
	abort_unless(peg_action_set(&peg, "output", &output) == 1);

	cpg_init(&cs, &peg, lgetc);

	if ( argc < 3 ) {
		pfile = stdin;
	} else {
		pfile = fopen(argv[2], "r");
		if ( pfile == NULL )
			errsys("Error opening %s\n", argv[2]);
	}

	rv = cpg_parse(&cs, pfile, &cs);

	if ( rv >= 0 )
		printf("%s parsed successfully\n", argv[2]);
	else
		printf("%s failed to parse successfully\n", argv[2]);

	return 0;
}


