#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/mem2.h>

#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB
#include <string.h>
#else
#include <cat/catstdlib.h>
#endif


static struct hnode *nf_htnew(void *key, void *data, unsigned int hash, 
			      void *ctx);
static void nf_htfree(struct hnode *, void *ctx);
static struct anode *nf_avlnew(void *key, void *data, void *ctx);
static void nf_avlfree(struct anode *, void *ctx);
static struct rbnode *nf_rbnew(void *key, void *data, void *ctx);
static void nf_rbfree(struct rbnode *, void *ctx);



static void *plms_alloc(struct memsys *m, unsigned size)
{
	struct pool *p;

	Assert(m);
	p = m->ms_ctx;
	Assert(p);

	return pl_alloc(p);
}





static void plms_free(struct memsys *m, void *mem)
{
	Assert(m);
	if ( ! mem ) 
		return;
	pl_free(m->ms_ctx, mem);
}




void pl_memsys(struct pool *p, struct memsys *m)
{
	Assert(p);
	Assert(m);

	m->ms_ctx    = p;
	m->ms_alloc  = plms_alloc;
	m->ms_resize = NULL;
	m->ms_free   = plms_free;
}





static void *pcms_alloc(struct memsys *m, unsigned size)
{
	struct pcache *pc;

	Assert(m);
	pc = m->ms_ctx;
	Assert(pc);
	Assert(pc->asiz >= size);

	return pc_alloc(pc);
}




static void pcms_free(struct memsys *m, void *mem)
{
	Assert(m);
	if ( ! mem ) 
		return;
	pc_free(mem);
}




void pc_memsys(struct pcache *pc, struct memsys *m)
{
	Assert(pc);
	Assert(m);

	m->ms_ctx    = pc;
	m->ms_alloc  = pcms_alloc;
	m->ms_resize = NULL;
	m->ms_free   = pcms_free;
}




static struct hnode *nf_htnew(void *key, void *data, unsigned int hash, 
			      void *ctx)
{
	struct nfactory *nf;
	struct hnode *hn;
	Assert(ctx);

	nf = ctx;
	hn = mem_get(nf->nf_msys, sizeof(struct hnode) + nf->nf_klen);
	if ( ! hn )
		return NULL;
	ht_ninit(hn, (hn + 1), data, hash);
	memcpy(hn->key, key, nf->nf_klen);

	return hn;
}


static void nf_htfree(struct hnode *node, void *ctx)
{
	struct nfactory *nf;
	Assert(ctx);
	Assert(node);

	nf = ctx;
	mem_free(nf->nf_msys, node);
}


void nf_initht(struct nfactory *nf, int klen, struct memsys *m,
               cmp_f cmp, hash_f hash, void *hctx)
{
	Assert(nf);
	nf->nf_msys = m;
	nf->nf_klen = klen;
	nf->nf_hsys.cmp = cmp;
	nf->nf_hsys.hash = hash;
	nf->nf_hsys.hctx = hctx;
	nf->nf_hsys.hnnew = nf_htnew;
	nf->nf_hsys.hnfree = nf_htfree;
	nf->nf_hsys.nctx = nf;
}




static struct anode *nf_avlnew(void *key, void *data, void *ctx)
{
	struct nfactory *nf;
	struct anode *an;
	Assert(ctx);

	nf = ctx;
	an = mem_get(nf->nf_msys, sizeof(struct anode) + nf->nf_klen);
	if ( ! an )
		return NULL;
	avl_ninit(an, (an + 1), data);
	memcpy(an->key, key, nf->nf_klen);

	return an;
}


static void nf_avlfree(struct anode *node, void *ctx)
{
	struct nfactory *nf;
	Assert(ctx);
	Assert(node);

	nf = ctx;
	mem_free(nf->nf_msys, node);
}


void nf_initavl(struct nfactory *nf, int klen, struct memsys *m, cmp_f cmp)
{
	Assert(nf);
	nf->nf_msys = m;
	nf->nf_klen = klen;
	nf->nf_asys.cmp = cmp;
	nf->nf_asys.annew = nf_avlnew;
	nf->nf_asys.anfree = nf_avlfree;
	nf->nf_asys.actx = nf;
}




static struct rbnode *nf_rbnew(void *key, void *data, void *ctx)
{
	struct nfactory *nf;
	struct rbnode *node;
	Assert(ctx);

	nf = ctx;
	node = mem_get(nf->nf_msys, sizeof(struct rbnode) + nf->nf_klen);
	if ( ! node )
		return NULL;
	rb_ninit(node, (node + 1), data);
	memcpy(node->key, key, nf->nf_klen);

	return node;
}


static void nf_rbfree(struct rbnode *node, void *ctx)
{
	struct nfactory *nf;
	Assert(ctx);
	Assert(node);

	nf = ctx;
	mem_free(nf->nf_msys, node);
}


void nf_initrb(struct nfactory *nf, int klen, struct memsys *m, cmp_f cmp)
{
	Assert(nf);
	nf->nf_msys = m;
	nf->nf_klen = klen;
	nf->nf_rbsys.cmp = cmp;
	nf->nf_rbsys.rbnew = nf_rbnew;
	nf->nf_rbsys.rbfree = nf_rbfree;
	nf->nf_rbsys.ctx = nf;
}
