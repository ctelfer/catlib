#ifndef __cat_mem_h
#define __cat_mem_h
#include <cat/cat.h>
#include <cat/aux.h>

struct memsys;
typedef void *(*alloc_f)(struct memsys *msys, unsigned size);
typedef void *(*resize_f)(struct memsys *msys, void *old, unsigned size);
typedef void  (*free_f)(struct memsys *msys, void * tofree);

struct memsys {
	alloc_f		ms_alloc;
	resize_f	ms_resize;
	free_f		ms_free;
	void *		ms_ctx;
};


void *mem_get(struct memsys *m, unsigned long len);
void *mem_resize(struct memsys *m, void *mem, unsigned long len);
void mem_free(struct memsys *m, void *mem);

extern void applyfree(void *data, void *memsys);

#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB

extern struct memsys stdmem;

/* uses fatal system error messages */
extern void *emalloc(size_t size);
extern void *erealloc(void *old, size_t size);
extern struct memsys estdmem;

/* uses exceptions */
extern const char *XMemErr;
extern void *xmalloc(size_t size);
extern void *xrealloc(void *old, size_t size);
extern struct memsys xstdmem;

#endif /* CAT_USE_STDLIB */
#endif /* __cat_mem_h */
