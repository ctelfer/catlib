/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/mcache.h>


void mc_init(struct mcache *mc, int pglen, memctl_f mctl)
{
	Assert(mc);
	Assert(slabsize > sizeof(union mc_pool_u) || (!slabsize && !mctl));

	l_init(&mc->types);
	l_init(&mc->pages);
	mc->pglen = pglen;
	mc->mctl  = mctl;
}




void mc_inittype(struct mc_type *type, struct mc_tdesc *tdesc, int maxalloc,
		 int maxcache);
{
	/* lots of sanity checks */
	Assert(type);
	Assert(tdesc);
	Assert(maxalloc >= 0);
	Assert(tdesc->pgsiz >= sizeof(union mc_page_u));
	Assert(tdesc->init);
	Assert(tdesc->alloc);
	Assert(tdesc->free);

	l_init(&type->entry);
	l_init(&type->avail);
	l_init(&type->empty);
	l_init(&type->cache);

	type->tdesc = *tdesc;
	type->maxalloc = maxalloc;
	type->maxcache = maxcache;
	type->clen = 0;
}




void mc_addtype(struct mcache *mc, struct mc_type *type)
{
	Assert(mc);
	Assert(type);

	l_ins(&mc->types, &type->entry);
	type->mcache = mc;
}




void mc_freetype(struct mc_type *type)
{
	struct mcache *mc;
	struct mc_page *mcp;
	struct list *lp;

	Assert(type);
	mc = type->mcache;

	for ( lp = l_head(&type->avail) ; lp != l_end(&type->avail) ; ) {

		mcp = base(lp, struct mc_page, te);
		lp = lp->next;

		l_rem(&mcp->ce);
		l_rem(&mcp->te);

		if ( mcp->isdyn )
			mc->mctl(mcp, 0);
	}

	for ( lp = l_head(&type->empty) ; lp != l_end(&type->empty) ; ) {

		mcp = base(lp, struct mc_page, te);
		lp = lp->next;

		l_rem(&mcp->ce);
		l_rem(&mcp->te);

		if ( mcp->isdyn )
			mc->mctl(mcp, 0);
	}

	for ( lp = l_head(&type->cache) ; lp != l_end(&type->cache) ; ) {

		mcp = base(lp, struct mc_page, te);
		lp = lp->next;

		l_rem(&mcp->ce);
		l_rem(&mcp->te);

		if ( mcp->isdyn )
			mc->mctl(mcp, 0);
	}

	l_rem(&type->entry);
}




void mc_addpage(struct mcache *mc, void *page, int plen)
{
	struct mc_page *mcp;

	Assert(mc);
	Assert(page);
	Assert(plen >= sizeof(union mc_page_u));

	mcp = page;
	l_init(&mcp->te);
	l_init(&mcp->ce);
	mcp->type = NULL;
	mcp->siz  = plen;
	mcp->avail = mcp->total = 0;

	l_ins(&mc->pages, &mcp->ce);
}




void mc_rempage(struct mc_page *mcp)
{
	l_rem(&mcp->te);
	l_rem(&mcp->ce);
	mcp->type = NULL;
}




