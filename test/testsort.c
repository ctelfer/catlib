#include <cat/sort.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define ASIZE	256 */
#define ASIZE	12345
int arr1[ASIZE], arr2[ASIZE], all[ASIZE];
unsigned long long ncmps;

int intcmp(void *i1, void *i2)
{
	++ncmps;
	return *(int *)i1 - *(int *)i2;
}

void test_intarr()
{
	int i;

	for ( i = 1 ; i <= ASIZE ; ++i )
		all[i] = i;
	for ( i = 0; i < ASIZE; ++i ) {
#if DEBUGGING
		unsigned idx = rand();
		arr1[i] = all[idx % ASIZE];
#else
		arr1[i] = rand();
#endif
	}

	/* insertion sort */
	ncmps = 0;
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
		       ncmps);

	/* selection sort */
	ncmps = 0;
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
			ncmps);

	/* heap sort */
	ncmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	hsort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for heap sort\n", i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("heap sort succeeded with %llu comparisons \n", ncmps);

	/* quick sort */
	ncmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	qsort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for quick sort\n", i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("quick sort succeeded with %llu comparisons\n", ncmps);

	/* quick sort 2 */
	ncmps = 0;
	memcpy(arr2, arr1, sizeof(arr1));
	qsort_array(arr2, ASIZE, sizeof(int), intcmp);
	for ( i = 1; i < ASIZE; i++ ) {
		if (arr2[i-1] > arr2[i]) {
			printf("Error at position %d for quick sort (sorted)\n",
			       i);
			break;
		}
	}
	if ( i == ASIZE )
		printf("quick sort 2 succeeded with %llu comparisons\n", ncmps);
}

int main(int argc, char *argv[])
{
	size_t i;

	srand(0);
	printf("run1\n");
	test_intarr();
	printf("\n\nrun2\n");
	srand(10);
	test_intarr();

	return 0;
}
