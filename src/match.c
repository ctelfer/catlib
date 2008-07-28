#include <cat/cat.h>
#include <cat/match.h>

#if CAT_USE_STDLIB
#include <stdlib.h>
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


void kmp_pinit(struct kmppat *kmp, struct raw *pat, size_t *skips)
{
	size_t q, k, len;
	unsigned char *pp;

	abort_unless(kmp);
	abort_unless(pat && pat->data);
	abort_unless(skips);

	kmp->pat = *pat;
	kmp->skips = skips;
	len = pat->len;
	pp = (unsigned char *)pat->data;

	skips[0] = 0;
	k = 0;
	for ( q = 1 ; q < len ; ++q ) {
		while ( k > 0 && pp[k] != pp[q] )
			k = skips[k-1];
		if ( pp[k] == pp[q] )
			++k;
		skips[q] = k;
	}
}


int kmp_match(const struct raw *str, struct kmppat *pat, size_t *loc)
{
	const char *sp, *pp;
	size_t slen, plen, q, i;

	abort_unless(str && str->data);
	abort_unless(pat && pat->skips && pat->pat.data);

	sp = str->data;
	slen = str->len;
	pp = pat->pat.data;
	plen = pat->pat.len;

	for ( i = 0, q = 0 ; i < slen ; ++i ) {
		while ( q > 0 && pp[q] != sp[i] )
			q = pat->skips[q-1];
		if ( pp[q] == sp[i] )
			++q;
		if ( q == plen ) {
			if ( loc )
				*loc = i - plen + 1;
			return 1;
		}
	}

	return 0;
}


void bm_pinit(struct bmpat *bmp, struct raw *pat, size_t *skips, 
	      size_t *scratch)
{
	size_t i, j, len;
	unsigned char *pp;

	abort_unless(bmp);
	abort_unless(pat);
	abort_unless(skips);
	abort_unless(pat->len <= ((size_t)~0 - 1));

	bmp->pat = *pat;
	bmp->skips = skips;
	len = pat->len;
	pp = (unsigned char *)pat->data;

	if ( len == 0 )
		return;

	for ( i = 0 ; i < 256 ; ++i )
		bmp->last[i] = 0;
	for ( i = 0 ; i < len ; ++i )
		bmp->last[pp[i]] = i + 1;

	for ( i = 0 ; i < len ; ++i )
		skips[i] = 0;
	i = len;
	j = len - 1;
	do { 
		scratch[j] = i + 1;
		while ( (i < len) && (pp[i] != pp[j]) ) {
			if ( skips[i] == 0 )
				skips[i] = i - j;
			i = scratch[i] - 1;
		}
		--i;
	} while ( j-- > 0 );

	for ( j = 0 ; j < len ; ++j ) {
		if ( skips[j] == 0 )
			skips[j] = i+1;
		if ( i == j )
			i = scratch[j] - 1;
	}
}


int bm_match(struct raw *str, struct bmpat *pat, size_t *loc)
{
	size_t i, j, slen, plen, skip;
	unsigned char *pp, *sp;

	abort_unless(str && str->data);
	abort_unless(pat && pat->skips && pat->pat.data);

	pp = (unsigned char *)pat->pat.data;
	plen = pat->pat.len;
	sp = (unsigned char *)str->data;
	slen = str->len;

	i = 0;
	while ( i <= slen - plen ) {
		j = plen;
		while ( j > 0 && pp[j-1] == sp[i+j-1] )
			--j;
		if ( j == 0 ) { 
			if ( loc )
				*loc = i;
			return 1;
		} else {
			skip = pat->last[sp[i+j-1]];
			if ( skip < j )
				skip = j - skip;
			else
				skip = 0;
			if ( skip < pat->skips[j-1] )
				skip = pat->skips[j-1];
			i += skip;
		}
	}

	return 0;
}


#define isexplicit(suffix) ( (suffix)->end < (suffix)->start )


static int edgecmp(void *c1, void *c2)
{
	int rv;
	struct sfxedgekey *ek1 = c1;
	struct sfxedgekey *ek2 = c2;

	if ( !(rv = (int)(ek1->node - ek2->node)) )
		rv = ek1->character - ek2->character;
	return rv;
}


