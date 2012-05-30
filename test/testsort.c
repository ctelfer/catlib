/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
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
		printf("qsort3 for strings succeeded w/%llu comparisons\n", n);
}


#define SPEEDLEN	100000
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
	printf("Time taken for %u sorts of %u elements for my sort: %f\n",
	       NTRIES, SPEEDLEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("my sort used %llu comparisons\n", spd_ncmps);

	/* make sure spd_arr2 is sorted */
	for ( i = 1; i < SPEEDLEN; ++i ) {
		if ( spd_arr2[i-1] > spd_arr2[i] ) {
			printf("speed  qsort error at position %d:\n", i-1);
			break;
		}
	}

	spd_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES ; ++j ) {
		memcpy(spd_arr2, spd_arr1, sizeof(spd_arr1));
		qsort(spd_arr2, SPEEDLEN, sizeof(void *), &ptr_cmp);
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for std qsort: %f\n",
	       NTRIES, SPEEDLEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("standard qsort used %llu comparisons\n", spd_ncmps);
}


#define SPEED2LEN	100000
#define NTRIES2		100
#define SPEED2PLEN	256

struct speed2_elem {
	int	key;
	char	pad[SPEED2PLEN];
};

struct speed2_elem spd2_arr1[SPEED2LEN];
struct speed2_elem spd2_arr2[SPEED2LEN];


unsigned long long spd2_ncmps;
int spd2_cmp(const void *i1, const void *i2)
{
	++spd2_ncmps;
	return *(int *)i1 - *(int *)i2;
}


void test_speed2()
{
	int i, j;
	struct timeval start, end;

	for ( i = 0; i < SPEED2LEN; ++i ) {
		spd2_arr1[i].key = rand();
		for ( j = 0 ; j < SPEED2PLEN ; ++j )
			spd2_arr1[i].pad[j] = rand();
	}

	spd2_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES2 ; ++j ) {
		memcpy(spd2_arr2, spd2_arr1, sizeof(spd2_arr1));
		qsort_array(spd2_arr2, SPEED2LEN, 
		            sizeof(struct speed2_elem), &spd2_cmp);
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for my sort: %f\n",
	       NTRIES2, SPEED2LEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("my sort used %llu comparisons\n", spd2_ncmps);

	/* make sure spd2_arr2 is sorted */
	for ( i = 1; i < SPEED2LEN; ++i ) {
		if ( spd2_arr2[i-1].key > spd2_arr2[i].key ) {
			printf("speed 2 qsort error at position %d: "
			       "%d vs %d\n", i-1, spd2_arr2[i-1].key, 
			       spd2_arr2[i].key);
			break;
		}
	}

	spd2_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES2 ; ++j ) {
		memcpy(spd2_arr2, spd2_arr1, sizeof(spd2_arr1));
		qsort(spd2_arr2, SPEED2LEN, sizeof(struct speed2_elem),
		      &spd2_cmp);
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for std qsort: %f\n",
	       NTRIES2, SPEED2LEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("standard qsort used %llu comparisons\n", spd2_ncmps);
}


void *spd3_arr[SPEED2LEN];
void *spd3_save[SPEED2LEN];
int spd3_uniq[SPEED2LEN];
unsigned long long spd3_ncmps;
struct speed2_elem t3elem;
int spd3_cmp(const void *i1, const void *i2)
{
	++spd3_ncmps;


	return ((struct speed2_elem *)(*(void **)i1))->key - 
	       ((struct speed2_elem *)(*(void **)i2))->key;
}


void test_speed3()
{
	int i, j;
	size_t idx;
	struct timeval start, end;

	/* verify that array_to_voidp works */
	for ( i = 0; i < SPEED2LEN; ++i )
		spd2_arr1[i].key = i;
	array_to_voidp(spd3_save, spd2_arr1, SPEED2LEN, 
		       sizeof(struct speed2_elem));
	for ( i = 0; i < SPEED2LEN; ++i ) {
		int k;
		if ( (k = ((struct speed2_elem *)spd3_save[i])->key) != i ) {
			printf("speed 3 qsort error in array_to_voidp at "
			       "%d: %d\n", i, k);
			break;
		}
	}

	for ( i = 0; i < SPEED2LEN; ++i ) {
		spd2_arr1[i].key = rand();
		for ( j = 0 ; j < SPEED2PLEN ; ++j )
			spd2_arr1[i].pad[j] = rand();
	}


	spd3_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES2 ; ++j ) {
		memcpy(spd2_arr2, spd2_arr1, sizeof(spd2_arr1));
		array_to_voidp(spd3_arr, spd2_arr2, SPEED2LEN, 
			       sizeof(struct speed2_elem));
		qsort_array(spd3_arr, SPEED2LEN, sizeof(void *), &spd3_cmp);
		if ( j == NTRIES2 - 1 )
			memcpy(spd3_save, spd3_arr, sizeof(spd3_arr));
		permute_array(spd2_arr2, &t3elem, spd3_arr, SPEED2LEN, 
			      sizeof(struct speed2_elem));
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for my sort: %f\n",
	       NTRIES2, SPEED2LEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("my sort used %llu comparisons\n", spd3_ncmps);

	/* make sure spd3_arr is a true permutation */
	memset(spd3_uniq, 0, sizeof(spd3_uniq));
	for ( i = 0 ; i < SPEED2LEN; ++i ) {
		idx = (struct speed2_elem *)spd3_save[i] - spd2_arr2;
		spd3_uniq[idx] = 1;
	}
	for ( i = 0 ; i < SPEED2LEN; ++i ) {
		if ( spd3_uniq[i] == 0 ) {
			printf("Element %d is missing in the permuted array!\n", i);
			break;
		}
	}

	/* make sure spd2_arr2 is sorted */
	for ( i = 1; i < SPEED2LEN; ++i ) {
		if ( spd2_arr2[i-1].key > spd2_arr2[i].key ) {
			printf("speed 3 qsort error at position %d: "
			       "%d vs %d\n", i-1, spd2_arr2[i-1].key, 
			       spd2_arr2[i].key);
			break;
		}
	}

	spd2_ncmps = 0;
	gettimeofday(&start, NULL);
	for ( j = 0 ; j < NTRIES2 ; ++j ) {
		memcpy(spd2_arr2, spd2_arr1, sizeof(spd2_arr1));
		qsort(spd2_arr2, SPEED2LEN, sizeof(struct speed2_elem),
		      &spd2_cmp);
	}
	gettimeofday(&end, NULL);
	printf("Time taken for %u sorts of %u elements for std qsort: %f\n",
	       NTRIES2, SPEED2LEN, 
	       (double)(end.tv_sec - start.tv_sec) + 
	       (double)(end.tv_usec - start.tv_usec) / 1000000.0);
	printf("standard qsort used %llu comparisons\n", spd2_ncmps);
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

	printf("\n\nspeed test 2 -- with %u byte elements\n",
	       (unsigned)sizeof(struct speed2_elem));
	test_speed2();

	printf("\n\nspeed test 3 -- with %u byte elements\n",
	       (unsigned)sizeof(struct speed2_elem));
	test_speed3();

	return 0;
}
