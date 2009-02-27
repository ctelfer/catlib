#include <cat/sort.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define ASIZE	256 */
#define ASIZE	12345
int arr1[ASIZE], arr2[ASIZE], all[ASIZE];
unsigned long long n_intcmps;


static unsigned urand(unsigned mod) {
	return (unsigned)rand() % mod;
}


int intcmp(void *i1, void *i2)
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
int string_compare(void *s1, void *s2) { 
	++n_scmps;
	return strcmp(*(char **)s1, *(char **)s2);
}

int string_compare2(void *s1, void *s2) { 
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

	return 0;
}
