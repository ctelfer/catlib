#ifndef __peg_h
#define __peg_h

#include <cat/list.h>
#include <cat/rbtree.h>
#include <stdio.h>

enum {
	PEG_ERR_NONE = 0,
	PEG_ERR_NOMEM = 1,
	PEG_ERR_BAD_DEF = 2,
	PEG_ERR_DUP_DEF = 3,
	PEG_ERR_BAD_EXPR = 4,
	PEG_ERR_BAD_PRIMARY = 5,
	PEG_ERR_BAD_PAREXPR = 6,
	PEG_ERR_BAD_LITERAL = 7,
	PEG_ERR_UNDEF_LITERAL = 8,
	PEG_ERR_BAD_CLASS = 9,
	PEG_ERR_BAD_CHAR = 10,
	PEG_ERR_BAD_RANGE = 11,
	PEG_ERR_BAD_CODE = 12,
	PEG_ERR_LAST = PEG_ERR_BAD_CODE
};


struct peg_cursor {
	uint pos;
	uint line;
};

struct peg_node;
struct peg_def;
struct peg_expr;
struct peg_seq;
struct peg_primary;
struct peg_id;
struct peg_literal;
struct peg_class;
struct peg_code;

struct peg_grammar {
	const char *input;
	size_t inlen;
	struct list def_list;
	struct rbtree def_table;
	struct list id_list;
	struct peg_def *start;
	int err;
	struct peg_cursor eloc;
};

enum {
	PEG_NONE,
	PEG_DEFINITION,
	PEG_EXPRESSION,
	PEG_SEQUENCE,
	PEG_PRIMARY,
	PEG_IDENTIFIER,
	PEG_LITERAL,
	PEG_CLASS,
	PEG_CODE
};

struct peg_node {
	int type;
	struct peg_cursor loc;
	uint len;
	uint nlines;
};

struct peg_def {
	struct peg_node node;
	struct rbnode rbe;
	struct list le;
	struct peg_id *id;
	struct peg_expr *expr;
};

struct peg_expr {
	struct peg_node node;
	struct peg_seq *seq;
};

struct peg_seq {
	struct peg_node node;
	struct peg_primary *pri;
	struct peg_code *code;
	struct peg_seq *next;
};

enum {
	PEG_ATTR_NONE = 0,
	PEG_ATTR_AND,
	PEG_ATTR_NOT,
	PEG_ATTR_QUESTION,
	PEG_ATTR_STAR,
	PEG_ATTR_PLUS
};

struct peg_primary {
	struct peg_node node;
	int prefix;
	union peg_node_u *match;
	int suffix;
	struct peg_primary *next;
};

struct peg_id {
	struct peg_node node;
	struct list le;
	struct raw name;
	struct peg_def *def;
};

struct peg_literal {
	struct peg_node node;
};

struct peg_class {
	struct peg_node node;
	byte_t cset[32];
};

struct peg_code {
	struct peg_node node;
};

union peg_node_u {
	struct peg_node node;
	struct peg_def def;
	struct peg_expr expr;
	struct peg_seq seq;
	struct peg_primary pri;
	struct peg_id id;
	struct peg_literal lit;
	struct peg_class cls;
	struct peg_code code;
};


int peg_parse(struct peg_grammar *peg, const char *string);

void peg_free_nodes(struct peg_grammar *peg);

void peg_reset(struct peg_grammar *peg);

const char *peg_err_message(int err);

void peg_print(struct peg_grammar *peg, FILE *out);

#endif /* __peg_h */
