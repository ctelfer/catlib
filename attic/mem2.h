#ifndef __mem2_h
#define __mem2_h
#include <cat/cat.h>
#include <cat/aux.h>
#include <cat/pool.h>
#include <cat/pcache.h>
#include <cat/hash.h>
#include <cat/avl.h>
#include <cat/rbtree.h>

void pl_memsys(struct pool *p, struct memsys *m);
void pc_memsys(struct pcache *p, struct memsys *m);

struct nfactory {
	union {
		struct hashsys	hsys;
		struct avlsys	asys;
		struct rbsys	rbsys;
	} nf_u;
	struct memsys * nf_msys;
	int		nf_klen;
};
#define nf_hsys		nf_u.hsys
#define nf_asys		nf_u.asys
#define nf_rbsys	nf_u.rbsys

void nf_initht(struct nfactory *nf, int klen, struct memsys *m,
               cmp_f cmp, hash_f hash, void *hctx);
void nf_initavl(struct nfactory *nf, int klen, struct memsys *m, cmp_f cmp);
void nf_initrb(struct nfactory *nf, int klen, struct memsys *m, cmp_f cmp);

#if 0 
	Example:

	struct pcache PagePC;
	struct memsys PageMC, HNodeMC, ANodeMC, RBNodeMC;
	struct pcache HNodePC, ANodePC, RBNodePC;
	struct nfactory HNodeFac, ANodeFac, RBNodeFac;

	pc_init(&PagePC, 4096, 65536 + 16 * sizeof(union pc_pool_u), 
		8, 0, xstdmem);
	pc_memsys(&PagePC, &PageMC);

	pc_init(&HNodePC, sizeof(struct hnode)+hkeylen, 4096, 0, 0, &PageMC);
	pc_memsys(&HNodePC, &HNodeMC);
	nf_initht(&HNodeFac, hkeylen, &HNodeMC, cmp_raw, ht_rhash, NULL);

	pc_init(&ANodePC, sizeof(struct anode)+akeylen, 4096, 0, 0, &PageMC);
	pc_memsys(&ANodePC, &ANodeMC);
	nf_initavl(&ANodeFac, akeylen, &ANodeMC, cmp_raw);

	pc_init(&RBNodePC, sizeof(struct rbnode)+rbkeylen, 4096, 0, 0, &PageMC);
	pc_memsys(&RBNodePC, &RBNodeMC);
	nf_initrb(&RBNodeFac, rbkeylen, &RBNodeMC, cmp_raw);

	....

	struct htab ConTab;
	ht_init(&ConTab, xmalloc(sizeof(struct list) * LEN), LEN, 
		&HNodeFac.nf_hsys);

	struct rbtree Personnel;
	rb_init(&Personnel, &RBNodeFac);

#endif /* 0 */

#endif /* __mem2_h */
