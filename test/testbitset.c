#include <stdio.h>
#include <sys/time.h>
#include <cat/stduse.h>

#define S0LEN	1024
#define NT 100000000

int main(int argc, char *argv[])
{
	struct timeval tv, tv2;
	double usec;
	struct safebitset *set1, *set2;
	unsigned i, bit;
	DECLARE_BITSET(set0, S0LEN);

	bset_fill(set0, S0LEN);
	bset_clr(set0, 100);
	if ( bset_test(set0, 100) )
		logrec(0, "Should not have found 100 in set0\n");
	bset_clr(set0, 2);
	bset_clr(set0, 198);
	bset_set_to(set0, 100, 1);
	if ( !bset_test(set0, 3) )
		logrec(0, "Should have found 3 in set0\n");
	if ( bset_test(set0, 2) )
		logrec(0, "Should not have found 2 in set0\n");
	if ( !bset_test(set0, 190) )
		logrec(0, "Should have found 190 in set0\n");
	if ( bset_test(set0, 198) )
		logrec(0, "Should not have found 198 in set0\n");
	if ( !bset_test(set0, 100) )
		logrec(0, "Should have found 100 in set0\n");

	set1 = sbs_new(&estdmm, 5);
	set2 = sbs_new(&estdmm, 378);

	sbs_fill(set2);
	sbs_set(set1, 3);
	sbs_set(set1, 2);

	if ( sbs_test(set1, 4) )
		logrec(0, "Should not have found 4 in set1\n");
	if ( !sbs_test(set1, 3) )
		logrec(0, "Should have found 3 in set1\n");

	sbs_clr(set2, 3);
	printf("Copied %u bits (should be %u)\n", (uint)sbs_copy(set1, set2),
	       (uint)set1->nbits);
	sbs_set_to(set1, 1, 0);
	sbs_set_to(set1, 0, 1);
	if ( bset_test(set1->set, 5) )
		logrec(0, "Should not have found 5 in set1 after copy\n");
	if ( sbs_test(set1, 3) )
		logrec(0, "Should not have found 3 in set1 after copy\n");
	if ( !sbs_test(set1, 4) )
		logrec(0, "Should have found 4 in set1 after copy\n");
	if ( sbs_test(set1, 1) )
		logrec(0, "Should not have found 1 in set1 after copy\n");
	if ( !sbs_test(set1, 0) )
		logrec(0, "Should have found 0 in set1 after copy\n");

	gettimeofday(&tv, 0);
	for ( i = 0 ; i < NT ; ++i )
		sbs_set(set2, i & 0xff);
	gettimeofday(&tv2, 0);
	usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
	usec /= NT;
	printf("Roughly %f nanoseconds for sbs_set()\n", usec * 1000);

	gettimeofday(&tv, 0);
	for ( i = 0 ; i < NT ; ++i )
		sbs_clr(set2, i & 0xff);
	gettimeofday(&tv2, 0);
	usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
	usec /= NT;
	printf("Roughly %f nanoseconds for sbs_clr()\n", usec * 1000);

	gettimeofday(&tv, 0);
	for ( i = 0 ; i < NT ; ++i )
		sbs_set_to(set2, i & 0xff, i & 1);
	gettimeofday(&tv2, 0);
	usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
	usec /= NT;
	printf("Roughly %f nanoseconds for sbs_set_to()\n", usec * 1000);

	gettimeofday(&tv, 0);
	for ( i = 0 ; i < NT ; ++i )
		sbs_test(set2, i & 0xff);
	gettimeofday(&tv2, 0);
	usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
	usec /= NT;
	printf("Roughly %f nanoseconds for sbs_test()\n", usec * 1000);


	sbs_free(set2);
	sbs_free(set1);


	gettimeofday(&tv, 0);
	for ( i = 0 ; i < NT ; ++i ) {
		bit = i & (S0LEN - 1);
		if ( bset_test(set0, bit) )
			bset_clr(set0, bit);
		else
			bset_set(set0, bit);
	}
	gettimeofday(&tv2, 0);
	usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
	usec /= NT;
	printf("Roughly %f nanoseconds for bset_*()-based toggle\n", usec * 1000);


	printf("Tests completed\n");
	return 0;
}
