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


void *mem_get(struct memsys *m, size_t len)
{
	if ( !m || !m->ms_alloc )
		return 0;
	return m->ms_alloc(m, len);
}


void *mem_resize(struct memsys *m, void *mem, size_t len)
{
	if ( !m || !m->ms_resize )
		return 0;
	return m->ms_resize(m, mem, len);
}


void mem_free(struct memsys *m, void *mem)
{
	if ( !m || !m->ms_free )
		return;
	m->ms_free(m, mem);
}


void applyfree(void *data, void *mp)
{
	struct memsys *m = mp;
	m->ms_free(m, data);
}
