#ifndef __grow_h
#define __grow_h
#include <cat/cat.h>
#include <cat/mem.h>

#ifndef CAT_MAXGROW
#define CAT_MAXGROW		((size_t)~0)
#endif

#ifndef CAT_MINGROW
#define CAT_MINGROW		32
#endif

int grow(byte_t **ptr, size_t *len, size_t min);
int agrow(void **ptr, size_t isiz, size_t *lenp, size_t min);

int mem_grow(struct memsys *m, byte_t **ptr, size_t *len, size_t min);
int mem_agrow(struct memsys *m, void **ptr, size_t isiz, size_t *lenp, 
              size_t min);

#endif /* __grow_h */
