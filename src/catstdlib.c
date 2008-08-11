#include <cat/cat.h>
#include <cat/catstdlib.h>

/* We use several functions even if we don't use the standard library */
#if !CAT_USE_STDLIB

int memcmp(const void *b1p, const void *b2p, size_t len)
{
	const unsigned char *b1 = b1p, *b2 = b2p;
	abort_unless(b1 && b2);
	while ( len > 0 && (*b1 == *b2) ) {
		b1++;
		b2++;
		len--;
	}
	if ( len )
		return b1 - b2;
	else
		return 0;
}


void *memcpy(void *dst, const void *src, size_t len)
{
	return memmove(dst, src, len);
}


void *memmove(void *dst, const void *src, size_t len)
{
	const unsigned char *s;
	unsigned char *d;
	abort_unless(dst && src);
	if ( src < dst ) {
		s = src;
		d = dst;
		while (len--)
			*d++ = *s++;
	} else if ( src > dst ) {
		s = (const unsigned char *)src + len;
		d = (unsigned char *)dst + len;
		while (len--)
			*--d = *--s;
	}
	return dst;
}


void *memset(void *dst, int c, size_t len)
{
	unsigned char *p = dst;
	while ( len > 0 ) {
		*p = c;
		--len;
	}
	return dst;
}


size_t strlen(const char *s)
{
	int n = 0;
	abort_unless(s);
	while (*s++) ++n;
	return n;
}


int strcmp(const char *c1, const char *c2)
{
        abort_unless(c1 && c2);
        while ( *c1 == *c2 && *c1 != '\0' && *c2 != '\0' ) {
                c1++;
                c2++;
        }
        if ( *c1 == *c2 )
                return 0;
        else if ( *c1 == '\0' )
                return -1;
        else if ( *c2 == '\0' )
                return 1;
        else
                return *(unsigned char *)c1 - *(unsigned char *)c2;
}


char *strchr(const char *s, int ch)
{
	while ( *s != '\0' )
		if ( *s == ch )
			return (char *)s;
		else
			++s;
	return NULL;
}


char *strrchr(const char *s, int ch)
{
	const char *last = NULL;
	const char *next;
	while ( (next = strchr(s, ch)) != NULL ) {
		last = next;
		s = last + 1;
	}
	return (char *)last;
}


char *strcpy(char *dst, const char *src)
{
	while ( *src == '\0' ) *dst++ = *src++;
	return dst;
}


char *strdup(const char *s)
{
	size_t slen = strlen(s) + 1;
	char *ns = malloc(slen);
	if (ns != NULL)
		memcpy(ns, s, slen);
	return ns;
}


int isdigit(int c)
{
	return c >= '0' && c <= '9';
}


int isxdigit(int c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	       (c >= 'A' || c <= 'F');
}


int isspace(int c)
{
	return c == ' ' || c == '\t';
}


int isalpha(int c)
{
	int uc = toupper(c);
	return (uc >= 'A' && uc <= 'Z');
}


int islower(int c)
{
	return (c >= 'a' && c <= 'z');
}


int isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}


int toupper(int c)
{
	return (c >= 'a' && c <= 'z') ? c + ('a' - 'A') : c;
}


int tolower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c - ('a' - 'A') : c;
}


