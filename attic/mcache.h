/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __mcache_h
#define __mcache_h
#include <cat/pool2.h>

#define MC_MAXOHEAD	sizeof(struct list)


struct mc_page {
	struct list		te;		/* type entry */
	struct list		ce;		/* cache entry */
	struct mc_type *	type;		/* pointer to curren type */
	int			isdyn;		/* was dynamically created */
	int			siz;		/* length of page */
	int			avail;		/* available memory */
	int			total;		/* total useable memory */
};


union mc_page_u { 
	CAT_ALIGN	align;
	struct mc_page	page;
};

#define MC_PGSIZ	sizeof(union mc_page_u)


typedef void  (*mc_initpg_f)(void *mem, int siz);
typedef void *(*mc_alloc_f)(struct mc_page *pg, int len);
typedef void  (*mc_free_f)(struct mc_page *pg, void *item);

struct mc_tdesc { 
	int			pgsiz;
	mc_initpg_f		init;
	mc_alloc_f		alloc;
	mc_free_f		free;
};



struct mc_type { 
	struct list		entry;
	struct mcache *		mcache;
	struct mc_tdesc		tdesc;

	int			asiz;
	int			maxalloc;
	int			maxcache;

	struct list		avail;
	struct list		empty;
	struct list		cache;
	int			clen;
};



struct mcache {
	struct list		types;
	struct list		pages;
	int			pglen;
	memctl_f		mctl;
};




void mc_init(struct mcache *mc, int pglen, memctl_f mctl);
void mc_inittype(struct mc_type *type, struct mc_tdesc *tdesc, int asiz,
		 int maxalloc, int maxcache);

void mc_addtype(struct mcache *mc, struct mc_type *type);
void mc_freetype(struct mc_type *type);

void mc_addpage(struct mcache *mc, void *page, int plen);
void mc_rempage(struct mc_page *mcp);

void * mc_alloc(struct mc_type *type, int len);
void   mc_free(void *item);



struct mc_pool {
	union mc_page_u		pageu;
	struct pool		pool;
	int			isiz;
};



union mc_pool_u {
	struct mc_pool		mcp;
	CAT_ALIGN		align;
};


void   mc_pl_initpg(void *page, int siz);
void * mc_pl_alloc(struct mc_page *mcp, int len);
void   mc_pl_free(struct mc_page *mcp, void *item);

extern struct mc_tdesc mc_pooldesc /* = { 
	sizeof(union mc_pool_u);
	mc_pl_initpg,
	mc_pl_alloc,
	mc_pl_free
} */ ;




/* example usage: 

   struct mcache Cache;
   struct mc_type MCT_hnode, MCT_htab, MCT_anode, MCT_avl;

   mc_init(&Cache, 65536, xmemctl);
   mc_inittype(&MCT_hnode, &mc_pooldesc, sizeof(struct hnode), 0, 6);
   mc_inittype(&MCT_htab, &mc_pooldesc, sizeof(struct htab), 0, 1);
   mc_inittype(&MCT_anode, &mc_pooldesc, sizeof(struct anode), 0, 6);
   mc_inittype(&MCT_avl, &mc_pooldesc, sizeof(struct avl), 0, 1);


   for ( i = 0 ; i < nitems ; ++i ) 
   {
	struct hnode *hnp;

	hnp = mc_alloc(&MCT_hnode, 1);
	...
   }


   for ( i = 0 ... )
   	mc_free(hnp);




   To create a hash table that gets nodes from a pool: 

   struct hnode * mc_newhn(void *k, void *d, unsigned h, void *ctx) 
   {
	struct hnode *hnp;
	struct mc_type *type;
	hnp = mc_alloc(type);
   };

   struct hashsys cachesys = { strcmp, strhash, NULL, mc_newhn, &MCT_hnode };
*/

#endif /* __mcache_h */
