#include <cat/cat.h>

#include <stdio.h>
#include <stdlib.h>

void cat_abort(const char *fn, unsigned ln, const char *expr)
{
	fprintf(stderr, "%s:%u -- check failed: %s\n", fn, ln, expr);
	abort();
	exit(255);	/* in case the signal is caught */
}

