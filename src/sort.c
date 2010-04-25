#include <cat/sort.h>
#include <string.h>


#define PTRSWAP		0
#define BULKLONG	1
#define BULKBYTE	2


#define SWAPTYPE(a, size)						\
	(((size) == sizeof(void *)) && 				        \
	 (((byte_t *)(a) - (byte_t *)0) % sizeof(cat_align_t) == 0)) ?	\
	 PTRSWAP :						        \
	(((size) % sizeof(long) == 0) && 			        \
	 (((byte_t *)(a) - (byte_t *)0) % sizeof(cat_align_t) == 0)) ?	\
	 BULKLONG :						        \
	 BULKBYTE
	

#define SWAP(a,b,type) 							\
	switch (type) { 						\
		case PTRSWAP: {						\
			void *__t = *(void **)(a);			\
			*(void **)(a) = *(void **)(b);	        	\
			*(void **)(b) = __t;			        \
		}							\
		break;							\
		case BULKLONG: {					\
			register size_t __n = esize / sizeof(long);	\
			register long *__ap = (long *)(a);		\
			register long *__bp = (long *)(b);		\
			register long __t;			        \
			do {						\
				__t = *__ap;			        \
				*__ap++ = *__bp;		        \
				*__bp++ = __t;			        \
			} while ( --__n > 0 );				\
		}							\
		break;				        		\
		default: {						\
			register size_t __n = esize;			\
			register byte_t *__ap = (byte_t *)(a);		\
			register byte_t *__bp = (byte_t *)(b);		\
			register byte_t __t;				\
			do {						\
				__t = *__ap;			        \
				*__ap++ = *__bp;		        \
				*__bp++ = __t;			        \
			} while ( --__n > 0 );				\
		}							\
	}