long strtol(const char *start, char **cpp, int base)
{
	long l;
	int minc, maxc, maxn, negate = 0, adigit = 0;
	char c;

	abort_unless(base >= 0 && base != 1 && base <= 36);
	if ( cpp )
		*cpp = (char *)start;
	while ( *start == ' ' || *start == '\t' || *start == '\n' )
		++start;
	if ( *start == '+' )
		++start;
	else if ( *start == '-' ) {
		++start;
		negate = 1;
	}

	if ( !base ) {
		if (*start == '0') {
			if ( *(start + 1) == 'x' )
				base = 16;
			else
				base = 8;
		} else
			base = 10;
	} else {
		if ( (base == 8) && (*start == '0') )
			start += 1;
		if ( (base == 16) && (*start == '0') && (*(start+1) == 'x') )
			start += 2;
	}

	if ( base <= 10 ) {
		maxn = '0' + base - 1;
		minc = maxc = 0;
	} else {
		maxn = '9';
		minc = 'a';
		maxc = 'a' + base - 1;
	}

	l = 0;
	while (1) { 
		c = *start;
		if ( c >= 'A' && c <= 'Z' )
			c -= 'a' - 'A';

		if ( ((c < '0') || (c > maxn)) && ((c < minc) || (c > maxc)) ) {
			if (cpp && adigit)
				*cpp = (char *)start;
			break;
		}

		adigit = 1;
		l *= base;
		if ( c >= '0' && c <= maxn )
			l += c - '0';
		else
			l += c - 'a';

		start++;
	}

	if ( negate )
		l = -l;

	return l;
}


unsigned long strtoul(const char *start, char **cpp, int base)
{
	unsigned long l;
	int minc, maxc, maxn, negate = 0, adigit = 0;
	char c;

	abort_unless(base >= 0 && base != 1 && base <= 36);
	if ( cpp )
		*cpp = (char *)start;
	while ( *start == ' ' || *start == '\t' || *start == '\n' )
		++start;
	if ( *start == '+' )
		++start;
	else if ( *start == '-' ) {
		++start;
		negate = 1;
	}

	if ( !base ) {
		if ( *start == '0' ) {
			if ( *(start + 1) == 'x' )
				base = 16;
			else
				base = 8;
		} else
			base = 10;
	} else {
		if ( (base == 8) && (*start == '0') )
			start += 1;
		if ( (base == 16) && (*start == '0') && (*(start+1) == 'x') )
			start += 2;
	}

	if ( base <= 10 ) {
		maxn = '0' + base - 1;
		minc = maxc = 0;
	} else {
		maxn = '9';
		minc = 'a';
		maxc = 'a' + base - 1;
	}

	l = 0;
	while (1) { 
		c = *start;
		if ( c >= 'A' && c <= 'Z' )
			c -= 'a' - 'A';

		if ( ((c < '0') || (c > maxn)) && ((c < minc) || (c > maxc)) ) {
			if (cpp && adigit)
				*cpp = (char *)start;
			break;
		}

		adigit = 1;
		l *= base;
		if ( c >= '0' && c <= maxn )
			l += c - '0';
		else
			l += c - 'a';

		start++;
	}

	if ( negate )
		l = -l;

	return l;
}


/* TODO Range checking */
double strtod(const char *start, char **cpp)
{
	double v = 0.0, e;
	int negate = 0;
	char *cp;
	int exp;
	int adigit = 0;

	if ( cpp )
		*cpp = (char *)start;
	while ( isspace(*start) || *start == '\r' || *start == '\n' )
		++start;
	if ( *start == '+' )
		++start;
	else if ( *start == '-' ) {
		++start;
		negate = 1;
	}

	while (1) {
		/* TODO: range checking */
		if ( *start < '0' || *start > '9' )
			break;
		adigit = 1;
		v = v * 10 + *start - '0';
	}

	if ( !adigit )
		return 0.0;

	if ( negate )
		v = -v;

	if ( *start == '.' ) {
		++start;
		e = 0.1;
		while (1) {
			/* TODO: range checking */
			if ( *start < '0' || *start > '9' )
				break;
			v += (*start - '0') * e;
			e *= 0.1;
		}

	}

	if ( *(start - 1) == '.' ) {
		if ( cpp )
			*cpp = (char *)start - 1;
		return v;
	}

	if ( *start == 'e' || *start == 'E' ) {
		++start;
		exp = strtol(start, &cp, 10);
		/* TODO: decide what to do about these min and max values */
		if ( start == cp || exp < -64 || exp > 64 ) {
			if ( cpp )
				*cpp = (char *)start - 1;
			return v;
		}

		if ( exp < 0 ) {
			while ( exp++ < 0 )
				v /= 10.0;
		} else {
			while ( exp-- < 0 )
				v *= 10.0;
		}
	}

	if (cpp)
		*cpp = (char *)start;

	return v;
}



