#include <cat/cat.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <stdlib.h>
#include <string.h>

static void def_emalloc_abort(char *s, void *omem, size_t size, size_t nmem,
		              int syserr);
static emalloc_abort_f abort_func = def_emalloc_abort;


static void def_emalloc_abort(char *s, void *omem, size_t size, size_t nmem, 
		              int syserr)
{
	errsys(s);
}


#if CAT_DEBUG_LEVEL <= 0 

void * emalloc(size_t size)
{
	void *m;
	abort_unless(size > 0);
	if ( !(m = malloc(size)) )
		(*abort_func)("emalloc: ", NULL, size, 1, 1);
	return m;
}


void * ecalloc(size_t nmemb, size_t size)
{
	void *m;
	abort_unless(nmemb > 0 && size > 0);
	if ( !(m = calloc(nmemb, size)) )
		(*abort_func)("ecalloc: ", NULL, size, nmemb, 1);
	return m;
}


void * erealloc(void *old, size_t size)
{
	void *m;
	if ( !(m = realloc(old, size)) && (size > 0) )
		(*abort_func)("erealloc: ", old, size, 1, 1);
	return m;
}


#else /* CAT_DEBUG_LEVEL <= 0 */


void * emalloc(size_t size)
{
	void *m;

	if ( size == 0 )
		(*abort_func)("emalloc: zero size!", NULL, size, 1, 0);

	if ( !(m = malloc(size)) )
		(*abort_func)("emalloc: ", NULL, size, 1, 1);

	return m;
}


void * ecalloc(size_t nmemb, size_t size)
{
	void *m;
	size_t tlen;

	if ( size == 0 )
		(*abort_func)("ecalloc: zero size!", NULL, size, 1, 0);
	if ( nmemb == 0 )
		(*abort_func)("ecalloc: zero nmemb!", NULL, size, 1, 0);
	if ( (size_t)~0 / nmemb < size )
		(*abort_func)("ecalloc: size overflow!", NULL, size, 1, 0);

	tlen = nmemb * size;
	if ( !(m = malloc(tlen)) )
		(*abort_func)("ecalloc: ", NULL, size, nmemb, 1);

	memset(m, 0, tlen);

	return m;
}


void * erealloc(void *old, size_t size)
{
	void *m;
	if ( !(m = realloc(old, size)) && (size > 0) )
		(*abort_func)("erealloc: ", old, size, 1, 1);
	return m;
}

#endif /* CAT_DEBUG_LEVEL <= 0 */