static unsigned edgehash(void *key, void *notused)
{
	struct sfxedgekey *ek = key;
	return ((unsigned long)ek->node * 257) + ek->character;
}


static struct hnode * newedge(void *k, void *d, unsigned h, void *c)
{
	struct sfxedge *edge;
	struct memsys *msys = c;
	struct sfxedgekey *ek = k, *nk;

	edge = mem_get(msys, sizeof(struct sfxedge)+sizeof(struct sfxedgekey));
	nk = (struct sfxedgekey *)(edge + 1);
	memset(nk, 0, sizeof(*nk));
	nk->node = ek->node;
	nk->character = ek->character;
	ht_ninit(&edge->hentry, nk, d, h);

	return &edge->hentry;
}


static void freeedge(struct hnode *n, void *ctx)
{
	mem_free(ctx, n);
}


/* if *create == 0, do not create the node.  If *create != 0 then *create */
/* returns 0 if the node was already there or 1 if the node was created   */

static struct sfxedge *getedge(struct sfxtree *t, struct sfxnode *par, int ch,
			       int *create)
{
	struct sfxedgekey ek;
	struct hnode *node;
	struct htab *ht = &t->edges;
	unsigned hash;

	ek.node = par;
	ek.character = ch;
	if ( !(node = ht_lkup(ht, &ek, &hash)) ) {
		if ( !*create )
			return NULL;
		if ( !(node = newedge(&ek, NULL, hash, &t->sys)) ) {
			*create = 0;
			return NULL;
		}
		*create = 1;
		ht_ins(ht, node);
	} else
		*create = 0;

	return (struct sfxedge *)node;
}


static struct sfxnode *newnode(struct sfxtree *sfx)
{
	struct sfxnode *node;
	if ( !(node = mem_get(&sfx->sys, sizeof(*node))) )
		return NULL;
	node->sptr = NULL;
	return node;
}


static struct sfxnode *split(struct sfxtree *sfx, struct sfxedge *edge,
			     long off, long index)
{
	long end;
	int create;
	struct sfxedge *e1, *e2;
	struct sfxnode *newint, *newleaf;
	unsigned char *text = (unsigned char *)sfx->str.data;

	end = edge->end;

	if ( !(newint = newnode(sfx)) )
		return NULL;
	create = 1;
	if ( !(e1 = getedge(sfx, newint, text[edge->start + off], &create)) ) {
		mem_free(&sfx->sys, newint);
		return NULL;
	}
	newint->sptr = ((struct sfxedgekey *)edge->hentry.key)->node;
	abort_unless(create);
	e1->hentry.data = edge->hentry.data;
	edge->hentry.data = newint;
	e1->start = edge->start + off;
	e1->end = edge->end;
	edge->end = edge->start + off - 1;

	create = 1;
	if ( !(e2 = getedge(sfx, newint, text[index], &create)) )
		return NULL;
	abort_unless(create);
	if ( !(newleaf = newnode(sfx)) )
		return NULL;
	e2->start = index;
	e2->end = sfx->str.len - 1;
	e2->hentry.data = newleaf;

	return newint;
}


static void canonical(struct sfxtree *t, struct suffix *s)
{
	long span;
	unsigned char *text;
	struct sfxedge *edge;
	int no = 0;

	text = (unsigned char *)t->str.data;
	if ( !isexplicit(s) ) {
		edge = getedge(t, s->node, text[s->start], &no);
		abort_unless(edge);
		span = edge->end - edge->start + 1;
		while ( span <= s->end - s->start + 1 ) {
			s->start += span;
			s->node = edge->hentry.data;
			if ( s->start <= s->end ) {
				edge = getedge(t, s->node, text[s->start], &no);
				span = edge->end - edge->start + 1;
			}
		}
	}
}