void * mc_alloc(struct mc_type *type, int len)
{
	struct mcache *mc;
	struct mc_page *mcp;
	void *item;
	struct list *lp;

	Assert(type);
	Assert(len > 0);

	mc = type->mcache;

	if ( !l_isempty(&type->avail) ) {

		mcp = base(l_head(&type->avail), struct mc_page, te);

	} else if ( !l_isempty(&type->cache) ) {

		mcp = base(l_head(&type->avail), struct mc_page, te);
		l_rem(&mcp->te);
		l_rem(&mcp->ce);
		l_ins(&type->avail, &mcp->te);
		type->clen -= 1;

	} else if ( !l_isempty(&mc->pages) ) {

		for ( lp = l_head(&mc->pages ; lp != l_end(&mc->pages) ;
		      lp = lp->next ) {

			mcp = base(lp, struct mc_page, ce);
			if ( mcp->siz <= type->tdesc.pgsiz + len + MC_MAXOHEAD )
				break;

			if ( lp == l_end(&mc->pages) ) {
				mcp = NULL;
			} else { 
				if ( mcp->type ) 
					mcp->type->clen -= 1;
				l_rem(&mcp->te);
				l_rem(&mcp->ce);
				mcp->type = type;
				type->tdesc.init(mcp, mcp->siz);
				l_ins(&type->avail, &mcp->te);
			}
		}
	}

	if ( mcp == NULL ) {
		if ( ! mc->mctl )
			return NULL;

		/* TODO:  adjust mc->pglen? */
		mcp = mc->mctl(NULL, mc->pglen);
		mcp->siz = mc->pglen;
		mcp->type = type;
		type->tdesc.init(mcp, mc->pglen);
		mcp->isdyn = 1;
		l_ins(&type->avail, &mcp->te);
	}

	/* by here we have a page with the available space */
	item = type->tdesc.alloc(mcp, len);

	if ( mcp->avail == 0 ) { 
		l_rem(&mcp->te);
		l_ins(&type->empty, &mcp->te);
	}

	return item;
}




void mc_free(void *item)
{
	struct mc_page *mcp;
	struct mc_type *type;
	struct mcache  *mc;
	int wasempty;

	Assert(item);

	/* ASSUMPTION:  the cat_pitem_t bytes before item contain a pointer */
	/* which points to memory immediately AFTER the memory page the item */
	/* was allocated from */
	mcp = (struct mc_page *) *(char **)((char *)item-sizeof(cat_pitem_t)) - 
	                         sizeof(union mc_page_u);

	wasempty = mcp->avail == 0;
	type = mcp->type;
	type->tdesc.free(mcp, item);

	if ( wasempty ) { 
		l_rem(&mcp->te);
		l_ins(&type->avail, &mcp->te);
	}

	if ( mcp->total == mcp->avail ) { 
		l_rem(&mcp->te);

		mc = type->mcache;

		/* 
		   if the cache is full then: 
		     if the page was dynamically allocated free it
		     otherwise put it on the page list with NO type
		*/
		if ( type->clen == type->maxcache ) { 
			if ( mcp->isdyn ) {
				mc->mctl(mcp, 0);
			} else { 
				mcp->type = NULL;
				l_ins(&mc->pages, &mcp->ce);
			}

		/* if the cache isn't full, put the page on the cache and */
		/* in the total list of pages */
		} else {
			type->clen += 1;
			l_ins(&type->cache, &mcp->te);
			l_ins(mc->pages.prev, &mcp->ce);
		}
	}
}




struct mc_tdesc mc_pooldesc = {
        sizeof(union mc_pool_u);      
	mc_pl_initpg,          
	mc_pl_alloc,
	mc_pl_free 
} ;           






void mc_pl_initpg(void *pagep, int siz)
{
	char *page;
	struct mc_page *mcp;
	struct mc_pool *pl;

	Assert(pagep);
	Assert(siz >= sizeof(union mc_pool_u));

	page = pagep;
	mcp = pagep;
	Assert(mcp->type->asiz > 0);

	pl  = (struct mc_pool *)mcp;

	page += sizeof(union mc_pool_u);
	siz -= sizeof(union mc_pool_u);

	pl2_init(&pl->pool, mcp->type->asiz, page, siz, &mcp->total);

	pl->isiz = pl2_isiz(siz);
	mcp->total *= pl->isiz;
	mcp->avail = mcp->total;
}




void * mc_pl_alloc(struct mc_page *mcp, int len)
{
	struct mc_pool *pl;
	void *item;

	Assert(mcp);
	Assert(mcp->type);
	Assert(len <= mcp->type->asiz);

	pl = (struct mc_pool *)mcp;

	item = pl2_alloc(&pl->pool);

	Assert(item);

	mcp->avail -= pl->isiz;
}




void mc_pl_free(struct mc_page *mcp, void *item)
{
	struct mc_pool *pl;

	Assert(mcp);
	Assert(item);

	pl2_free(item);
	pl = (struct mc_pool *)mcp;
	mcp->avail += pl->isiz;
}
