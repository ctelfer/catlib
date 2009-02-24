#include <cat/sort.h>

#define INTSWAP		0
#define LONGSWAP	1
#define BULKLONG	2
#define BULKBYTE	3


#define SWAPTYPE(a, size)					\
	(((size) == sizeof(int)) && 				\
         (((byte_t*)(a) - (byte_t *)0) % sizeof(int) == 0)) ?	\
	 INTSWAP :						\
	(((size) == sizeof(long)) && 				\
         (((byte_t *)(a) - (byte_t *)0) % sizeof(long) == 0)) ?	\
	 LONGSWAP :						\
	(((size) % sizeof(long) == 0) && 			\
         (((byte_t *)(a) - (byte_t *)0) % sizeof(long) == 0)) ?	\
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
			size_t __n = esize / sizeof(long);	\
			long *__ap = (long *)a;			\
			long *__bp = (long *)b;			\
			long __t;				\
			do {					\
				__t = *__ap;			\
				*__ap++ = *__bp;		\
				*__bp++ = __t;			\
			} while ( --__n > 0 );			\
		}						\
		case BULKBYTE: {				\
			size_t __n = esize;			\
			byte_t *__ap = (byte_t *)a;		\
			byte_t *__bp = (byte_t *)b;		\
			byte_t __t;				\
			do {					\
				__t = *__ap;			\
				*__ap++ = *__bp;		\
				*__bp++ = __t;			\
			} while ( --__n > 0 );			\
		}						\
		break;						\
	}



static void isort_i(void *arr, const size_t nelem, const size_t esize, 
		    cmp_f cmp, int swaptype)
{
	byte_t *start = arr, *p1, *p2, *tmp;
	const byte_t * const end = (byte_t *)arr + nelem * esize;

	for ( p1 = start + esize ; p1 < end ; p1 += esize ) {
		for ( p2 = p1 ; p2 > start ; p2 -= esize ) {
			if ((*cmp)(p2 - esize, p2) > 0) {
				tmp = p2 - esize;
				SWAP(tmp, p2, swaptype);
			} else {
				break;
			}
		}
	}
}


void isort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	int type = SWAPTYPE(arr, esize);
	if ( arr == NULL || nelem <= 1 || esize == 0 || cmp == NULL )
		return;
	isort_i(arr, nelem, esize, cmp, type);
}


static void ssort_i(void *arr, const size_t nelem, const size_t esize, 
		    cmp_f cmp, int swaptype)
{
	byte_t *p1, *p2, *min;
	const byte_t * const end = (byte_t *)arr + nelem * esize;

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
	if ( arr == NULL || nelem <= 1 || esize == 0 || cmp == NULL )
		return;
	ssort_i(arr, nelem, esize, cmp, type);
}



static void reheap_down(void *arr, const size_t nelem, const size_t esize, 
		        cmp_f cmp, int swaptype, size_t pos)
{
	int didswap;
	size_t cld;
	byte_t *ep, *cp;

	if ( pos >= nelem ) 
		return;

	do {
		if ( (cld = (pos << 1) + 1) >= nelem )
			break;
		didswap = 0;
		ep = (byte_t *)arr + pos * esize;

		/* find the max child */
		cp = (byte_t *)arr + cld * esize;
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
	byte_t *ep;

	/* build the max heap */
	i = nelem >> 1; 
	do { 
		reheap_down(arr, nelem, esize, cmp, swaptype, i);
	} while ( i-- > 0 );

	while ( nelem > 1 ) {
		--nelem;
		ep = (byte_t *)arr + nelem * esize;
		SWAP((byte_t*)arr, ep, swaptype);
		reheap_down(arr, nelem, esize, cmp, swaptype, 0);
	}
}


void hsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	int type = SWAPTYPE(arr, esize);
	if ( arr == NULL || nelem <= 1 || esize == 0 || cmp == NULL )
		return;
	hsort_i(arr, nelem, esize, cmp, type);
}



#define QS_MAXDEPTH	48
struct qswork {
	byte_t *start;
	size_t nelem;
	int qdepth;
};

#define PUSH(__stk, __t, __s, __ne, __qd)	\
	__stk[__t].start = __s;			\
	__stk[__t].nelem = __ne;		\
	__stk[__t++].qdepth = __qd;

#define POP(__stk, __t, __s, __ne, __qd)	\
	__s = __stk[--__t].start;		\
	__ne = __stk[__t].nelem;		\
	__qd = __stk[__t].qdepth;

/* This is actually an "intro-sort" because it will only sort down to log2 */
/* of the number of elements and will heap sort sub arrays past that. This */
/* guarantees nlogn performance.  Also, the sort uses insertion sort for   */
/* subarrays of 7 elements or less. */
void qsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	byte_t *pivot, *lo, *hi, *end;
	int bailout_depth = 1, depth;
	struct qswork stack[QS_MAXDEPTH];
	int top = 0;
	size_t n;
	const int swaptype = SWAPTYPE(arr, esize);

	if ( arr == NULL || nelem <= 1 || esize == 0 || cmp == NULL )
		return;

	/* find floor(log2(nelem)) + 1 */
	n = nelem;
	do {
		++bailout_depth;
		n >>= 1;
	} while ( n );

	PUSH(stack, top, arr, nelem, 1);

	while ( top != 0 ) {
		POP(stack, top, pivot, n, depth);
		/* insertion sort for small arrays */
		if ( n <= 8 ) {
			isort_i(pivot, n, esize, cmp, swaptype);
			continue;
		}
		/* heap sort past the log2 of # of elements in recurse depth */
		if ( depth > bailout_depth ) {
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
			PUSH(stack, top, pivot, (lo - pivot) / esize, depth+1);
		}
		else {
			/* see whether the last element goes in the first or */
			/* second subarray */
			if ( (*cmp)(pivot, lo) >= 0 ) /* low subarray */
				hi += esize;
			else                          /* high subarray */
				lo -= esize;
			if ( lo != pivot ) {
				SWAP(pivot, lo, swaptype);
				PUSH(stack, top, pivot, (lo - pivot) / esize,
				     depth+1);
			} 
		}


		if ( top == QS_MAXDEPTH )
			hsort_i(hi, (end - hi) / esize, esize, cmp, swaptype);
		else
			PUSH(stack, top, hi, (end - hi) / esize, depth+1);
	} 
}

