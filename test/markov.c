#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/ring.h>
#include <cat/hash.h>
#include <cat/list.h>
#include <cat/err.h>
#include <cat/stduse.h>
#include <cat/cds.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


CDS_NEWSTRUCT(struct list, char *, strent_s);

#define NPREF	2
#define NONWORD "\n"
#define END	"\t"
#define MAXGEN	10000
#define HTSIZ	4096
#define WORDLEN	79
#define BUFLEN ((WORDLEN + 1) * NPREF + 1)

char *Prefixes[NPREF];
struct ring Buffer;
struct htab *Table;


void printall(struct htab *tab)
{
	struct list *l, *l3, *l2, *t;
	int i;
	struct hnode *n;

	for ( l = tab->tab, i=0 ; i < tab->size ; ++i, ++l ) {
		for ( l2 = l->next ; l2 != l ; l2 = l2->next ) {
			n = (struct hnode *)l2;
			l3 = n->data;
			printf("*%s* : *%s* ", (char *)n->key, CDS_DATA(l3, strent_s));
			for ( t = l3->next ; t != l3 ; t = t->next )
				printf(" *%s*", CDS_DATA(t, strent_s));
			printf("\n");
		}
	}
}


void add(char *word)
{
	struct hnode *node;
	struct list *ol;
	strent_s *w;
	char *s;
	int i;

	ring_reset(&Buffer);
	ring_fmt(&Buffer, "%s", Prefixes[0]);
	for ( i = 1 ; i < NPREF ; ++i )
		ring_fmt(&Buffer, " %s", Prefixes[i]);
	s = estrdup(word);
	abort_unless(s != NULL);
	CDS_NEW(w, s);
	ol = ht_get(Table, Buffer.data);
	if ( ! ol )
		ht_put(Table, Buffer.data, CDS_NPTR(w, strent_s));
	else
		l_ins(ol->prev, CDS_NPTR(w, strent_s));
	memmove(Prefixes, Prefixes + 1, (NPREF-1) * sizeof(char *));
	Prefixes[NPREF-1] = s;
}



void generate(void)
{
	int i, j, n;
	struct hnode *node;
	struct list *s, *t, *h;
	char *word;

	for ( i = 0 ; i < MAXGEN ; ++i ) {
		ring_reset(&Buffer);
		ring_fmt(&Buffer, "%s", Prefixes[0]);
		for ( j = 1 ; j < NPREF ; ++j )
			ring_fmt(&Buffer, " %s", Prefixes[j]);
		h = s = ht_get(Table, Buffer.data);
		n = 1;
		for ( t = s->next ; t != s ; t = t->next )
			if ( (random() % ++n) == 0 )
				h = t;

		word = CDS_DATA(h, strent_s);
		if ( strcmp(word, END) == 0 ) 
			return;
		memmove(Prefixes, Prefixes + 1, (NPREF-1) * sizeof(char *));
		Prefixes[NPREF-1] = word;
		printf("%s\n", word);
	}
}



int main(int argc, char *argv)
{
	int i;
	char word[WORDLEN+1], fmt[10], *cur = NULL;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);

	for ( i = 0 ; i < NPREF ; ++i ) Prefixes[i] = NONWORD;
	Table = ht_new(HTSIZ, CAT_DT_STR);
	ring_init(&Buffer, emalloc(BUFLEN), BUFLEN);

	sprintf(fmt, "%%%ds", (int)sizeof(word)-1);
	while ( scanf(fmt, word) > 0 )
		add(word);
	add(END);
	/* printall(Table); */

	for ( i = 0 ; i < NPREF ; ++i ) Prefixes[i] = NONWORD;
	generate();
	return 0;
}
