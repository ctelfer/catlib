/*
 * pcache.c -- Pool cache for fast allocation.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/pcache.h>

void pc_init(struct pcache *pc, size_t asiz, size_t pgsiz, uint hiwat, 
	     uint maxpools, struct memmgr *mm)
{
	abort_unless(pc);
	abort_unless(asiz > 0);
	abort_unless(pgsiz == 0 || 
		     (pgsiz >= sizeof(union pc_pool_u) + pl_isiz(asiz, -1)));

	if ( pgsiz == 0 )
		pgsiz = PC_DEF_SIZE;

	if ( !maxpools )
		maxpools = ~0;

	pc->asiz     = asiz + sizeof(cat_pcpad_t);
	pc->mm       = mm;
	pc->pgsiz    = pgsiz;
	pc->hiwat    = hiwat;
	pc->maxpools = maxpools;
	pc->npools   = 0;

	l_init(&pc->avail);
	l_init(&pc->empty);
	l_init(&pc->full);
}


void pc_freeall(struct pcache *pc)
{
	struct list *lp;

	if ( ! pc->mm ) 
		return;
	while ( (lp = l_pop(&pc->avail)) )
		mem_free(pc->mm, lp);
	while ( (lp = l_pop(&pc->empty)) )
		mem_free(pc->mm, lp);
}


void * pc_alloc(struct pcache *pc)
{
	struct pool *pp;
	struct pc_pool *pcp, **item;

	abort_unless(pc);

	if ( l_isempty(&pc->avail) ) {
		if ( l_isempty(&pc->full) ) {
			if ( ! pc->mm || (pc->npools == pc->maxpools) )
				return NULL;

			pcp = mem_get(pc->mm, pc->pgsiz);
			if ( ! pcp ) 
				return NULL;
			pc_addpg(pc, pcp, pc->pgsiz);
		} else {
			pcp = (struct pc_pool *)pc->full.next;
			l_rem(&pcp->entry);
			l_ins(pc->avail.prev, &pcp->entry);
		}
	} 

	pcp = (struct pc_pool *)pc->avail.next;
	pp = &pcp->pool;

	if ( pp->fill == 1 ) {
		l_rem(&pcp->entry);
		l_ins(&pc->empty, &pcp->entry);
	}

	item = pl_alloc(pp);
	*item = pcp;
	return (char *)item + sizeof(cat_pcpad_t);
}


void pc_free(void *item)
{
	struct pcache *pc;
	struct pc_pool *pcp;
	struct pool *pp;

	abort_unless(item);

	item = (char *)item - sizeof(cat_pcpad_t);
	pcp = *(struct pc_pool **)item;
	pp = &pcp->pool;
	if ( pp->fill == 0 ) {
		l_rem(&pcp->entry);
		l_ins(pcp->cache->avail.prev, &pcp->entry);
	}

	pl_free(pp, item);

	if ( pp->fill == pp->max ) {
		pc = pcp->cache;

		l_rem(&pcp->entry);
		if ( pc->mm && (pc->hiwat > 0) && (pc->npools > pc->hiwat) ) {
			mem_free(pc->mm, pcp);
			pc->npools -= 1;
		} else {
			l_ins(&pc->full, &pcp->entry);
		}
	}
}


void pc_addpg(struct pcache *pc, void *page, size_t pgsiz)
{
	struct pc_pool *pcp;

	abort_unless(pc);
	abort_unless(page);
	abort_unless(pgsiz >= sizeof(union pc_pool_u) + pl_isiz(pc->asiz, -1));

	pcp = page;
	pl_init(&pcp->pool, pc->asiz, -1, (char *)pcp + sizeof(union pc_pool_u),
		pgsiz - sizeof(union pc_pool_u));
	pcp->cache = pc;
	l_ins(&pc->avail, &pcp->entry);
	pc->npools += 1;
}


void pc_delpg(void *page)
{
	struct pc_pool *pcp;

	abort_unless(page);

	pcp = page;
	l_rem(&pcp->entry);
	pcp->cache->npools -= 1;
	pcp->cache = NULL;
}
