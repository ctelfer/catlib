#ifndef __cat_dynmem_h
#define __cat_dynmem_h
#include <cat/cat.h>
#include <cat/list.h>

struct dynmem { 
	struct list dm_pools;
	struct list dm_blocks;
	struct list *dm_current;
	void *(*add_mem_func)(size_t len);
};

struct dynmempool {
	struct list 	dmp_entry;
	size_t		dmp_total_len;
	size_t		dmp_useable_len;
	void *		dmp_start;
};

struct dynmem_block_fake { 
	int		allocated;
	int		prev_allocated;
	size_t		size;
	void *		ptr;
};

void dynmem_init(struct dynmem *dm);
void *dynmem_malloc(struct dynmem *dm, size_t amt);
void dynmem_free(struct dynmem *dm, void *mem);
void *dynmem_realloc(struct dynmem *dm, void *omem, size_t newamt);

void add_dynmempool(struct dynmem *dm, void *mem, size_t len);
/* allows walking each pool in a dynmem heap */
void dynmem_each_pool(struct dynmem *dm, apply_f f, void *ctx);
void dynmem_each_block(struct dynmempool *pool, apply_f f, void *ctx);

#endif /* __cat_dynmem_h */
