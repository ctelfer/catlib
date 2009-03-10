#ifndef __pcache_h
#define __pcache_h

#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/pool.h>

#ifndef PC_DEF_SIZE
#define PC_DEF_SIZE	65536
#endif /* PC_DEF_SIZE */


struct pcache { 
  struct list		avail;
  struct list		full;
  struct list		empty;
  size_t		asiz;
  size_t		pgsiz;
  unsigned		npools;
  unsigned		maxpools;
  unsigned		hiwat;
  struct memsys *	memsys;
};


struct pc_pool {
  struct list		entry;
  struct pcache * 	cache;
  struct pool		pool;
};


union pc_pool_u { 
  cat_align_t		align;
  struct pc_pool	pool;
};


void  pc_init(struct pcache *pc, size_t asiz, size_t pgsiz, unsigned hiwat, 
              unsigned maxpools, struct memsys *memsys);
void  pc_freeall(struct pcache *pc);
void *pc_alloc(struct pcache *pc);
void  pc_free(void *item);

void  pc_addpg(struct pcache *pc, void *page, size_t size);
void  pc_delpg(void *page);

#endif /* __pcache_h */
