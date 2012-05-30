/*
 * dbgmem.h -- Memory management API instrumented for debugging.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __dbgmem_h
#define __dbgmem_h
#include <cat/cat.h>
#include <cat/mem.h>

void *dbg_mem_get(struct memmgr *mm, size_t amt);
void *dbg_mem_resize(struct memmgr *mm, void *p, size_t newsize);
void  dbg_mem_free(struct memmgr *mm, void *p);

ulong dbg_get_num_alloc(void);
ulong dbg_get_alloc_amt(void);
int dbg_is_dyn_mem(void *ptr);
void dbg_mem_reset(void);

extern struct memmgr dbgmem;

#endif /* __dbgmem_h */
