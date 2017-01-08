#ifndef __peg_h
#define __peg_h

#include <cat/cat.h>
#include <stdio.h>

struct peg_node;
struct peg_grammar;

typedef int (*peg_action_f)(int, struct raw *, void *);

struct peg_node {
	int pn_type;
	struct raw pn_str;
	int pn_next;
	int pn_subnode;
	uint pn_line;
	ushort pn_status;
	uchar pn_flag1;
	uchar pn_flag2;
	peg_action_f pn_action_cb;
};

#define pn_is_type(_peg, _idx, _type) \
	((_idx) >= 0 && (_idx < (_peg)->max_nodes) && \
	 (_peg)->nodes[(_idx)].pn_type == (_type))

#define pd_id pn_subnode
#define pd_expr pn_next	/* defs aren't in lists */

#define ps_pri pn_subnode

#define pp_match pn_subnode
#define pp_action pn_status
#define pp_label pn_str
#define pp_code  pn_str
#define pp_prefix pn_flag1
#define pp_suffix pn_flag2

#define pi_name pn_str
#define pi_refcnt pn_status
#define pi_def pn_subnode

#define pl_value pn_str

#define pc_cset_raw pn_str
#define pc_cset pn_str.data
#define pc_cset_size pn_str.len

struct peg_grammar {
	struct peg_node *nodes;
	uint max_nodes;
	uint num_nodes;
	uint start_node;
	int dynamic;
};

enum {
	PEG_ERR_NONE = 0,
	PEG_ERR_NOMEM = 1,
	PEG_ERR_BAD_DEF = 2,
	PEG_ERR_DUP_DEF = 3,
	PEG_ERR_BAD_EXPR = 4,
	PEG_ERR_BAD_PRIMARY = 5,
	PEG_ERR_BAD_PAREXPR = 6,
	PEG_ERR_BAD_LITERAL = 7,
	PEG_ERR_UNDEF_ID = 8,
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

struct peg_grammar_parser {
	const char *input;
	uint input_len;
	uint len;
	uint nlines;
	struct peg_grammar *peg;
	int err;
	struct peg_cursor eloc;
};

enum {
	PEG_NONE,
	PEG_DEFINITION,
	PEG_SEQUENCE,
	PEG_PRIMARY,
	PEG_IDENTIFIER,
	PEG_LITERAL,
	PEG_CLASS
};

enum {
	PEG_ATTR_NONE = 0,
	PEG_ATTR_AND,
	PEG_ATTR_NOT,
	PEG_ATTR_QUESTION,
	PEG_ATTR_STAR,
	PEG_ATTR_PLUS
};

enum {
	PEG_ACT_NONE = 0,
	PEG_ACT_CODE,
	PEG_ACT_LABEL,
	PEG_ACT_CALLBACK
};

int peg_parse(struct peg_grammar_parser *pgp, struct peg_grammar *peg,
	      const char *string, uint len);

void peg_free_nodes(struct peg_grammar *peg);

char *peg_err_string(struct peg_grammar_parser *pgp, char *buf, size_t blen);

void peg_print(struct peg_grammar *peg, FILE *out);

int peg_action_set(struct peg_grammar *peg, char *label, peg_action_f cb);

#endif /* __peg_h */