static int sfx_build(struct sfxtree *sfx)
{
	long i, span;
	struct suffix active;
	struct sfxnode *lastpar, *cur;
	unsigned char *text;
	struct sfxedge *edge;
	int create;

	active.node = &sfx->root;
	active.start = 0;
	active.end = -1;
	text = (unsigned char *)sfx->str.data;

	for ( i = 0 ; i < sfx->str.len ; ++i ) {
		lastpar = NULL;
		while (1) {
			cur = active.node;
			if (isexplicit(&active)) {
				create = 1;
				edge = getedge(sfx, cur, text[i], &create);
				if ( !edge )
					return -1;
				if ( !create )
					break;
				if ( !(edge->hentry.data = newnode(sfx)) )
					return -1;
				edge->start = i;
				edge->end = sfx->str.len - 1;
			} else {
				create = 0;
				edge = getedge(sfx, cur, text[active.start],
					       &create);
				abort_unless(edge);
				span = active.end - active.start + 1;
				abort_unless(edge->end - edge->start + 1 > span);
				if ( text[edge->start + span] == text[i] )
					break;
				if ( !(cur = split(sfx, edge, span, i)) )
					return -1;
			}

			if ( lastpar && (lastpar != &sfx->root) )
				lastpar->sptr = cur;
			lastpar = cur;

			if ( active.node == &sfx->root )
				++active.start;
			else
				active.node = active.node->sptr;
			canonical(sfx, &active);
		}

		if ( lastpar && (lastpar != &sfx->root) )
			lastpar->sptr = cur;
		++active.end;
		canonical(sfx, &active);
	}

	return 0;
}


int sfx_init(struct sfxtree *sfx, struct raw *str, struct memsys *sys)
{
	struct list *entries;
	struct sfxnode *root;
	struct hashsys sfxhsys = { edgecmp, edgehash, NULL };

	abort_unless(sfx);
	abort_unless(str && str->data);
	abort_unless(str->len <= CAT_SFX_MAXLEN);

	sfx->sys = *sys;
	sfx->str = *str;
	/* XXX fix size? */
	if ( ((size_t)~0) / sizeof(struct list) < str->len )
		return -1;
	if ( !(entries = mem_get(sys, sizeof(struct list) * str->len)) )
		return -1;
	ht_init(&sfx->edges, entries, str->len, &sfxhsys);
	root = &sfx->root;
	root->sptr = NULL;

	if (sfx_build(sfx) < 0) {
		sfx_clear(sfx);
		return -1;
	}

	return 0;
}


int sfx_match(struct sfxtree *sfx, struct raw *pat, unsigned long *loc)
{
	struct sfxnode *cur;
	struct sfxedge *edge = NULL, *lastedge = NULL;
	unsigned char *cp, *end;
	long len;
	int nocre = 0;

	abort_unless(sfx);
	abort_unless(pat && pat->data);
	abort_unless(pat->len <= CAT_SFX_MAXLEN);

	len = 0;
	cur = &sfx->root;
	end = (unsigned char *)pat->data + pat->len;
	cp = (unsigned char *)pat->data;
	while ( (cp < end) &&
		((edge = getedge(sfx, cur, *cp, &nocre)) != NULL) ) {
		lastedge = edge;
		len = edge->end - edge->start + 1;
		if ( end - cp < len )
			len = end - cp;
		abort_unless(len >= 1);
		if ( memcmp(sfx->str.data + edge->start, cp, len) )
			return 0;
		cp += len;
		cur = edge->hentry.data;
	}

	if ( cp != end )
		return 0;

	if ( loc ) {
		if ( !lastedge )
			*loc = 0;
		else {
			*loc = edge->start + len - pat->len;
			abort_unless((long)*loc >= 0);
		}
	}

	return 1;
}


static void ap_edgefree(void *data, void *ctx)
{
	struct hnode *node = data;
	struct sfxtree *t = ctx;
	ht_rem(node);
	if ( node->data )
		mem_free(&t->sys, node->data);
	freeedge(node, &t->sys);
}


void sfx_clear(struct sfxtree *sfx)
{
	ht_apply(&sfx->edges, ap_edgefree, sfx);
	mem_free(&sfx->sys, sfx->edges.tab);
}

