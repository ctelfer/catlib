#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cat/cat.h>
#include <cat/match.h>
#include <cat/err.h>
#include <cat/raw.h>
#include <cat/stduse.h>

#define I2P(x)	((void *)(ptrdiff_t)x)

void usage(void)
{
	err("usage: testmatch (-k|-b|-s) <string> <pattern>\n");
}


void dokmp(struct raw *str, struct raw *pat)
{
	struct kmppat *kmp;
	unsigned long loc;

	kmp = kmp_pnew(pat);
	if (kmp_match(str, kmp, &loc))
		printf("Found %s at position %u in %s\n", pat->data,
		       (uint)loc, str->data);
	else
		printf("%s not found in %s\n", pat->data, str->data);
	free(kmp);
}


void dobm(struct raw *str, struct raw *pat)
{
	struct bmpat *bmp;
	unsigned long loc;

	bmp = bm_pnew(pat);
	if (bm_match(str, bmp, &loc))
		printf("Found %s at position %u in %s\n", pat->data,
		       (uint)loc, str->data);
	else
		printf("%s not found in %s\n", pat->data, str->data);
	free(bmp);
}


struct gatherctx {
	struct sfxtree *sfx;
	struct htab *edges;
	struct htab *nodes;
};


static int nedges = 0, nnodes = 0;


void gather(void *edgep, void *gatherp)
{
	struct sfxedge *edge = edgep;
	struct gatherctx *gctx = gatherp;
	++nedges;
	if ( !ht_get(gctx->edges, edge) )
		ht_put(gctx->edges, edge, I2P(nedges));
	if ( !ht_get(gctx->nodes, edge->hentry.data) ) {
		++nnodes;
		ht_put(gctx->nodes, edge->hentry.data, I2P(nnodes));
	}
}


void printedges(void *edgep, void *gatherp)
{
	struct sfxedge *edge = edgep;
	struct gatherctx *gctx = gatherp;
	struct sfxedgekey *ek = edge->hentry.key;
	int src, dst, edgenum;
	char ch;
	char str[50];

	src = (int)(ptrdiff_t)ht_get(gctx->nodes, ek->node);
	dst = (int)(ptrdiff_t)ht_get(gctx->nodes, edge->hentry.data);
	edgenum = (int)(ptrdiff_t)ht_get(gctx->edges, edge);
	ch = ek->character;
	if ( ch == '\0' )
		ch = '@';

	if ( edge->end - edge->start + 1 > sizeof(str) )
		str[0] = '\0';
	else {
		memcpy(str, gctx->sfx->str.data + edge->start,
		       edge->end - edge->start + 1);
		str[edge->end - edge->start + 1] = '\0';
	}
	
	printf("Edge %3d: %3d (%c:%3ld,%3ld) -> %3d: %s\n", edgenum, src, ch,
		edge->start, edge->end, dst, str);
}


void printsfx(struct sfxtree *sfx)
{
	struct gatherctx gctx;
	int i;
	char ch;

	gctx.sfx = sfx;
	gctx.edges = ht_new(100, CAT_DT_PTR);
	gctx.nodes = ht_new(100, CAT_DT_PTR);
	ht_put(gctx.nodes, &sfx->root, I2P(1));
	nnodes = 1;
	ht_apply(&sfx->edges, gather, &gctx);
	for ( i = 0 ; i < sfx->str.len ; ++i )
		printf("%2d ", i);
	printf("\n");
	for ( i = 0 ; i < sfx->str.len ; ++i ) {
		ch = sfx->str.data[i];
		if ( !isprint(ch) )
			ch = '@';
		printf(" %c ", ch);
	}
	printf("\n");
	ht_apply(&sfx->edges, printedges, &gctx);
	ht_free(gctx.edges);
	ht_free(gctx.nodes);
}


void dosuffix(struct raw *str, struct raw *pat)
{
	struct sfxtree *sfx;
	unsigned long loc;

	sfx = sfx_new(str);
	if (sfx_match(sfx, pat, &loc))
		printf("Found %s at position %u in %s\n", pat->data,
		       (uint)loc, str->data);
	else
		printf("%s not found in %s\n", pat->data, str->data);
	/* printsfx(sfx); */
	sfx_clear(sfx);
	free(sfx);
}


int main(int argc, char *argv[])
{
	struct raw str, pat;

	if (argc < 4 || argv[1][0] != '-' )
		usage();

	str_to_raw(&str, argv[2], 0);
	str_to_raw(&pat, argv[3], 0);

	switch (argv[1][1]) {
	case 'k':
		dokmp(&str, &pat);
		break;
	case 'b':
		dobm(&str, &pat);
		break;
	case 's':
		dosuffix(&str, &pat);
		break;
	default:
		usage();
	}

	return 0;
}
