#ifndef __cat_dynmem_h
#define __cat_dynmem_h
#include <cat/cat.h>
#include <cat/list.h>

/* core alignment type */
union align_u {
	long		l;
	size_t		sz;
};
#define UNITSIZE	sizeof(union align_u)

struct memblk {
	union align_u		mb_len;
	struct list		mb_entry;
};

struct dynmem { 
	struct list		dm_pools;
	struct list		dm_blocks;
	struct list *		dm_current;
	void *			(*add_mem_func)(size_t len);
};

struct dynmempool {
	struct list 		dmp_entry;
	size_t			dmp_total_len;
	size_t			dmp_useable_len;
	void *			dmp_start;
};

struct dynmem_block_fake { 
	int			allocated;
	int			prev_allocated;
	size_t			size;
	void *			ptr;
};

void dynmem_init(struct dynmem *dm);
void *dynmem_malloc(struct dynmem *dm, size_t amt);
void dynmem_free(struct dynmem *dm, void *mem);
void *dynmem_realloc(struct dynmem *dm, void *omem, size_t newamt);

void dynmem_add_pool(struct dynmem *dm, void *mem, size_t len);
/* allows walking each pool in a dynmem heap */
void dynmem_each_pool(struct dynmem *dm, apply_f f, void *ctx);
void dynmem_each_block(struct dynmempool *pool, apply_f f, void *ctx);


#if CAT_HAS_FIXED_WIDTH
/* unsupported on platforms without fixed width integers */

#include <cat/cattypes.h>
#if CAT_64BIT

#ifndef TLSF_LG2_ALIM
#define TLSF_LG2_ALIM 63
#endif /* TLSF_LG2_ALIM */

#if CAT_HAS_LONGLONG
#define TLSF_ALIM (1LL << TLSF_LG2_ALIM)
typedef uint64_t tlsf_sz_t;
#else /* CAT_HAS_LONGLONG */
#define TLSF_ALIM (1L << TLSF_LG2_ALIM)
typedef ulong tlsf_sz_t;
#endif /* CAT_HAS_LONGLONG */

#define TLSF_SZ_BITS 64
#define TLSF_LG2_UNITSIZE 3 /* guess: verify by assert */

#ifndef TLSF_L2_LEN
#define TLSF_L2_LEN 6
#endif /* TLSF_L2_LEN */
#if TLSF_L2_LEN > 6
#error "Max supported L2 len is 6 in a 64-bit architecture"
#endif /* TLSF_L2_LEN */

#define tlsf_nlz(x) nlz_64(x)
#define tlsf_ntz(x) ntz_64(x)

#else /* CAT_64BIT */

#ifndef TLSF_LG2_ALIM
#define TLSF_LG2_ALIM 31
#endif /* TLSF_LG2_ALIM */
#define TLSF_ALIM (1L << TLSF_LG2_ALIM)

typedef uint32_t tlsf_sz_t;
#define TLSF_SZ_BITS 32
#define TLSF_LG2_UNITSIZE 2 /* guess: verify by assert */

#ifndef TLSF_L2_LEN
#define TLSF_L2_LEN 5
#endif /* TLSF_L2_LEN */
#if TLSF_L2_LEN > 5
#error "Max supported L2 len is 5 in a 32-bit architecture"
#endif /* TLSF_L2_LEN */

#define tlsf_nlz(x) nlz_32(x)
#define tlsf_ntz(x) ntz_32(x)

#endif /* CAT_64BIT */

#define TLSF_MINNU 	((sizeof(struct memblk)+2*UNITSIZE-1)/UNITSIZE)
#define TLSF_MINSZ	(MINNU * UNITSIZE)
#define TLSF_MINPOOL	((MINNU + 2) * UNITSIZE)
#define TLSF_NUML2	(TLSF_LG2_ALIM - TLSF_LG2_UNITSIZE)
#define TLSF_FULLBLLEN	(1 << TLSF_L2_LEN)
#define TLSF_NUMSMALL	(TLSF_L2_LEN)
#define TLSF_NUMFULL	(TLSF_NUML2 - TLSF_L2_LEN)

/* 
  TLSF_SZ_BITS list heads for each list with a # of UNITSIZE slots >= to min 
  size for the list head.  Then consider the smaller lists.  
  Number of slots there is 1 for slot 0, 2 for slot 1, 4 for slot 2... 
  So there are 2^TLSF_NUMSMALL-1 blocks in all for small lists.
*/
#define TLSF_MINBINS  TLSF_MINNU
#define TLSF_NUMHEADS ((TLSF_NUMFULL * TLSF_FULLBLLEN) + ((1 << TLSF_NUMSMALL) - 1))

struct tlsf_l2 {
	tlsf_sz_t		tl2_bm;
	int			tl2_nblists;
	struct list *		tl2_blists;
};

struct tlsf { 
	struct list		tlsf_pools;
	tlsf_sz_t 		tlsf_l1bm;
	struct tlsf_l2		tlsf_l1[TLSF_NUML2];
	struct list		tlsf_lists[TLSF_NUMHEADS];
};

struct tlsfpool {
	struct list 		tpl_entry;
	size_t			tpl_total_len;
	size_t			tpl_useable_len;
	void *			tpl_start;
};

struct tlsf_block_fake { 
	int			allocated;
	int			prev_allocated;
	size_t			size;
	void *			ptr;
};


void tlsf_init(struct tlsf *tlsf);
void tlsf_add_pool(struct tlsf *tlsf, void *mem, size_t len);
void *tlsf_malloc(struct tlsf *tlsf, size_t amt);
void tlsf_free(struct tlsf *tlsf, void *mem);
void *tlsf_realloc(struct tlsf *tlsf, void *omem, size_t newamt);
void tlsf_each_pool(struct tlsf *tlsf, apply_f f, void *ctx);
void tlsf_each_block(struct tlsfpool *pool, apply_f f, void *ctx);

#endif /* CAT_HAS_FIXED_WIDTH */

#endif /* __cat_dynmem_h */