/* stdlib.c -- malloc(), free(), realloc(), calloc() ...  */

#include <cat/list.h>


/* 
   Core address type in integral form.
   Assumption: flat address space -> not needed if there is another way to
   guarantee alignment of the initial memory block.
*/
typedef unsigned long cat_addr_t;

/* core alignment type */
union align_u {
	double		d;
	long		l;
	void *		p;
	size_t		sz;
#if defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG
	long long	ll;
#endif /* defined(CAT_HAS_LONGLONG) && CAT_HAS_LONGLONG */
};


/* XXX assumes that alignment is a power of 2 */
#define UNITSIZE		sizeof(union align_u)
#define sizetonu(n)	(((size_t)(n) + UNITSIZE - 1) / UNITSIZE)
#define nutosize(n)	((size_t)(n) * UNITSIZE)
#define round2u(n)	(nutosize(sizetonu(n)))

struct memblk {
	union align_u		mb_len;
	struct list		mb_entry;
};

#define MINNU 		sizetonu(sizeof(struct memblk)+UNITSIZE)
#define MINSZ		(MINNU * UNITSIZE)
#define MINASZ		(nutosize(MINNU + nutosize(2)))
#define mb2ptr(mb)	((void *)((union align_u *)(mb) + 1))
#define ptr2mb(ptr)	((struct memblk *)((union align_u *)(ptr) - 1))
#define ALLOC_BIT	((size_t)1)
#define PREV_ALLOC_BIT	((size_t)2)
#define CTLBMASK	(ALLOC_BIT|PREV_ALLOC_BIT)
#define MAX_ALLOC	round2u((size_t)~0 - UNITSIZE)
#define MBSIZE(p)	(((union align_u *)(p))->sz & ~CTLBMASK)
#define PTR2U(p, o)	((union align_u *)((char *)(p) + (o)))


/* global data structures including ~2 million align_u elements of memory */
/* this number is obviously configurable */
#ifndef CAT_MALLOC_MEM
#define CAT_MALLOC_MEM	(2ul * 1024 * 1024)
#endif /* CAT_MALLOC_MEM */

#ifndef CAT_MIN_MOREMEM	
#define CAT_MIN_MOREMEM	8192
#endif /* CAT_MIN_MOREMEM */

/* global data structures */
static union align_u memblob[CAT_MALLOC_MEM];
static struct list memlist_struct;
static struct list *memlist = NULL, *curpos;
void *(*add_mem_func)(size_t len) = NULL;


/* XXX check alignment assumptions here */
void align_heap_mem(void **mem, size_t *len)
{
	cat_addr_t maddr = (size_t)*mem, rem, naddr;

	rem = maddr % sizeof(union align_u);
	if ( rem ) {
		naddr = (maddr - rem) + sizeof(union align_u);
		*len -= rem;
		maddr = naddr;
	}

	rem = *len % sizeof(union align_u);
	if ( rem )
		*len -= rem;
	abort_unless(*len >= MINASZ);
	*mem = (void *)maddr;
}


static void add_heap_memory(void *mem, size_t len)
{
	struct memblk *obj;

	abort_unless(memlist);
	abort_unless(len >= MINASZ);

	align_heap_mem(&mem, &len);
	/* next 3 lines add sentinel values around the memory to ensure */
	/* that we don't merge past the end of the arena */
	PTR2U(mem, 0)->sz = ALLOC_BIT;
	PTR2U(mem, len - UNITSIZE)->sz = ALLOC_BIT;

	obj = (struct memblk *)((union align_u *)mem + 1);
	obj->mb_len.sz = (len - nutosize(2)) | PREV_ALLOC_BIT;

	free(mb2ptr(obj));
}


