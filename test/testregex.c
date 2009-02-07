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

	if ( rex_init(&rp, str_to_raw(&r, pat, 0), &estdmem, &e) < 0 ) {
		printf("Error parsing pattern '%s' at position %d\n", e+1);
		return;
	}

	memset(locs, 0, sizeof(locs));
	rv = rex_match(&rp, str_to_raw(&r, str, 0), locs, 16);
	if ( rv == REX_MATCH ) { 
		printf("MATCH: /%s/ (@ %u:%u) -- \"%s\"\n", pat, 
			locs[0].start, locs[0].len, str);
		for ( i = 1; i < 16; i++ )
			if ( locs[i].valid )
				printf("Sub-group %d match: @%u:%u\n", 
					locs[i].start, locs[i].len);
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
	return 0;
}
