#ifndef __cat_mem_h
#define __cat_mem_h
#include <cat/cat.h>
#include <cat/aux.h>

struct memsys;
typedef void *(*alloc_f)(struct memsys *msys, size_t size);
typedef void *(*resize_f)(struct memsys *msys, void *old, size_t size);
typedef void  (*free_f)(struct memsys *msys, void * tofree);

struct memsys {
  alloc_f	ms_alloc;
  resize_f	ms_resize;
  free_f	ms_free;
  void *	ms_ctx;
};


void *mem_get(struct memsys *m, size_t len);
void *mem_resize(struct memsys *m, void *mem, size_t len);
void mem_free(struct memsys *m, void *mem);

extern void applyfree(void *data, void *memsys);


struct arraymsys {
  struct memsys         sys;
  byte_t *              mem;
  size_t                mlen;
  size_t                fill;
  int                   alignp2;
  int                   hi2lo;
};


void amsys_init(struct arraymsys *sys, void *mem, size_t mlen, int align,
                int hi2lo);
void amsys_reset(struct arraymsys *sys);
size_t amsys_get_fill(struct arraymsys *sys);
size_t amsys_get_avail(struct arraymsys *sys);

#endif /* __cat_mem_h */
