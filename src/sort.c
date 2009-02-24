#include <cat/sort.h>

#define INTSWAP		0
#define LONGSWAP	1
#define BULKLONG	2
#define BULKBYTE	3


#define SWAPTYPE(a, size)					\
	(((size) == sizeof(int)) && 				\
         (((char *)(a) - (char *)0) % sizeof(int) == 0)) ?	\
	 INTSWAP :						\
	(((size) == sizeof(long)) && 				\
         (((char *)(a) - (char *)0) % sizeof(long) == 0)) ?	\
	 LONGSWAP :						\
	(((size) % sizeof(long) == 0) && 			\
         (((char *)(a) - (char *)0) % sizeof(long) == 0)) ?	\
	 BULKLONG :						\
	 BULKBYTE
	

#define SWAP(a,b,type) 						\
	switch (type) { 					\
		case INTSWAP: {					\
			int __t = *(int *)a;			\
			*(int *)a = *(int *)b;			\
			*(int *)b = __t;			\
		}						\
		break;						\
		case LONGSWAP: {				\
			long __t = *(long *)a;			\
			*(long *)a = *(long *)b;		\
			*(long *)b = __t;			\
		}						\
		break;						\
		case BULKLONG: {				\
			size_t n = esize / sizeof(long);	\
			long *ap = (long *)a;			\
			long *bp = (long *)b;			\
			long __t;				\
			do {					\
				__t = *ap;			\
				*ap++ = *bp;			\
				*bp++ = __t;			\
			} while ( --n > 0 );			\
		}						\
		case BULKBYTE: {				\
			size_t n = esize;			\
			byte_t *ap = (byte_t *)a;		\
			byte_t *bp = (byte_t *)b;		\
			byte_t __t;				\
			do {					\
				__t = *ap;			\
				*ap++ = *bp;			\
				*bp++ = __t;			\
			} while ( --n > 0 );			\
		}						\
		break;						\
	}



static void isort_i(void *arr, const size_t nelem, const size_t esize, 
		    cmp_f cmp, int swaptype)
{
	char *start = arr, *p1, *p2, *tmp;
	const char * const end = (char *)arr + nelem * esize;
	for ( p1 = start + esize ; p1 < end ; p1 += esize ) {
		for ( p2 = p1 ; p2 > start ; p2 -= esize ) {
			if ((*cmp)(p2 - esize, p2) > 0) {
				tmp = p2 - esize;
				SWAP(tmp, p2, swaptype);
			}
		}
	}
}


void isort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	int type = SWAPTYPE(arr, esize);
	if ( nelem == 0 )
		return;
	isort_i(arr, nelem, esize, cmp, type);
}


static void ssort_i(void *arr, const size_t nelem, const size_t esize, 
		    cmp_f cmp, int swaptype)
{
	char *p1, *p2, *min;
	const char * const end = (char *)arr + nelem * esize;
	for ( p1 = arr ; p1 < end - esize ; p1 += esize ) {
		min = p1;
		for ( p2 = p1 + esize ; p2 < end ; p2 += esize )
			if ((*cmp)(min, p2) > 0)
				min = p2;
		if ( min != p1 )
			SWAP(p1, min, swaptype);
	}
}


void ssort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	int type = SWAPTYPE(arr, esize);
	if ( nelem == 0 )
		return;
	ssort_i(arr, nelem, esize, cmp, type);
}



static void reheap_down(void *arr, const size_t nelem, const size_t esize, 
		        cmp_f cmp, int swaptype, size_t pos)
{
	int didswap;
	size_t cld;
	char *ep, *cp;

	if ( pos >= nelem ) 
		return;

	do {
		didswap = 0;
		if ( (cld = (pos << 1) + 1) >= nelem )
			break;

		ep = (char *)arr + pos * esize;

		/* find the max child */
		cp = (char *)arr + cld * esize;
		if ( ( cld + 1 < nelem ) && ( (*cmp)(cp + esize, cp) > 0 ) ) {
			++cld;
			cp += esize;
		}

		if ( (*cmp)(ep, cp) < 0 ) {
			didswap = 1;
			SWAP(ep, cp, swaptype);
			pos = cld;
		}
	} while ( didswap );
}