static void isort_i(void *arr, const size_t nelem, const size_t esize, 
		    cmp_f cmp, int swaptype)
{
	byte_t *start = arr, *p1, *p2, *tmp;
	const byte_t * const end = (byte_t *)arr + nelem * esize;

	for ( p1 = start + esize ; p1 < end ; p1 += esize ) {
		for ( p2 = p1 ; p2 > start ; p2 -= esize ) {
			tmp = p2 - esize;
			if ((*cmp)(tmp, p2) > 0) {
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
	__stk[__t].nelem = __ne;        	\
	__stk[__t++].qdepth = __qd;

#define POP(__stk, __t, __s, __ne, __qd)	\
	__s = __stk[--__t].start;		\
	__ne = __stk[__t].nelem;		\
	__qd = __stk[__t].qdepth;


static byte_t *median3(byte_t *m1, byte_t *m2, byte_t *m3, cmp_f cmp)
{
	if ( (*cmp)(m1, m2) < 0 ) { 
		/* (1,2,3),(3,1,2),(1,3,2) */
		if ( (*cmp)(m2, m3) < 0 ) { /* (1,2,3) */
			return m2;
		} else if ( (*cmp)(m1, m3) < 0 ) { /* (1,3,2) */
			return m3;
		} else { /* (3,1,2) */
			return m1;
		}
	} else {  /* m2 >= m1 */
		/* (2,1,3),(2,3,1),(3,2,1) */
		if ( (*cmp)(m1, m3) < 0 ) { /* (2,1,3) */
			return m1;
		} else if ( (*cmp)(m2, m3) < 0 ) { /* (2,3,1) */
			return m3;
		} else { /* (3,2,1) */
			return m2;
		}
	}
}


#define ISORT_THRESH	8

/* This is actually an "intro-sort" because it will only sort down to 2log2 */
/* of the number of elements and will heap sort sub arrays past that. This */
/* guarantees nlogn performance.  Also, the sort uses insertion sort for   */
/* subarrays of 8 elements or less. */
void qsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp)
{
	byte_t *start, *pivot, *lo, *hi, *end, *m1, *m2, *m3;
	int bailout_depth = 0, depth;
	struct qswork stack[QS_MAXDEPTH];
	int top = 0;
	size_t n, nlo, stride;
	const int swaptype = SWAPTYPE(arr, esize);

	if ( arr == NULL || nelem <= 1 || esize == 0 || cmp == NULL )
		return;

	if ( nelem <= ISORT_THRESH ) {
		isort_i(arr, nelem, esize, cmp, swaptype);
		return;
	}

	/* find 2*floor(log2(nelem)) */
	n = nelem;
	while ( (n >>= 1) )
		++bailout_depth;
	bailout_depth = bailout_depth * 2;
	if ( bailout_depth > QS_MAXDEPTH-3 )
		bailout_depth = QS_MAXDEPTH-3;

	start = arr;
	n = nelem;
	depth = 1;

	while ( 1 ) {
		/* heap sort past the log2 of # of elements in recurse depth */
		if ( depth > bailout_depth ) {
			hsort_i(start, n, esize, cmp, swaptype);
			if ( top == 0 )
				break;
			POP(stack, top, start, n, depth);
			continue;
		}

		/* actual quicksort:  we must have 9 or more elements now */
		m1 = start;
		lo = start + esize;
		m2 = start + (n >> 1) * esize;
		end = start + esize * n;
		m3 = hi = end - esize;
		if ( n > 64 ) {
			stride = esize * (n / 9);
			m1 = median3(m1, m1 + stride, m1 + stride * 2, cmp);
			m2 = median3(m2 - stride, m2, m2 + stride, cmp);
			m3 = median3(m3 - stride * 2, m3 - stride, m3, cmp);
		}
		pivot = median3(m1, m2, m3, cmp);
		if ( pivot != start )
			SWAP(start, pivot, swaptype);

		while ( 1 ) {
			while ( (*cmp)(start, lo) >= 0 ) {
				if ( lo == hi ) { 
					hi += esize;
					goto done_inner;
				}
				lo += esize;
			}
			while ( (*cmp)(start, hi) < 0 ) {
				if ( lo == hi ) { 
					lo -= esize;
					goto done_inner;
				}
				hi -= esize;
			}

			SWAP(lo, hi, swaptype);
			if ( lo + esize == hi )
				goto done_inner;
			lo += esize;
			hi -= esize;
		}
done_inner:

		/* swap the pivot to the end of the low array */
		SWAP(start, lo, swaptype);
		depth += 1;

		n = (end - hi) / esize;
		if ( n > ISORT_THRESH ) { 
			nlo = (lo - start) / esize;
			if ( nlo > ISORT_THRESH ) {
				PUSH(stack, top, start, nlo, depth);
			} else {
				isort_i(start, nlo, esize, cmp, swaptype);
			}
			start = hi;
		} else {
			isort_i(hi, n, esize, cmp, swaptype);
			n = (lo - start) / esize;
			if ( n <= ISORT_THRESH ) { 
				isort_i(start, n, esize, cmp, swaptype);
				if ( top == 0 )
					break;
				POP(stack, top, start, n, depth);
			}
		}
	} 
}



void array_to_voidp(void **varr, void *arr, size_t nelem, size_t esize)
{
	byte_t *p = arr;

	if ( (nelem < 1) || (esize == 0) || (~(size_t)0 / esize < nelem) )
		return;
	do {
		*varr++ = p;
		p += esize;
	} while ( --nelem > 0 );
}


void permute_array(void *arr, void *tmp, void **varr, size_t nelem, size_t esize)
{
	byte_t *ap = arr, *phi, *lastp, *hold;
	size_t hi, to;

	if ( (nelem < 2) || (esize == 0) || (~(size_t)0 / esize < nelem) )
		return;

	for ( hi = 0, phi = ap; hi < nelem ; ++hi, phi += esize ) {
		if ( varr[hi] == phi )
			continue;

		to = hi;
		lastp = phi;
		memcpy(tmp, phi, esize);

		do {
			memcpy(lastp, varr[to], esize);
			hold = lastp;
			lastp = varr[to];
			varr[to] = hold;
			to = (lastp - ap) / esize;
		} while ( varr[to] != phi );

		memcpy(lastp, tmp, esize);
		varr[to] = lastp;
	}
}
