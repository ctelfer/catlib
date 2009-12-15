#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/grow.h>
#include <cat/hash.h>
#include <cat/list.h>
#include <cat/err.h>
#include <sys/time.h>
#include <cat/stduse.h>
#include <cat/cds.h>
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
struct htab *Word_tbl;
struct raw Strings = { 0, NULL };


void add(int prefixes[NPREF], int word)
{
	struct hnode *node;
	struct clist *list;
	unsigned hash;
	struct raw key;

	key.data = (byte_t*)prefixes;
	key.len  = sizeof(int) * NPREF;
	node = ht_lkup(Prefix_tbl, &key, &hash);
	if ( ! node ) { 
		list = clist_new_list(&estdmm, sizeof(int));
		clist_enqueue(list, &word);
		ht_put(Prefix_tbl, &key, list);
	} else {
		clist_enqueue(node->data, &word);
	}
	memmove(prefixes, prefixes + 1, (NPREF-1) * sizeof(int));
	prefixes[NPREF-1] = word;
}



void generate(void)
{
	int i, j, n, word;
	struct hnode *node;
	struct clist *list;
	struct clist_node *t, *h;
	int prefixes[NPREF];
	struct raw key;

	key.data = (byte_t*)prefixes;
	key.len  = sizeof(prefixes);
	for ( i = 0 ; i < NPREF ; ++i )
		prefixes[i] = i * sizeof(NONWORD);
	for ( i = 0 ; i < MAXGEN ; ++i ) {
		list = ht_get_dptr(Prefix_tbl, &key);
		h = cl_first(list);
		for ( n = 2, t = cln_next(h) ; t != cl_end(list) ; 
		      t = cln_next(t), ++n )
			if ( (random() % n) == 0 )
				h = t;
		word = cln_data(h, int);
		if ( word == END )
			return;
		memmove(prefixes, prefixes + 1, (NPREF-1) * sizeof(int));
		prefixes[NPREF-1] = word;
		printf("%s\n", STR(word));
	}
}



int main(int argc, char **argv)
{
	int i, l, idx;
	unsigned long cur;
	char fmt[32];
	struct timeval tv;
	int prefixes[NPREF];

	gettimeofday(&tv, NULL);
	srandom(tv.tv_usec);
	Prefix_tbl = ht_new(&estdmm, HTSIZ, CAT_KT_RAW, 0, 0);
	Word_tbl = ht_new(&estdmm, HTSIZ, CAT_KT_STR, 0, 0);
	if (grow(&Strings.data, &Strings.len, 
		 NPREF * sizeof(NONWORD) + MAXWORD) < 0)
		errsys("Out of memory\n");
	for ( i = 0 ; i < NPREF ; ++i ) {
		prefixes[i] = sizeof(NONWORD) * i;
		sprintf(STR(sizeof(NONWORD) * i), NONWORD);
	}
	cur = NPREF * sizeof(NONWORD);
	sprintf(fmt, "%%%ds", MAXWORD - 1);

	while ( scanf(fmt, STR(cur)) > 0 ) { 
		if ( (idx = ptr2int(ht_get_dptr(Word_tbl, STR(cur)))) == 0 ) {
			ht_put(Word_tbl, STR(cur), int2ptr(cur));
			l = strlen(STR(cur)) + 1;
			if (grow(&Strings.data,&Strings.len,cur+l+MAXWORD) < 0)
				errsys("Out of memory\n");
			idx = cur;
			cur += l;
		}
		add(prefixes, idx);
	}
	add(prefixes, END);
	ht_free(Word_tbl);

	printf("Generating...\n");
	generate();
	return 0;
}