static void hsort_i(void *arr, size_t nelem, const size_t esize, 
		    cmp_f cmp, int swaptype)
{
	size_t i;
	char *ep;

	/* build the max heap */
	i = nelem >> 1; 
	do { 
		reheap_down(arr, nelem, esize, cmp, swaptype, i);
	} while ( i-- > 0 );

	while ( nelem > 1 ) {
		--nelem;
		ep = (char *)arr + nelem * esize;
		SWAP((char *)arr, ep, swaptype);
		reheap_down(arr, nelem, esize, cmp, swaptype, 0);
	}
}


void hsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	int type = SWAPTYPE(arr, esize);
	if ( nelem == 0 )
		return;
	hsort_i(arr, nelem, esize, cmp, type);
}



#define QS_MAXDEPTH	48
struct qswork {
	char *start;
	size_t nelem;
};

#define PUSH(__stk, __t, __s, __ne)	\
	__stk[__t].start = __s;		\
	__stk[__t++].nelem = __ne;

#define POP(__stk, __t, __s, __ne)	\
	__s = __stk[--__t].start;	\
	__ne = __stk[__t].nelem;

/* This is actually an "intro-sort" because it will only sort down to log2 */
/* of the number of elements and will heap sort sub arrays past that. This */
/* guarantees nlogn performance.  Also, the sort uses insertion sort for   */
/* subarrays of 7 elements or less. */
void qsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	int qd = 1;
	struct qswork stack[QS_MAXDEPTH];
	int top = 0;
	const int swaptype = SWAPTYPE(arr, esize);
	size_t n;
	char *pivot, *lo, *hi, *end;

	if ( arr == NULL || nelem == 0 || esize == 0 || cmp == NULL )
		return;

	/* find floor(log2(nelem)) + 1 */
	n = nelem;
	do {
		++qd;
		n >>= 1;
	} while ( n );

	PUSH(stack, top, arr, nelem);

	while ( top != 0 ) {
		POP(stack, top, pivot, n);
		/* insertion sort for small arrays */
		if ( n <= 8 ) {
			isort_i(pivot, n, esize, cmp, swaptype);
			continue;
		}
		/* heap sort past the log2 of # of elements in recurse depth */
		if ( top + 1 >= qd ) {
			hsort_i(pivot, n, esize, cmp, swaptype);
			continue;
		}

		/* actual quicsort:  we must have 9 or more elements now */
		lo = pivot + esize;
		end = pivot + esize * n;
		hi = end - esize;
		while ( lo < hi ) {
			while ( (lo < hi) && (*cmp)(pivot, lo) >= 0 )
				lo += esize;
			while ( (lo < hi) && (*cmp)(pivot, hi) < 0 )
				hi -= esize;
			if ( lo < hi ) {
				SWAP(lo, hi, swaptype);
				lo += esize;
				hi -= esize;
			}
		}

		/* At this point, there are 2 cases:  low > hi or low == hi. */
		/* the former can only happen if we ended the loop swapping */
		/* two adjacent elements.  The latter happens when the scan */
		/* can't find an element lower or higher than the pivot */
		if ( lo > hi ) { 
			/* swap the pivot to the end of the low array */
			SWAP(pivot, hi, swaptype);
			PUSH(stack, top, pivot, (lo - pivot) / esize);
		}
		else {
			/* see whether the last element goes in the first or */
			/* second subarray */
			if ( (*cmp)(pivot, lo) >= 0 ) /* low subarray */
				hi += esize;
			else                        /* high subarray */
				lo -= esize;
			if ( lo != pivot ) {
				SWAP(pivot, lo, swaptype);
				PUSH(stack, top, pivot, (lo - pivot) / esize);
			} 
		}


		if ( top == QS_MAXDEPTH ) {
			size_t num = (end - hi) / esize;
			if ( num <= 8 )
				isort_i(hi, num, esize, cmp, swaptype);
			else
				hsort_i(hi, num, esize, cmp, swaptype);
		}
		else {
			PUSH(stack, top, hi, (end - hi) / esize);
		}
	} 
}