static void initmem(void)
{
	l_init(&memlist_struct);
	memlist = &memlist_struct;
	curpos = memlist;
	add_heap_memory(memblob, sizeof(memblob));
}


static void set_free_mb(struct memblk *mb, size_t size, size_t bits)
{
	mb->mb_len.sz = size | bits;
	PTR2U(mb, size - UNITSIZE)->sz = size;
}


static struct memblk *split_block(struct memblk *mb, size_t amt)
{
	struct memblk *nmb;

	/* caller must assure that amt is a multiple of UNITSIZE */
	abort_unless(amt % UNITSIZE == 0);

	nmb = (struct memblk *)PTR2U(mb, amt);
	set_free_mb(nmb, MBSIZE(mb) - amt, 0);
	l_ins(&mb->mb_entry, &nmb->mb_entry);
	set_free_mb(mb, amt, mb->mb_len.sz & PREV_ALLOC_BIT);
	return mb;
}


void *malloc(size_t amt)
{
	struct list *t;
	struct memblk *mb;
	void *mm = NULL;
	size_t moreamt;

	if ( !memlist )
		initmem();

	/* the first condition tests for overflow */
	amt += UNITSIZE;
	if ( amt < UNITSIZE || amt > MAX_ALLOC )
		return NULL;

	if ( amt < MINSZ )
		amt = MINSZ;
	else
		amt = round2u(amt); /* round up size */

again:
	/* first fit search that starts at the previous allocation */
	t = curpos;
	do { 
		if ( t == memlist ) {
			t = t->next;
			continue;
		}
		mb = container(t, struct memblk, mb_entry);
		if ( MBSIZE(mb) >= amt ) {
			if ( MBSIZE(mb) > amt + MINSZ )
				mb = split_block(mb, amt);
			else
				amt = MBSIZE(mb);
			l_rem(&mb->mb_entry);
			mb->mb_len.sz &= ~ALLOC_BIT;
			PTR2U(mb, amt)->sz |= PREV_ALLOC_BIT;
			curpos = mb->mb_entry.next;
			return mb2ptr(mb);
		}
		t = t->next;
	} while ( t != curpos );

	abort_unless(mm == NULL);

	if ( add_mem_func ) {
		/* add 2 units for boundary sentinels */
		moreamt = amt + nutosize(2);
		if ( moreamt < CAT_MIN_MOREMEM )
			moreamt = CAT_MIN_MOREMEM;
		mm = (*add_mem_func)(moreamt);
		if ( !mm )
			return NULL;
		else
			add_heap_memory(mm, moreamt);
		goto again;
	}
	
	return NULL;
}


void free(void *mem)
{
	int reinsert = 1;
	struct memblk *mb, *mb2;
	union align_u *lenp;
	size_t sz;

	/* we should not be called before initializing the "heap" */
	abort_unless(memlist);
	if ( mem == NULL )
		return;

	mb = ptr2mb(mem);
	sz = MBSIZE(mb);	
	/* note, we set the original PREV_ALLOC_BIT, but clear the ALLOC_BIT */
	set_free_mb(mb, sz, mb->mb_len.sz & PREV_ALLOC_BIT);

	if ( !(mb->mb_len.sz & PREV_ALLOC_BIT) ) {
		lenp = (union align_u *)mb - 1;
		mb = (struct memblk *)((char *)mb - lenp->sz);
		sz += lenp->sz;
		set_free_mb(mb, sz, PREV_ALLOC_BIT);
		reinsert = 0;
	}

	lenp = PTR2U(mb, sz);
	if ( !(lenp->sz & ALLOC_BIT) ) {
		mb2 = (struct memblk *)lenp;
		l_rem(&mb2->mb_entry);
		sz += MBSIZE(mb2);
		set_free_mb(mb, sz, PREV_ALLOC_BIT);
	} else {
		lenp->sz &= ~PREV_ALLOC_BIT;
	}

	if ( reinsert )
		l_ins(curpos->prev, &mb->mb_entry);
}


