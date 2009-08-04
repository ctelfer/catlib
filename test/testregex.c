#include <stdio.h>
#include <string.h>
#include <cat/raw.h>
#include <cat/stduse.h>
#include <cat/match.h>
#include <cat/mem.h>

struct rex_match_loc locs[16];


void tmatch(char *pat, char *str)
{
	struct rex_pat rp;
	struct raw r;
	int rv, e, i;

	if ( rex_init(&rp, str_to_raw(&r, pat, 0), &estdmm, &e) < 0 ) {
		printf("Error parsing pattern '%s' at position %d\n", pat, e+1);
		return;
	}

	memset(locs, 0, sizeof(locs));
	rv = rex_match(&rp, str_to_raw(&r, str, 0), locs, 16);
	if ( rv == REX_MATCH ) { 
		printf("MATCH: /%s/ (@ %u:%u) -- \"%s\"\n", pat, 
			(uint)locs[0].start, (uint)locs[0].len, str);
		for ( i = 1; i < 16; i++ )
			if ( locs[i].valid )
				printf("Sub-group %d match: @%u:%u\n", i,
				       (uint)locs[i].start, (uint)locs[i].len);
	} else if ( rv == REX_NOMATCH ) { 
		printf("NO MATCH: /%s/ -- \"%s\"\n", pat, str);
	} else if ( rv == REX_ERROR ) {
		printf("ERROR: /%s/ -- \"%s\"\n", pat, str);
	} else { 
		printf("Unknown return code! (%d)\n", rv);
	}

	rex_free(&rp);
}

int main(int argc, char *argv[])
{
	tmatch("a", "bad");
	tmatch("a", "good");
	tmatch("z?", "bad");
	tmatch("a?", "bad");
	tmatch("a+", "bad");
	tmatch("a+", "bonk");
	tmatch("a*", "bonk");
	tmatch(".", "bonk");
	tmatch(".*", "bonk");
	tmatch(".+", "bonk");
	tmatch("[abc]", "c");
	tmatch("[abc]", "9");
	tmatch("[a-z]", "b");
	tmatch("[a-z]", "9");
	tmatch("[a-zA-Z]", "K");
	tmatch("[a-zA-Z]", ",");
	tmatch("[^a-zA-Z]", ",");
	tmatch("[^a-zA-Z]", "q");
	tmatch("9[a-z]?", "9q");
	tmatch("9[a-z]?", "99");
	tmatch("[]]", "a]b");
	tmatch("[^]]", "]b");
	tmatch("[a-z]*", "  hello  ");
	tmatch("[a-z]+", "  hello  ");
	tmatch("(ab)", " caby");
	tmatch("(a(b)*)", " cabbby");
	tmatch("(a(b)?)", " cabbby");
	tmatch("(a(b)?)", " cacbbby");
	tmatch("(a(b)+)", " cay");
	tmatch("(a(b)+)", " caby");
	tmatch("(a(b)+)", " cabbby");
	tmatch("a|b", "a");
	tmatch("a|b", "b");
	tmatch("a|b", "cad");
	tmatch("a|b", "cbd");
	tmatch("(a|b)", "cbd");
	tmatch("(a|b)+", "cbabbd");
	tmatch("(a|bb)+", "cbabbbd");
	tmatch("^abc", "abcd");
	tmatch("^abc", "aaabcd");
	tmatch("abc$", "abcd");
	tmatch("bcd$", "aaabcd");
	return 0;
}
