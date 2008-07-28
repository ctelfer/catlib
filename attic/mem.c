/*
 * mem.c -- funtions needed by most or all modules
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004 See accompanying license
 *
 */

#include <cat/cat.h>
#include <cat/mem.h>


void *mem_get(struct memsys *m, unsigned long len)
{
	if ( !m || !m->ms_alloc )
		return NULL;
	return m->ms_alloc(m, len);
}


void *mem_resize(struct memsys *m, void *mem, unsigned long len)
{
	if ( !m || !m->ms_resize )
		return NULL;
	return m->ms_resize(m, mem, len);
}


void mem_free(struct memsys *m, void *mem)
{
	if ( !m || !m->ms_free )
		return;
	return m->ms_free(m, mem);
}


void applyfree(void *data, void *mp)
{
	struct memsys *m = mp;
	m->ms_free(m, data);
}




#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB
#include <stdlib.h>
#include <string.h>
#include <cat/err.h>


void * std_alloc(struct memsys *m, unsigned size) 
{
	assert(m && size > 0);
	return malloc(size);
}




void * std_resize(struct memsys *m, void *old, unsigned newsize) 
{
	assert(m && newsize > 0);
	return realloc(old, newsize);
}




void std_free(struct memsys *m, void *old) 
{
	assert(m);
	return free(old);
}




struct memsys stdmem = { 
	std_alloc,
	std_resize,
	std_free, 
	&stdmem
};





void * emalloc(size_t size)
{
	void *m;
	if ( !(m = malloc(size)) )
		errsys("emalloc: ");
	return m;
}




void * erealloc(void *old, size_t size)
{
	void *m;
	if ( !(m = realloc(old, size)) )
		errsys("erealloc: ");
	return m;
}




void * std_ealloc(struct memsys *mc, unsigned size) 
{
	void *m;
	assert(mc && size > 0);
	if ( !(m = malloc(size)) )
		errsys("malloc: ");
	return m;
}




void * std_eresize(struct memsys *mc, void *old, unsigned newsize) 
{
	void *m;
	assert(mc && newsize >= 0);
	if ( !(m = realloc(old, newsize)) )
		errsys("realloc: ");
	return m;
}




struct memsys estdmem = { 
	std_ealloc,
	std_eresize,
	std_free,
	&xstdmem
};




#include <cat/ex.h>
const char *XMemErr = "Memory Error";

void *xmalloc(size_t size)
{
	void *m;
	if ( !(m = malloc(size)) )
		ex_throw((void *)XMemErr);
	return m;
}




void *xrealloc(void *old, size_t size)
{
	void *m;
	if ( !(m = realloc(old, size)) )
		ex_throw((void *)XMemErr);
	return m;
}




void *std_xalloc(struct memsys *msys, unsigned size)
{
	assert(msys);
	return xmalloc(size);
}




void *std_xresize(struct memsys *msys, void *old, unsigned size)
{
	assert(msys);
	return xrealloc(old, size);
}




struct memsys xstdmem = { 
	std_xalloc,
	std_xresize,
	std_free,
	&xstdmem
};




#endif /* CAT_USE_STDLIB */
