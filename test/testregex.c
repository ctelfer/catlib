/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
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


void tfail(char *pat)
{
	struct rex_pat rp;
	struct raw r;
	int e;

	if ( rex_init(&rp, str_to_raw(&r, pat, 0), &estdmm, &e) < 0 )
		printf("Pattern /%s/ correctly failed at position %d.\n", 
		       pat, e+1);
	else
		printf("ERROR: /%s/ -- should have failed to compile.\n", pat);
}


int main(int argc, char *argv[])
{
	tmatch("a", "bad");
	tmatch("a", "good");
	tmatch("z?", "bad");
	tmatch("a?", "bad");
	tmatch("a+", "bad");
	tmatch("a+", "baaaad");
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
	tmatch("^", "");
	tmatch("$", "");
	tmatch("^$", "");
	tmatch("a?", "");
	tmatch("a*", "");
	tmatch("a+", "");
	tmatch("a{0,3}", "");
	tmatch("a{1,3}", "");
	tmatch("a{1,3}", "aa");
	tmatch("a{1,3}", "baaaa");
	tmatch("a{2,2}", "baaaa");
	tmatch("a{5}", "aaaa");
	tmatch("a{5}", "baaaa");
	tmatch("a{5}", "aaaaa");
	tmatch("a{5}", "bababaaaaabbbb");
	tmatch("a{5}", "bababaaaabbbb");
	tmatch("(abc){1,2}", "bababcabb");
	tmatch("(abc){1,2}", "bababcabc");
	tmatch("(abc){2}", "bababcabc");
	tmatch("(abc){2}", "bababcabb");
	tmatch("(abc|cba){2}", "bababccbaxx");
	tmatch("(abc|cba){2}", "bababcccaxx");
	tmatch("[a-zA-Z_0-9]{1,10}", "bababcccaxx");
	tmatch("[^a-zA-Z_0-9]{1,10}", "bababcccaxx");
	tmatch("[a-zA-Z_0-9]{1,10}", "bab9abcccaxx");
	tmatch("[^a-zA-Z_0-9]{1,10}", "bab9abcccaxx");
	tmatch("[a-zA-Z_0-9]{1,}", "999");
	tmatch("[a-zA-Z_0-9]{1,}", "bababcccaxx");
	tmatch("[a-zA-Z_0-9]{,5}", "999");
	tmatch("[a-zA-Z_0-9]{,5}", "999a");

	tfail("(abc");
	tfail("abc)");
	tfail("(abc)(*");
	tfail("(a(bc)");
	tfail("(a(b*c)");
	tfail("(a(bc)))");
	tfail("(a(bc*)))");
	tfail("[abc");
	tfail("abc]");
	tfail("*");
	tfail("*abc");
	tfail("?abc");
	tfail("+abc");
	tfail("abc??");
	tfail("abc**");
	tfail("abc++");
	tfail("[a-zA-Z_0-9]{1,0}");
	tfail("[a-zA-Z_0-9]{5,2}");
	tfail("a{1000}");
	tfail("a{1000,1000}");
	tfail("a{,1000}");

	return 0;
}