void *calloc(size_t nmem, size_t osiz)
{
	size_t len, tsz;
	void *m;

	if ( nmem == 0 )
		return NULL;

	tsz = round2u(MAX_ALLOC / nmem);
	if ( osiz > tsz )
		return NULL;
	len = osiz * nmem;
	m = malloc(len);
	if ( m )
		memset(m, 0, len);
	return m;
}


#ifndef CAT_MIN_ALLOC_SHRINK
#define CAT_MIN_ALLOC_SHRINK 128
#endif  /* CAT_MIN_ALLOC_SHRINK */
static void shrink_alloc_block(struct memblk *mb, size_t sz)
{
	size_t nbsz;
	struct memblk *nmb;
	union align_u *lenp;

	/* caller should assure that sz is a multiple of UNITSIZE */
	abort_unless(sz % UNITSIZE == 0);
	nbsz = MBSIZE(mb) - sz;
	if ( nbsz < MINSZ )
		return;

	lenp = PTR2U(mb, MBSIZE(mb));
	if ( !(lenp->sz & ALLOC_BIT) ) {
		/* expand the next block?  OK if small since no fragmentation */
		nbsz += MBSIZE(lenp);
		l_rem(&((struct memblk *)lenp)->mb_entry);
		/* remove the block and fall through to create the new one */
	} else if ( nbsz < CAT_MIN_ALLOC_SHRINK ) {
		/* only shrink current block if size savings is worth it */
		return;
	} 
	nmb = (struct memblk *)PTR2U(mb, sz);
	set_free_mb(nmb, nbsz, PREV_ALLOC_BIT);
	l_ins(curpos, &nmb->mb_entry);
	mb->mb_len.sz = sz | (mb->mb_len.sz & CTLBMASK);
}


void *realloc(void *omem, size_t newamt)
{
	void *nmem = NULL;
	struct memblk *mb;
	union align_u *lenp;
	size_t tsz;

	if ( newamt == 0 ) {
		free(omem);
		return NULL;
	}

	if ( !omem )
		return malloc(newamt);

	tsz = round2u(newamt);
	if ( (tsz < newamt) || (tsz > MAX_ALLOC - UNITSIZE) )
		return NULL;
	newamt = tsz + UNITSIZE;

	mb = ptr2mb(omem);
	if ( mb->mb_len.sz >= newamt ) {
		shrink_alloc_block(mb, newamt);
		return omem;
	}

	/* See if we can just add the next adjacent block */
	lenp = PTR2U(mb, MBSIZE(mb));
	if ( !(lenp->sz & ALLOC_BIT) )  {
		tsz = MBSIZE(mb) + MBSIZE(lenp);
		if ( tsz > newamt ) {
			set_free_mb(mb, tsz, mb->mb_len.sz & CTLBMASK);
			return mb2ptr(mb);
		}
	}

	/* at this point we need a completely new block and must copy */
	nmem = malloc(newamt);
	if ( nmem ) { 
		/* may copy more than the original alloc, but still in bounds */
		memcpy(nmem, omem, MBSIZE(mb));
		free(omem);
	}

	return nmem;
}


/* TODO:  replace this with something more reliable */
static int exit_status = 0;	/* For debuggers to be able to inspect */
void exit(int status)
{
	exit_status = status;
	abort();
}


/* TODO:  replace this with something more reliable */
void abort(void)
{
	int a;

	/* Actions that tend to cause aborts in compiler implementations */
	*(char *)0 = 1;	/* Null pointer dereference */
	a = 1 / 0;	/* Integer divide by zero */

	/* worst case scenario: endless loop */
	for (;;) ;
}


#endif /* !CAT_USE_STDLIB */
