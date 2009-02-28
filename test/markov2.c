#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/grow.h>
#include <cat/hash.h>
#include <cat/list.h>
#include <cat/err.h>
#include <sys/time.h>
#include <cat/stduse.h>
#include <stdio.h>
#include <string.h>

#define NPREF	2
#define NONWORD "\n"
#define END	-1
#define MAXGEN	10000
#define HTSIZ	4096
#define MAXWORD 80

#define STR(x) ((char *)&Strings.data[x])
struct htab *Prefix_tbl;
struct raw Strings = { 0, NULL };


void add(int prefixes[NPREF], int word)
{
	struct hnode *node;
	struct list *l, *l2;
	unsigned hash;
	struct raw key;

	key.data = (byte_t*)prefixes;
	key.len  = sizeof(int) * NPREF;
	node = ht_lkup(Prefix_tbl, &key, &hash);
	if ( ! node ) { 
		clist_enq(l = clist_newlist(), int, word);
		ht_ins(Prefix_tbl, ht_nnew(&Prefix_tbl->sys, &key, l, hash));
	} else {
		clist_enq((struct list *)node->data, int, word);
	}
	memmove(prefixes, prefixes + 1, (NPREF-1) * sizeof(int));
	prefixes[NPREF-1] = word;
}



void generate(void)
{
	int i, j, n, word;
	struct hnode *node;
	struct list *s, *t, *h;
	int prefixes[NPREF];
	struct raw key;

	key.data = (byte_t*)prefixes;
	key.len  = sizeof(prefixes);
	for ( i = 0 ; i < NPREF ; ++i )
		prefixes[i] = i * sizeof(NONWORD);
	for ( i = 0 ; i < MAXGEN ; ++i ) {
		s = ht_get(Prefix_tbl, &key);
		h = l_head(s);
		for ( n = 2, t = h->next ; t != l_end(s) ; t = t->next, ++n )
			if ( (random() % n) == 0 )
				h = t;
		word = clist_data(h, int);
		if ( word == END )
			return;
		memmove(prefixes, prefixes + 1, (NPREF-1) * sizeof(int));
		prefixes[NPREF-1] = word;
		printf("%s\n", STR(word));
	}
}



int main(int argc, char *argv)
{
	int i, l;
	unsigned long cur;
	char fmt[32];
	struct timeval tv;
	int prefixes[NPREF];

	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);
	Prefix_tbl = ht_new(HTSIZ, CAT_DT_RAW);
	if (grow((char *)&Strings.data, &Strings.len, 
		 NPREF * sizeof(NONWORD) + MAXWORD) < 0)
		errsys("Out of memory\n");
	for ( i = 0 ; i < NPREF ; ++i ) {
		prefixes[i] = sizeof(NONWORD) * i;
		sprintf(STR(sizeof(NONWORD) * i), NONWORD);
	}
	cur = NPREF * sizeof(NONWORD);
	sprintf(fmt, "%%%ds", MAXWORD - 1);

	while ( scanf(fmt, STR(cur)) > 0 ) { 
		l = strlen(STR(cur)) + 1;
		if (grow((char*)&Strings.data, &Strings.len, cur+l+MAXWORD) < 0)
			errsys("Out of memory\n");
		add(prefixes, cur);
		cur += l;
	}
	add(prefixes, END);

	printf("Generating...\n");
	generate();
	return 0;
}
