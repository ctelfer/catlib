#include <cat/cat.h>

#if !CAT_HAS_FIXED_WIDTH
#include <stdio.h>
int main(int argc, char *argv[]) { 
	printf("No fixed width integers on this platform\n");
	return 0;
}

#else /* !CAT_HAS_FIXED_WIDTH */

#include <cat/dynmem.h>
#include <cat/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NWORDS		(1024 * 1024 * 2)
#define MAXALLOCS	32
#define NITER		128
#define PRINTPERIOD	1

unsigned long Memory[NWORDS];
struct tlsf Tlsf;


void printblock(void *obj, void *ctx)
{
	struct tlsf_block_fake *block = obj;
	printf("\t%c%c: %u : %p\n", 
		(block->allocated ? 'A' : '-'),
		(block->prev_allocated ? 'P' : '-'),
		block->size, block->ptr);
}


void printpool(void *obj, void *ctx)
{
	struct tlsfpool *pool = obj;
	int *poolno = ctx;
	*poolno += 1;
	printf("Pool %d\n", *poolno);
	tlsf_each_block(pool, printblock, NULL);
}


void printmem(struct tlsf *tlsf, const char *title)
{
	int poolno = 0;
	printf("-- %s --\n", title);
	tlsf_each_pool(tlsf, printpool, &poolno);
	printf("\n");
}


void getmem(struct tlsf *tlsf, struct raw *r, size_t len)
{
	if (r->data)
		tlsf_free(tlsf, r->data);
	r->data = tlsf_malloc(tlsf, len);
	if (r->data)
		r->len = len;
}


void freemem(struct tlsf *tlsf, struct raw *r)
{
	tlsf_free(tlsf, r->data);
	r->data = NULL;
	r->len = 0;
}

void resizemem(struct tlsf *tlsf, struct raw *r, size_t len)
{
	void *d = tlsf_realloc(tlsf, r->data, len);
	if (len == 0) {
		r->data = NULL;
		len = 0;
	} else if (d != NULL) {
		r->data = d;
		r->len = len;
	}
}


void freeallmem(struct tlsf *tlsf, struct raw *rarr, size_t ralen)
{
	size_t i;
	for ( i = 0; i < ralen; i++ )
		freemem(tlsf, rarr + i);
}


void printraw(struct raw *r, size_t ralen)
{
	unsigned int i;
	printf("Allocated slots\n");
	for ( i = 0; i < ralen; i++ )
		if (r[i].data != NULL)
			printf("\tSlot %d: %p:%u\n", i, r[i].data, r[i].len);
}


void doit()
{

	struct raw allocs[MAXALLOCS] = { 0 };

	printmem(&Tlsf, "Initial state");
	getmem(&Tlsf, &allocs[0], 128);
	printmem(&Tlsf, "First alloc");
	getmem(&Tlsf, &allocs[1], 7);
	getmem(&Tlsf, &allocs[2], 7);
	printmem(&Tlsf, "Some more allocs");
	freemem(&Tlsf, &allocs[1]);
	printmem(&Tlsf, "First free");
	getmem(&Tlsf, &allocs[3], 48);
	printmem(&Tlsf, "Another alloc");
	freemem(&Tlsf, &allocs[2]);
	printmem(&Tlsf, "more free");
	freeallmem(&Tlsf, allocs, array_length(allocs));
	printmem(&Tlsf, "freed all");
}


void doit2()
{
	int i, nops, len, idx, op;
	struct raw allocs[MAXALLOCS] = { 0 };
	printmem(&Tlsf, "Initial state");
	char str[256];

	for (i = 1; i <= NITER; i++) { 
		idx = abs(rand()) % MAXALLOCS;
		op = abs(rand()) % 2;
		sprintf(str, "Iter %d -> %s %d, (%u)", i, 
			op ? 
				"free" : 
				(allocs[idx].data ? "free/alloc" : "alloc"), 
			idx, len);
		if (op == 0) {  /* alloc */
			len = abs(rand()) % 8192;
			getmem(&Tlsf, &allocs[idx], len);
		} else { 
			len = 0;
			freemem(&Tlsf, &allocs[idx]);
		}
		printraw(allocs, array_length(allocs));
		printmem(&Tlsf, str);
	}

	printmem(&Tlsf, "Final before clear");
	freeallmem(&Tlsf, allocs, array_length(allocs));
	printmem(&Tlsf, "Final");
}


void doit3() /* realloc tests */
{
	struct raw allocs[MAXALLOCS] = { 0 };

	printmem(&Tlsf, "Initial state for realloc tests");
	getmem(&Tlsf, &allocs[0], 280);
	printmem(&Tlsf, "init -> 280");
	resizemem(&Tlsf, &allocs[0], 290);
	printmem(&Tlsf, "resize -> 290");
	resizemem(&Tlsf, &allocs[0], 270);
	printmem(&Tlsf, "resize -> 270");
	resizemem(&Tlsf, &allocs[0], 310);
	printmem(&Tlsf, "resize -> 310");
	freeallmem(&Tlsf, allocs, array_length(allocs));
	printmem(&Tlsf, "freed all");

	getmem(&Tlsf, &allocs[0], 12);
	getmem(&Tlsf, &allocs[1], 12);
	getmem(&Tlsf, &allocs[2], 12);
	freemem(&Tlsf, &allocs[1]);
	printmem(&Tlsf, "After 3 allocations & 1 free");
	printraw(allocs, array_length(allocs));
	resizemem(&Tlsf, &allocs[0], 28);
	printmem(&Tlsf, "After resize");
	printraw(allocs, array_length(allocs));
	freeallmem(&Tlsf, allocs, array_length(allocs));
	printmem(&Tlsf, "freed all");


	getmem(&Tlsf, &allocs[0], 12);
	printmem(&Tlsf, "After 1 allocations: should split next block");
	printraw(allocs, array_length(allocs));
	resizemem(&Tlsf, &allocs[0], 28);
	printmem(&Tlsf, "After resize");
	freeallmem(&Tlsf, allocs, array_length(allocs));
	printmem(&Tlsf, "freed all");

}


int main(int argc, char *argv[])
{
	srand(time(NULL));
	tlsf_init(&Tlsf);
	tlsf_add_pool(&Tlsf, Memory, sizeof(Memory));

	doit();
	doit2();
	doit3();
	return 0;
} 

#endif /* !CAT_HAS_FIXED_WIDTH */
