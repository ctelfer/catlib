/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/cat.h>
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
struct dynmem Dm;


void printblock(void *obj, void *ctx)
{
	struct dynmem_block_fake *block = obj;
	printf("\t%c%c: %u : %p\n", 
		(block->allocated ? 'A' : '-'),
		(block->prev_allocated ? 'P' : '-'),
		(uint)block->size, block->ptr);
}


void printpool(void *obj, void *ctx)
{
	struct dynmempool *pool = obj;
	int *poolno = ctx;
	*poolno += 1;
	printf("Pool %d\n", *poolno);
	dynmem_each_block(pool, printblock, NULL);
}


void printmem(struct dynmem *dm, const char *title)
{
	int poolno = 0;
	printf("-- %s --\n", title);
	dynmem_each_pool(dm, printpool, &poolno);
	printf("\n");
}


void getmem(struct dynmem *dm, struct raw *r, size_t len)
{
	if (r->data)
		dynmem_free(dm, r->data);
	r->data = dynmem_malloc(dm, len);
	if (r->data)
		r->len = len;
}


void freemem(struct dynmem *dm, struct raw *r)
{
	dynmem_free(dm, r->data);
	r->data = NULL;
	r->len = 0;
}


void freeallmem(struct dynmem *dm, struct raw *rarr, size_t ralen)
{
	size_t i;
	for ( i = 0; i < ralen; i++ )
		freemem(dm, rarr + i);
}


void printraw(struct raw *r, size_t ralen)
{
	unsigned int i;
	printf("Allocated slots\n");
	for ( i = 0; i < ralen; i++ )
		if (r[i].data != NULL)
			printf("\tSlot %d: %p:%u\n", i, r[i].data, 
                               (uint)r[i].len);
}


void doit()
{

	struct raw allocs[MAXALLOCS] = { 0 };

	printmem(&Dm, "Initial state");
	getmem(&Dm, &allocs[0], 128);
	printmem(&Dm, "First alloc");
	getmem(&Dm, &allocs[1], 7);
	getmem(&Dm, &allocs[2], 7);
	printmem(&Dm, "Some more allocs");
	freemem(&Dm, &allocs[1]);
	printmem(&Dm, "First free");
	getmem(&Dm, &allocs[3], 48);
	printmem(&Dm, "Another alloc");
	freemem(&Dm, &allocs[2]);
	printmem(&Dm, "more free");
	freeallmem(&Dm, allocs, array_length(allocs));
	printmem(&Dm, "freed all");
}


void doit2()
{
	int i, nops, len, idx, op;
	struct raw allocs[MAXALLOCS] = { 0 };
	printmem(&Dm, "Initial state");
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
			getmem(&Dm, &allocs[idx], len);
		} else { 
			len = 0;
			freemem(&Dm, &allocs[idx]);
		}
		printraw(allocs, array_length(allocs));
		printmem(&Dm, str);
	}

	printmem(&Dm, "Final before clear");
	freeallmem(&Dm, allocs, array_length(allocs));
	printmem(&Dm, "Final");
}


int main(int argc, char *argv[])
{
	srand(time(NULL));
	dynmem_init(&Dm);
	dynmem_add_pool(&Dm, Memory, sizeof(Memory));

	doit();
	doit2();
	return 0;
} 
