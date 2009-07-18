#include <cat/sort.h>
#include <cat/time.h>
#include <cat/aux.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define ASIZE	256 */
#define ASIZE	10000
int arr1[ASIZE], arr2[ASIZE], all[ASIZE];
unsigned long long n_intcmps;


static unsigned urand(unsigned mod) {
	return (unsigned)rand() % mod;
}


int intcmp(const void *i1, const void *i2)
{
	++n_intcmps;
	return *(int *)i1 - *(int *)i2;
}

void test_intarr()
{
	int i;

	for ( i = 1 ; i <= ASIZE ; ++i )
		all[i] = i;
	for ( i = 0; i < ASIZE; ++i ) {
#if DEBUGGING
		arr1[i] = all[urand(ASIZE)];
#else
		arr1[i] = rand();
#endif
	}

	/* insertion sort */
	n_intcmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	isort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for insertion sort\n", i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("insertion sort succeeded with %llu comparisons\n", 
		       n_intcmps);

	/* selection sort */
	n_intcmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	ssort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for selection sort\n", i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("selection sort succeeded with %llu comparisons\n", 
			n_intcmps);

	/* heap sort */
	n_intcmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	hsort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for heap sort\n", i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("heap sort succeeded with %llu comparisons \n",
		       n_intcmps);

	/* quick sort */
	n_intcmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	qsort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for quick sort\n", i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("quick sort succeeded with %llu comparisons\n", 
			n_intcmps);

	/* quick sort 2: array already sorted */
	n_intcmps = 0;
	qsort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for quick sort (sorted)\n",
			       i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("quick sort 2 succeeded with %llu comparisons\n", 
		       n_intcmps);
}


unsigned long long n_scmps;
int string_compare(const void *s1, const void *s2) { 
	++n_scmps;
	return strcmp(*(const char **)s1, *(const char **)s2);
}

int string_compare2(const void *s1, const void *s2) { 
	++n_scmps;
	return strcmp(s1, s2);
}

#define SARRLEN		10000
char sorig[SARRLEN][80];
char *sarr1[SARRLEN];
char sarr2[SARRLEN][80];

void test_strarr()
{
	int i, j;
	unsigned long long n;

	for ( i = 0; i < SARRLEN; i++ ) {
		int slen = urand(74) + 5;
		for ( j = 0; j < slen; ++j )
			sorig[i][j] = urand(26) + 'a';
		sorig[i][j] = '\0';
	}

	for ( i = 0; i < SARRLEN; i++ )
		sarr1[i] = sorig[i];
	n_scmps = 0;
	qsort_array(sarr1, SARRLEN, sizeof(sarr1[0]), &string_compare);
	n = n_scmps;
	for ( i = 1; i < SARRLEN; ++i )
		if ( strcmp(sarr1[i-1], sarr1[i]) > 0 ) {
			printf("qsort1 error at position %d\n", i);
			printf("'%s' before '%s'\n", sarr1[i-1], sarr1[i]);
			break;
		}
	if ( i == SARRLEN )
		printf("qsort1 for strings succeeded w/%llu comparisons\n",
		       n);


	memcpy(sarr2, sorig, sizeof(sorig));
	n_scmps = 0;
	qsort_array(sarr2, SARRLEN, sizeof(sarr2[0]), &string_compare2);
	n = n_scmps;
	for ( i = 1; i < SARRLEN; ++i )
		if ( strcmp(sarr2[i-1], sarr2[i]) > 0 ) {
			printf("qsort2 error at position %d\n", i);
			printf("'%s' before '%s'\n", sarr2[i-1], sarr2[i]);
			break;
		}
	if ( i == SARRLEN )
		printf("qsort2 for strings succeeded w/%llu comparisons\n",
		       n);

	n_scmps = 0;
	qsort_array(sarr2, SARRLEN, sizeof(sarr2[0]), &string_compare2);
	n = n_scmps;
	for ( i = 1; i < SARRLEN; ++i )
		if ( strcmp(sarr2[i-1], sarr2[i]) > 0 ) {
			printf("qsort3 error at position %d\n", i);
			printf("'%s' before '%s'\n", sarr2[i-1], sarr2[i]);
			break;
		}
	if ( i == SARRLEN )
		printf("qsort3 for strings succeeded w/%llu comparisons\n",
		       n);
}


#define SPEEDLEN	10000
#define NTRIES		100

byte_t spd_dst[SPEEDLEN];
void *spd_arr1[SPEEDLEN];
void *spd_arr2[SPEEDLEN];


unsigned long long spd_ncmps;
int ptr_cmp(const void *p1, const void *p2)
{
	++spd_ncmps;
	return *(const void **)p1 - *(const void **)p2;
}


void test_speed()
{
	int i, j;
	struct timeval start, end;
	for ( i = 0; i < SPEEDLEN; ++i )
		spd_arr1[i] = &spd_dst[urand(SPEEDLEN)];
	spd_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES ; ++j ) {
		memcpy(spd_arr2, spd_arr1, sizeof(spd_arr1));
		qsort_array(spd_arr2, SPEEDLEN, sizeof(void *), &ptr_cmp);
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for my sort: %lf\n",
	       NTRIES, SPEEDLEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("my sort used %llu comparisons\n", spd_ncmps);

	spd_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES ; ++j ) {
		memcpy(spd_arr2, spd_arr1, sizeof(spd_arr1));
		qsort(spd_arr2, SPEEDLEN, sizeof(void *), &ptr_cmp);
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for std qsort: %lf\n",
	       NTRIES, SPEEDLEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("standard qsort used %llu comparisons\n", spd_ncmps);
}


int main(int argc, char *argv[])
{
	size_t i;

	srand(0);
	printf("int sort run1\n"); 
	test_intarr();
	printf("\n\nrun2\n");
	srand(10);
	test_intarr();
	printf("\n\nstring run 1\n");
	test_strarr();
	printf("\n\nspeed test\n");
	test_speed();

	return 0;
}
