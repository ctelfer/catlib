#include <cat/cat.h>

#if CAT_USE_STDLIB
#include <stdlib.h>
#include <stdio.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#include <cat/catstdio.h>
#endif /* CAT_USE_STDLIB */

void cat_abort(const char *fn, unsigned ln, const char *expr)
{
  fprintf(stderr, "%s:%u -- check failed: %s\n", fn, ln, expr);
  abort();
  exit(255);	/* in case the signal is caught */
}

