#ifndef __cpg_h
#define __cpg_h

#include <cat/peg.h>

struct cpg_cursor {
	uint i;
	uint line;
};


struct cpg_state {
	int debug;
	int depth;
	void *in;
	struct cpg_cursor cur;
	uchar *buf;
	uint buflen;
	uint readlen;
	uint eof;
	int (*getc)(void *in);
	struct peg_grammar *peg;
};


int cpg_init(struct cpg_state *state, struct peg_grammar *peg,
	     int (*getc)(void *in));

void cpg_set_debug_level(struct cpg_state *state, int level);

int cpg_parse(struct cpg_state *state, void *in, void *aux);

void cpg_reset(struct cpg_state *state);

void cpg_fini(struct cpg_state *state);

#endif /* __cpg_h */
