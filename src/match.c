#include <cat/cat.h>
#include <cat/match.h>

#if CAT_USE_STDLIB
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


void kmp_pinit(struct kmppat *kmp, struct raw *pat, ulong *skips)
{
	ulong q, k, len;
	uchar *pp;

	abort_unless(kmp);
	abort_unless(pat && pat->data);
	abort_unless(skips);

	kmp->pat = *pat;
	kmp->skips = skips;
	len = pat->len;
	pp = (uchar *)pat->data;

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


int kmp_match(const struct raw *str, struct kmppat *pat, ulong *loc)
{
	const char *sp, *pp;
	ulong slen, plen, q, i;

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


void bm_pinit(struct bmpat *bmp, struct raw *pat, ulong *skips, 
	      ulong *scratch)
{
	ulong i, j, len;
	byte_t *pp;

	abort_unless(bmp);
	abort_unless(pat);
	abort_unless(skips);
	abort_unless(pat->len <= ((ulong)~0 - 1));

	bmp->pat = *pat;
	bmp->skips = skips;
	len = pat->len;
	pp = pat->data;

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


int bm_match(struct raw *str, struct bmpat *pat, ulong *loc)
{
	ulong i, j, slen, plen, skip;
	uchar *pp, *sp;

	abort_unless(str && str->data);
	abort_unless(pat && pat->skips && pat->pat.data);

	pp = (uchar *)pat->pat.data;
	plen = pat->pat.len;
	sp = (uchar *)str->data;
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


static int edgecmp(const void *c1, const void *c2)
{
	int rv;
	const struct sfxedgekey *ek1 = c1;
	const struct sfxedgekey *ek2 = c2;

	if ( !(rv = (int)(ek1->node - ek2->node)) )
		rv = ek1->character - ek2->character;
	return rv;
}


static unsigned edgehash(void *key, void *notused)
{
	struct sfxedgekey *ek = key;
	return ((ulong)ek->node * 257) + ek->character;
}


static struct hnode * newedge(void *k, void *d, unsigned h, void *c)
{
	struct sfxedge *edge;
	struct memmgr *mm = c;
	struct sfxedgekey *ek = k, *nk;

	edge = mem_get(mm, sizeof(struct sfxedge) + sizeof(struct sfxedgekey));
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
		if ( !(node = newedge(&ek, NULL, hash, t->mm)) ) {
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
	if ( !(node = mem_get(sfx->mm, sizeof(*node))) )
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
	uchar *text = (uchar *)sfx->str.data;

	end = edge->end;

	if ( !(newint = newnode(sfx)) )
		return NULL;
	create = 1;
	if ( !(e1 = getedge(sfx, newint, text[edge->start + off], &create)) ) {
		mem_free(sfx->mm, newint);
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
	uchar *text;
	struct sfxedge *edge;
	int no = 0;

	text = (uchar *)t->str.data;
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
	uchar *text;
	struct sfxedge *edge;
	int create;

	active.node = &sfx->root;
	active.start = 0;
	active.end = -1;
	text = (uchar *)sfx->str.data;

	for ( i = 0 ; i < sfx->str.len ; ++i ) {
		lastpar = NULL;
		while (1) {
			cur = active.node;
			if ( isexplicit(&active) ) {
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
				abort_unless(edge->end - edge->start+1 > span);
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


int sfx_init(struct sfxtree *sfx, struct raw *str, struct memmgr *mm)
{
	struct list *entries;
	struct sfxnode *root;
	struct hashsys sfxhsys = { edgecmp, edgehash, NULL };

	abort_unless(sfx);
	abort_unless(str && str->data);
	abort_unless(str->len <= CAT_SFX_MAXLEN);

	sfx->mm = mm;
	sfx->str = *str;
	/* XXX fix size? */
	if ( ((ulong)~0) / sizeof(struct list) < str->len )
		return -1;
	if ( !(entries = mem_get(mm, sizeof(struct list) * str->len)) )
		return -1;
	ht_init(&sfx->edges, entries, str->len, &sfxhsys);
	root = &sfx->root;
	root->sptr = NULL;

	if ( sfx_build(sfx) < 0 ) {
		sfx_clear(sfx);
		return -1;
	}

	return 0;
}


int sfx_match(struct sfxtree *sfx, struct raw *pat, ulong *loc)
{
	struct sfxnode *cur;
	struct sfxedge *edge = NULL, *lastedge = NULL;
	uchar *cp, *end;
	long len;
	int nocre = 0;

	abort_unless(sfx);
	abort_unless(pat && pat->data);
	abort_unless(pat->len <= CAT_SFX_MAXLEN);

	len = 0;
	cur = &sfx->root;
	end = (uchar *)pat->data + pat->len;
	cp = (uchar *)pat->data;
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
		mem_free(t->mm, node->data);
	freeedge(node, t->mm);
}


void sfx_clear(struct sfxtree *sfx)
{
	ht_apply(&sfx->edges, ap_edgefree, sfx);
	mem_free(sfx->mm, sfx->edges.tab);
}


struct rex_parse_aux {
	struct memmgr *mm;
	struct rex_group *initial;
	struct rex_group *final;
	uchar *start;
	uchar *end;
	struct rex_group *gstack;
	struct rex_choice *lastch;
	int *eptr;
	int gnum;
};


static int rex_parse(struct rex_node **rxnn, uchar *p, 
		     struct rex_parse_aux *aux);


static int rex_parse_error(struct rex_node **rxnn, struct rex_parse_aux *aux, 
		           uchar *p)
{
	*rxnn = NULL;
	if ( aux->eptr ) {
		if ( p ) 
			*aux->eptr = p - aux->start;
		else
			*aux->eptr = -1;
	}
	return -1;
}


static int parse_bound(uchar **p)
{
	int v = 0;
	uchar *dp = *p;

	while ( isdigit(*dp) ) {
		v = v * 10 + (*dp - '0');
		++dp;
	}
	if ( dp == *p )
		return REX_WILDCARD;
	if ( v < 0 || v > 255 )
		return -1;
	*p = dp;
	return v;
}


static int rex_check_repitions(struct rex_node *rxn, uchar **pp,
		               struct rex_parse_aux *aux)
{
	uchar *p = *pp;

	if ( p == aux->end )
		return 0;
	if ( *p == '?' ) { 
		rxn->repmin = 0;
		rxn->repmax = 1;
		*pp = p + 1;
	} else if ( *p == '*' ) { 
		rxn->repmin = 0;
		rxn->repmax = REX_WILDCARD;
		*pp = p + 1;
	} else if ( *p == '+' ) { 
		rxn->repmin = 1;
		rxn->repmax = REX_WILDCARD;
		*pp = p + 1;
	} else if ( *p == '{' ) {
		int v;
		++p;
		v = parse_bound(&p);
		if ( v < 0 )
			return rex_parse_error(&rxn->next, aux, p);
		rxn->repmin = (v == REX_WILDCARD) ? 0 : v;
		if ( *p != ',' ) { 
			if ( *p != '}' || v < 1 )
				return rex_parse_error(&rxn->next, aux, p);
			rxn->repmax = v;
			*pp = p + 1;
			return 0;
		}

		++p;
		v = parse_bound(&p);
		if ( v < 1 )
			return rex_parse_error(&rxn->next, aux, p);
		rxn->repmax = v;
		if ( rxn->repmin > rxn->repmax ) 
			return rex_parse_error(&rxn->next, aux, *pp);
		if ( *p != '}' )
			return rex_parse_error(&rxn->next, aux, p);
		*pp = p + 1;
	}

	return 0;
}


static int span_regular(uchar *p, unsigned max)
{
	static const char *special = "*[](){}+?|.\\$^";
	uchar *t = p;
	while ( max-- > 0 && !strchr(special, *t) ) ++t;
	return t - p;
}


static int rex_parse_str(struct rex_node **rxnn, uchar *p,
			 struct rex_parse_aux *aux)
{
	int rv, len = 0, maxlen = aux->end - p, span;
	struct rex_node_str *rs;
	uchar *s, *op;

	if ( !(rs = mem_get(aux->mm, sizeof(*rs))) )
		return rex_parse_error(rxnn, aux, NULL);
	*rxnn = (struct rex_node *)rs;
	rs->base.type = REX_T_STRING;
	rs->base.repmin = rs->base.repmax = 1;
	rs->base.next = NULL;
	rxnn = &rs->base.next;
	s = rs->str;

	if ( maxlen > sizeof(rs->str) )
		maxlen = sizeof(rs->str);

	do { 
		span = span_regular(p, maxlen);
		memcpy(s, p, span);
		s += span;
		p += span;
		maxlen -= span;
		len += span;
		if ( span == 0 ) { 
			if ( *p != '\\' )
				break;
			if ( maxlen < 2 )
				return rex_parse_error(rxnn, aux, p);
			*s++ = *(p + 1);
			p += 2;
			--maxlen;
			++len;
		}
	} while ( maxlen > 0 );

	rs->len = len;
	
	op = p;
	if ( (rv = rex_check_repitions(&rs->base, &p, aux)) < 0 )
		return rv;
	if ( p != op ) { 
		if ( rs->len > 1 ) {
			struct rex_node_str *rs2;
			if ( !(rs2 = mem_get(aux->mm, sizeof(*rs2))) )
				return rex_parse_error(rxnn, aux, NULL);
			rs->len -= 1;
			*rxnn = (struct rex_node *)rs2;
			rs2->base.type = REX_T_STRING;
			rs2->base.repmin = rs->base.repmin;
			rs2->base.repmax = rs->base.repmax;
			rs2->base.next = NULL;
			rs2->len = 1;
			rxnn = &rs2->base.next;
			rs->base.repmin = rs->base.repmax = 1;
		}
	}
	return rex_parse(rxnn, p, aux);
}


static int rex_parse_class(struct rex_node **rxnn, uchar *p, 
		           struct rex_parse_aux *aux)
{
	uchar *start = p + 1, *end = start;
	int invert = 0, i, rv;
	struct rex_ascii_class *rxc;

	/* handle the special cases for the first character */
	if ( end == aux->end ) { return rex_parse_error(rxnn, aux, p - 1); }
	if ( *end == '^' )     { ++invert; ++end; ++start; }
	if ( end == aux->end ) { return rex_parse_error(rxnn, aux, p - 1); }
	if ( *end == ']' )     { ++end; } 

	/* find the end delimeter and make sure we */
	for ( ; end < aux->end && *end != ']'; ++end )
		;
	if ( end == start || end == aux->end )
		rex_parse_error(rxnn, aux, p - 1);

	if ( !(rxc = mem_get(aux->mm, sizeof(*rxc))) )
		return rex_parse_error(rxnn, aux, NULL);
	*rxnn = &rxc->base;
	rxc->base.type = REX_T_CLASS;
	rxc->base.repmin = rxc->base.repmax = 1;
	rxc->base.next = NULL;
	rxnn = &rxc->base.next;
	memset(&rxc->set, 0, sizeof(rxc->set));

	for ( p = start; p < end; ++p ) {
		if ( *p == '-' && p != start && p != end - 1 ) { 
			uchar ch = *(p - 1);
			uchar last = *(p + 1);
			if ( ch > last )
				return rex_parse_error(rxnn, aux, p - 1);
			do {
				rxc->set[(ch >> 3) & 0x1F] |= (1 << (ch & 0x7));
			} while ( ch++ < last );
			++p;
		} else {
			rxc->set[(*p >> 3) & 0x1F] |= (1 << (*p & 0x7));
		}
	}

	if ( invert ) {
		for ( i = 0; i < 32 ; ++i )
			rxc->set[i] = ~rxc->set[i];
	}

	p = end + 1;
	if ( (rv = rex_check_repitions(&rxc->base, &p, aux)) < 0 )
		return rv;
	return rex_parse(rxnn, p, aux);
}


static int rex_parse_any(struct rex_node **rxnn, uchar *p, 
		         struct rex_parse_aux *aux)
{
	struct rex_ascii_class *rc;
	int rv;

	if ( !(rc = mem_get(aux->mm, sizeof(*rc))) )
		return rex_parse_error(rxnn, aux, NULL);
	*rxnn = &rc->base;
	rc->base.type = REX_T_CLASS;
	rc->base.repmin = rc->base.repmax = 1;
	rc->base.next = NULL;
	rxnn = &rc->base.next;
	memset(rc->set, ~0, sizeof(rc->set));
	++p;
	if ( (rv = rex_check_repitions(&rc->base, &p, aux)) < 0 )
		return rv;
	return rex_parse(rxnn, p, aux);
}


static int rex_parse_choice(struct rex_node **rxnn, uchar *p, 
		            struct rex_parse_aux *aux)
{
	struct rex_choice *rc;

	if ( !(rc = mem_get(aux->mm, sizeof(*rc))) )
		return rex_parse_error(rxnn, aux, NULL);
	rc->base.type = REX_T_CHOICE;
	rc->base.repmin = rc->base.repmax = 1;
	rc->base.next = NULL;

	if ( aux->lastch == NULL ) { 
		struct rex_group *rg = aux->gstack;
		if ( rg == NULL )
			rg = aux->initial;
		rc->opt1 = rg->base.next;
		rg->base.next = &rc->base;
	} else { 
		rc->opt1 = aux->lastch->opt2;
		aux->lastch->opt2 = &rc->base;
	}
	rc->opt2 = NULL;
	*rxnn = NULL;
	rxnn = &rc->opt2;
	aux->lastch = rc;

	return rex_parse(rxnn, p+1, aux);
}


static int rex_parse_group_s(struct rex_node **rxnn, uchar *p, 
		             struct rex_parse_aux *aux)
{
	struct rex_group *rg;

	if ( !(rg = mem_get(aux->mm, sizeof(*rg))) )
		return rex_parse_error(rxnn, aux, NULL);
	*rxnn = &rg->base;
	rg->base.type = REX_T_GROUP_S;
	rg->base.repmin = rg->base.repmax = 1;
	rg->base.next = NULL;
	rxnn = &rg->base.next;
	rg->num = ++aux->gnum;

	rg->other = aux->gstack;
	aux->gstack = rg;
	aux->lastch = NULL;

	return rex_parse(rxnn, p+1, aux);
}


static int finalize_choices(struct rex_group *rgs, struct rex_group *rge,
		            struct rex_parse_aux *aux)
{
	struct rex_node *rn, **trav;
	struct rex_choice *rc;
	if ( aux->lastch == NULL )
		return 0;
	abort_unless(rgs->base.next->type == REX_T_CHOICE);

	/* we should now have a linked list of choices linked through the */
	/* ->opt2 pointers with the last opt2 pointer pointing to a       */
	/* non-choice node.  Each of the opt1 lists should end with a node */
	/* pointing to NULL.  The list _can_ be empty meaning an opt1      */
	/* can point to NULL.  Any sub-groups with choices should have the */
	/* form of a group start node followed by a list of choices _all_ */
	/* of which point to the group end node through their next pointer. */
	/* So, hitting such a group should go from start to choice to end to */
	/* whatever follows in the rest of the choice expression. This code */
	/* changes all the final NULL pointers to point to the group end node.*/
	/* Note that the last list will already point to the end node. */
	rn = rgs->base.next;
	while ( rn->type == REX_T_CHOICE ) {
		rc = (struct rex_choice *)rn;
		rc->base.next = &rge->base;
		trav = &rc->opt1;
		while ( *trav != NULL )
			trav = &(*trav)->next;
		*trav = &rge->base;
		rn = rc->opt2;
	}
	return 0;
}


static int rex_parse_group_e(struct rex_node **rxnn, uchar *p, 
		             struct rex_parse_aux *aux)
{
	struct rex_group *rge, *rgs, *rgn;
	int rv;

	/* Check the group start stack and pop the top one */
	if ( aux->gstack == NULL )
		return rex_parse_error(rxnn, aux, p);
	rgs = aux->gstack;
	aux->gstack = rgs->other;


	/* allocate node */
	if ( !(rge = mem_get(aux->mm, sizeof(*rge))) )
		return rex_parse_error(rxnn, aux, NULL);
	*rxnn = &rge->base;
	rge->base.type = REX_T_GROUP_E;
	rge->base.repmin = rge->base.repmax = 1;
	rge->base.next = NULL;
	rxnn = &rge->base.next;
	rgs->other = rge;
	rge->other = rgs;
	rge->num = rgs->num;
	++p;

	/* close out any current choice nodes */
	finalize_choices(rgs, rge, aux);

	/* reinstate enclosing group's choice (if any) */
	if ( ((rgn = aux->gstack) != NULL) && 
			 (rgn->base.next->type == REX_T_CHOICE) ) {
		struct rex_choice *rc = (struct rex_choice *)rgn->base.next;
		while ( rc->opt2->type == REX_T_CHOICE )
			rc = (struct rex_choice *)rc->opt2;
		aux->lastch = rc;
	} else {
		aux->lastch = NULL;
	}

	if ( (rv = rex_check_repitions(&rge->base, &p, aux)) < 0 )
		return rv;
	return rex_parse(rxnn, p, aux);
}


static int rex_parse_anchor(struct rex_node **rxnn, uchar *p, 
		            struct rex_parse_aux *aux)
{
	struct rex_node *rn;
	if ( !(rn = mem_get(aux->mm, sizeof(*rn))) )
		return rex_parse_error(rxnn, aux, NULL);
	*rxnn = rn;
	rn->type = (*p == '^') ? REX_T_BANCHOR : REX_T_EANCHOR;
	rn->repmin = rn->repmax = 1;
	rn->next = NULL;
	rxnn = &rn->next;

	return rex_parse(rxnn, p + 1, aux);
}


static int rex_parse(struct rex_node **rxnn, uchar *p, 
		     struct rex_parse_aux *aux)
{
	abort_unless(aux && aux->end && aux->initial && aux->final && aux->mm);

	if ( p == aux->end ) {
		if ( aux->gstack != NULL )
			return rex_parse_error(rxnn, aux, p);
		*rxnn = &aux->final->base;
		finalize_choices(aux->initial, aux->final, aux);
		return 0;
	}

	switch (*p) { 
	case '*': 
	case ']': 
	case '+': 
	case '?': 
	case '{': 
	case '}': 
		return rex_parse_error(rxnn, aux, p);
	case '^':
	case '$':
		return rex_parse_anchor(rxnn, p, aux);
	case '(':
		return rex_parse_group_s(rxnn, p, aux);
	case '[':
		return rex_parse_class(rxnn, p, aux);
	case ')':
		return rex_parse_group_e(rxnn, p, aux);
	case '.':
		return rex_parse_any(rxnn, p, aux);
	case '|':
		return rex_parse_choice(rxnn, p, aux);
	default:
		return rex_parse_str(rxnn, p, aux);
	}
}


static int all_paths_start(struct rex_node *rxn)
{
	switch (rxn->type) {
		case REX_T_BANCHOR:
			return 1;
		case REX_T_GROUP_S:
			return all_paths_start(rxn->next);
		case REX_T_CHOICE: {
			struct rex_choice *rc = (struct rex_choice *)rxn;
			return all_paths_start(rc->opt1) &&
						 all_paths_start(rc->opt2);
		} break;
		default:
			return 0;
	}
}


int rex_init(struct rex_pat *rxp, struct raw *pat, struct memmgr *mm,
	     int *error)
{
	struct rex_parse_aux aux;
	int rv;

	if ( !rxp || !pat || !pat->data || mm == NULL )
		return -1;

	rxp->start_anchor = 0;

	rxp->start.base.type = REX_T_GROUP_S;
	rxp->start.base.repmin = 1;
	rxp->start.base.repmax = 1;
	rxp->start.base.next = NULL;
	rxp->start.num = 0;
	rxp->start.other = &rxp->end;

	rxp->end.base.type = REX_T_GROUP_E;
	rxp->end.base.repmin = 1;
	rxp->end.base.repmax = 1;
	rxp->end.base.next = NULL;
	rxp->end.num = 0;
	rxp->end.other = &rxp->start;

	rxp->mm = mm;
	aux.mm = rxp->mm;
	aux.initial = &rxp->start;
	aux.final = &rxp->end;
	aux.start = (uchar *)pat->data;
	aux.end = (uchar *)pat->data + pat->len;
	aux.gstack = NULL;
	aux.lastch = NULL;
	aux.eptr = error;
	aux.gnum = 0;

	rv = rex_parse(&rxp->start.base.next, (uchar *)pat->data, &aux);
	if ( rv == 0 )
		rxp->start_anchor = all_paths_start(&rxp->start.base);
	else
		rex_free(rxp);
	return rv;
}


static void rex_free_help(struct rex_node *node, struct rex_node *end, 
		          struct memmgr *mm)
{
	if ( node == NULL || node == end ) {
		return;
	} else if ( node->type == REX_T_GROUP_S ) {
		struct rex_group *rg = (struct rex_group *)node;
		rex_free_help(node->next, (struct rex_node *)rg->other, mm);
		rex_free_help((struct rex_node *)rg->other, end, mm);
		if ( rg->num > 0 )
			mem_free(mm, node);
	} else if ( node->type == REX_T_CHOICE ) {
		struct rex_choice *rc = (struct rex_choice *)node;
		rex_free_help(rc->opt1, end, mm);
		rex_free_help(rc->opt2, end, mm);
		mem_free(mm, node);
	} else {
		rex_free_help(node->next, end, mm);
		mem_free(mm, node);
	}
}


void rex_free(struct rex_pat *rxp)
{
	abort_unless(rxp);
	rex_free_help(&rxp->start.base, &rxp->end.base, rxp->mm);
}


struct rex_match_aux {
	char *start;
	char *end;
	char *gstart;
	unsigned grep;
	struct rex_match_loc *match;
	unsigned nmatch;
	struct rex_match_aux *prev;
};


static int rex_match_rxn(struct rex_node *rxn, char *cur, unsigned rep,
		         struct rex_match_aux *aux);


static int rex_next(struct rex_node *rxn, char *cur, unsigned rep,
		    struct rex_match_aux *aux)
{
	int rv = REX_NOMATCH;
	if ( (rxn->repmax == REX_WILDCARD) || (rep < rxn->repmax) ) {
		rv = rex_match_rxn(rxn, cur, rep, aux);
		if ( rv != REX_NOMATCH )
			return rv;
	}
	if ( rep >= rxn->repmin )
		rv = rex_match_rxn(rxn->next, cur, 0, aux);
	return rv;
}


static int rex_match_string(struct rex_node *rxn, char *cur, unsigned rep,
		            struct rex_match_aux *aux)
{
	struct rex_node_str *rs = (struct rex_node_str *)rxn;
	if ( ((aux->end - cur) >= rs->len) &&
			 (memcmp(rs->str, cur, rs->len) == 0) ) {
		return rex_next(rxn, cur + rs->len, rep + 1, aux);
	} else if ( rxn->repmin == 0 && rep == 0 ) { 
		return rex_match_rxn(rxn->next, cur, 0, aux);
	} else { 
		return REX_NOMATCH;
	}
}


static int rex_match_class(struct rex_node *rxn, char *cur, unsigned rep,
		           struct rex_match_aux *aux)
{
	struct rex_ascii_class *rac = (struct rex_ascii_class *)rxn;
	int rv = REX_NOMATCH;
	int i = *((uchar *)cur);
	int j = i >> 3;
	i &= 0x7;
	if ( (rac->set[j] & (1 << i)) ) { 
		rv = rex_next(rxn, cur + 1, rep + 1, aux);
	} else if ( rxn->repmin == 0 && rep == 0 ) { 
		rv = rex_match_rxn(rxn->next, cur, 0, aux);
	}
	return rv;
}


static int rex_match_choice(struct rex_node *rxn, char *cur, unsigned rep,
		            struct rex_match_aux *aux)
{
	struct rex_choice *rc = (struct rex_choice *)rxn;
	int rv;
	abort_unless(!rep);
	rv = rex_match_rxn(rc->opt1, cur, 0, aux);
	if ( rv != REX_NOMATCH )
		return rv;
	else
		return rex_match_rxn(rc->opt2, cur, 0, aux);
}


static int rex_match_group_s(struct rex_node *rxn, char *cur, unsigned rep,
		             struct rex_match_aux *aux)
{
	struct rex_group *rg = (struct rex_group *)rxn;
	struct rex_match_aux aux2, *new_aux = aux;
	int rv;

	abort_unless(aux);
	if ( rep == 0 ) { 
		aux2 = *aux;
		aux2.gstart = cur;
		aux2.prev = aux;
		new_aux = &aux2;
	}
	new_aux->grep = rep + 1;

	rv = rex_match_rxn(rxn->next, cur, 0, new_aux);

	if ( rv != REX_ERROR ) {
		if ( rv == REX_NOMATCH && rxn->repmin == 0 && rep == 0 ) {
			/* we are doing the job of the end group here */
			if ( rg->num < aux->nmatch && 
					 !aux->match[rg->num].valid ) {
				aux->match[rg->num].valid = 1;
				aux->match[rg->num].start = cur - aux->start;
				aux->match[rg->num].len = 0;
			}
			rv = rex_match_rxn(rg->other->base.next, cur, 0, aux);
		}
	}

	return rv;
}


static int rex_match_group_e(struct rex_node *rxn, char *cur, unsigned rep,
		             struct rex_match_aux *aux)
{
	struct rex_group *rg = (struct rex_group *)rxn;
	int rv = REX_NOMATCH;
	rep = aux->grep;

	abort_unless(aux);
	if ( (rxn->repmax == REX_WILDCARD) || (rep < rxn->repmax) ) {
		rv = rex_match_group_s(&rg->other->base, cur, aux->grep, aux);
		aux->grep = rep;
		if ( rv != REX_NOMATCH )
			return rv;
	} 
	if ( rep >= rxn->repmin )
		rv = rex_match_rxn(rxn->next, cur, 0, aux->prev);
	if ( rv == REX_MATCH && rg->num < aux->nmatch && 
			 !aux->match[rg->num].valid ) {
		aux->match[rg->num].valid = 1;
		aux->match[rg->num].start = aux->gstart - aux->start;
		aux->match[rg->num].len = cur - aux->gstart;
	}

	return rv;
}


static int rex_match_rxn(struct rex_node *rxn, char *cur, unsigned rep,
		         struct rex_match_aux *aux)
{
	int rv;
	if ( rxn == NULL )
		return REX_MATCH;

	/* handle end anchor */
	if ( cur == aux->end ) {
		if ( rxn->type == REX_T_EANCHOR )
			return rex_match_rxn(rxn->next, cur, 0, aux);
		else if ( rxn->type == REX_T_GROUP_E )
			return rex_match_group_e(rxn, cur, 0, aux);
		else
			return REX_NOMATCH;
	}

	switch( rxn->type ) { 
	case REX_T_STRING:
		rv = rex_match_string(rxn, cur, rep, aux);
		break;
	case REX_T_CLASS:
		rv = rex_match_class(rxn, cur, rep, aux);
		break;
	case REX_T_CHOICE:
		rv = rex_match_choice(rxn, cur, rep, aux);
		break;
	case REX_T_GROUP_S:
		rv = rex_match_group_s(rxn, cur, rep, aux);
		break;
	case REX_T_GROUP_E:
		rv = rex_match_group_e(rxn, cur, rep, aux);
		break;
	case REX_T_BANCHOR:
		if ( cur == aux->start )
			return rex_match_rxn(rxn->next, cur, 0, aux);
		else
			return REX_NOMATCH;
		break;
	case REX_T_EANCHOR:
		return REX_NOMATCH;
	default:
		return REX_ERROR;
	}

	return rv;
}


int rex_match(struct rex_pat *rxp, struct raw *str, struct rex_match_loc *m, 
	      uint nm)
{
	struct rex_match_aux aux;
	char *cur;
	int rv;

	if ( rxp == NULL || str == NULL || str->data == NULL )
		return REX_ERROR;
	if ( rxp->start.base.repmin != 1 || rxp->start.base.repmax != 1 ||
	     rxp->end.base.repmin != 1 || rxp->end.base.repmax != 1 )
		return REX_ERROR;
	if ( nm > 0 && m == 0 )
		return REX_ERROR;

	aux.start = str->data;
	aux.end = str->data + str->len;
	aux.match = m;
	aux.nmatch = nm;
	memset(m, 0, sizeof(*m) * nm);

	for ( cur = aux.start; cur < aux.end; ++cur ) { 
		aux.gstart = cur;
		rv = rex_match_rxn(&rxp->start.base, cur, 0, &aux);
		if ( rv != REX_NOMATCH )
			return rv;
		/* if all paths start with a start anchor don't search */
		/* the entire string: stop and return here */
		if ( rxp->start_anchor )
			return REX_NOMATCH;
	}

	return REX_NOMATCH;
}
