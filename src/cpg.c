#include <cat/cpg.h>
#include <cat/str.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define STR(_state, _cursor) ((_state)->buf + (_cursor)->i)
#define CHAR(_state, _cursor) ((_state)->buf[(_cursor)->i])
#define CHARI(_state, _cursor, _off) ((_state)->buf[(_cursor)->i + _off])
#define NODE(_peg, _idx) (&(_peg)->nodes[_idx])
#define NODE_VALID_IDX(_peg, _idx) \
	((_idx) >= 0 && (_idx) < (_peg)->max_nodes)
#define NODE_VALID(_peg, _idx, _type) \
	(NODE_VALID_IDX(_peg, _idx) && ((_peg)->nodes[_idx].pn_type == _type))


int cpg_init(struct cpg_state *state, struct peg_grammar *peg,
	     int (*getc)(void *in))
{
	abort_unless(state != NULL);

	state->debug = 0;
	state->depth = 0;
	state->in = NULL;
	state->buf = malloc(4096);
	if ( state->buf == NULL )
		return -1;
	state->buflen = 4096;
	state->getc = getc;
	state->peg = peg;
	cpg_reset(state);

	return 0;
}


static int cpg_getc(struct cpg_state *state)
{
	void *newbuf;
	int c;

	if ( state->cur.i >= state->eof ) {
		abort_unless(state->cur.i == state->eof);
		return EOF;
	}

	if ( state->cur.i >= state->buflen ) {
		abort_unless(state->cur.i == state->buflen);
		newbuf = realloc(state->buf, state->buflen + 4096);
		if ( newbuf == NULL ) 
			return -2;
		state->buf = newbuf;
		memset((uchar *)newbuf + state->buflen, '\0', 4096);
		state->buflen += 4096;
	}

	if ( state->cur.i >= state->readidx ) {
		abort_unless(state->cur.i == state->readidx);
		c = (*state->getc)(state->in);
		if ( c == EOF ) {
			state->eof = state->cur.i;
			return EOF;
		}
		state->buf[state->readidx++] = c;
	}

	if ( state->buf[state->cur.i - 1] == '\n' )
		state->cur.line++;
	return state->buf[state->cur.i++];
}


/*
 * Forward definitions
 */
static int cpg_match_expression(struct cpg_state *state, int expr, void *aux);
static int cpg_match_sequence(struct cpg_state *state, int seq, void *aux);
static int cpg_match_primary(struct cpg_state *state, int pri, void *aux);
static int cpg_match_identifier(struct cpg_state *state, int id, void *aux);
static int cpg_match_token(struct cpg_state *state, int id, void *aux);
static int cpg_match_literal(struct cpg_state *state, int lit, void *aux);
static int cpg_match_class(struct cpg_state *state, int cls, void *aux);


static int cpg_match_expression(struct cpg_state *state, int seq, void *aux)
{
	struct peg_grammar *peg = state->peg;
	int rv;

	for ( ; seq >= 0 ; seq = NODE(peg, seq)->pn_next ) {
		rv = cpg_match_sequence(state, seq, aux);
		if ( rv != 0 )
			return rv;
	}

	return 0;
}


static int cpg_match_sequence(struct cpg_state *state, int seq, void *aux)
{
	struct peg_grammar *peg = state->peg;
	struct cpg_cursor oc = state->cur;
	struct peg_node *pn;
	int pri;
	int rv;

	abort_unless(NODE_VALID(peg, seq, PEG_SEQUENCE));
	pn = NODE(peg, seq);

	for ( pri = pn->ps_pri; pri >= 0; pri = NODE(peg, pri)->pn_next ) {
		rv = cpg_match_primary(state, pri, aux);
		if ( rv <= 0 ) {
			state->cur = oc;
			return rv;
		}
	}
	return 1;
}


static int cpg_match_primary(struct cpg_state *state, int pri, void *aux)
{
	struct peg_grammar *peg = state->peg;
	struct cpg_cursor oc = state->cur;
	struct peg_node *pn;
	uint nmatch = 0;
	int repeat;
	int mtype;
	int rv = -1;
	struct raw r;
	int act_rv;

	abort_unless(NODE_VALID(peg, pri, PEG_PRIMARY));
	pn = NODE(peg, pri);
	repeat = (pn->pp_suffix == PEG_ATTR_STAR ||
		  pn->pp_suffix == PEG_ATTR_PLUS);

	abort_unless(NODE_VALID_IDX(peg, pn->pp_match));
	mtype = NODE(peg, pn->pp_match)->pn_type;

	do {
		switch ( mtype ) {
		case PEG_SEQUENCE:
			rv = cpg_match_expression(state, pn->pp_match, aux);
			break;
		case PEG_IDENTIFIER:
			rv = cpg_match_identifier(state, pn->pp_match, aux);
			break;
		case PEG_LITERAL:
			rv = cpg_match_literal(state, pn->pp_match, aux);
			break;
		case PEG_CLASS:
			rv = cpg_match_class(state, pn->pp_match, aux);
			break;
		default:
			abort_unless(0);
		}
		if ( rv > 0 )
			++nmatch;
	} while ( rv > 0 && repeat );

	if ( rv < 0 )
		return rv;

	switch ( pn->pp_suffix ) {
	case PEG_ATTR_NONE: rv = (nmatch == 1); break;
	case PEG_ATTR_QUESTION: rv = 1; break;
	case PEG_ATTR_STAR: rv = 1; break;
	case PEG_ATTR_PLUS: rv = (nmatch >= 1); break;
	default:
		abort_unless(0);
	}

	if ( pn->pp_prefix != PEG_ATTR_NONE ) {
		state->cur = oc;
		if ( pn->pp_prefix == PEG_ATTR_NOT )
			rv = !rv;
		else
			abort_unless(pn->pp_prefix == PEG_ATTR_AND);
	} else if ( rv <= 0 ) {
		state->cur = oc;
	}

	if ( rv > 0 && pn->pp_action == PEG_ACT_CALLBACK ) {
		r.data = state->buf + oc.i;
		r.len = state->cur.i - oc.i;
		act_rv = (*pn->pn_action_cb)(pri, &r, aux);
		if ( act_rv < 0 )
			return act_rv;
	}

	return rv;
}


static void pad(struct cpg_state *state)
{
	int i;
	for ( i = 0; i < state->depth; ++i )
		fputc(' ', stderr);
}


static int cpg_match_identifier(struct cpg_state *state, int id, void *aux)
{
	int rv;
	struct peg_grammar *peg = state->peg;
	struct peg_node *pn = NODE(state->peg, id);
	if ( state->debug ) {
		pad(state);
		fprintf(stderr, ">:'%s' at <L:%d/P:%d>\n",
			pn->pn_str.data, state->cur.line, state->cur.i);
	}
	++state->depth;
	if ( PEG_IDX_IS_TOKEN(peg, pn->pi_def) ) {
		rv = cpg_match_token(state, id, aux);
	} else {
		abort_unless(NODE_VALID(peg, pn->pi_def, PEG_DEFINITION));
		rv = cpg_match_expression(state, NODE(peg, pn->pi_def)->pd_expr,
					  aux);
	}
	--state->depth;
	if ( state->debug ) {
		if ( rv < 0 ) {
			pad(state);
			fprintf(stderr, "E:'%s' at <L:%d/P:%d>\n",
				pn->pn_str.data, state->cur.line, state->cur.i);
		} else if ( rv == 0 ) {
			pad(state);
			fprintf(stderr, "F:'%s' at <L:%d/P:%d>\n",
				pn->pn_str.data, state->cur.line, state->cur.i);
		} else {
			pad(state);
			fprintf(stderr, "S:'%s' at <L:%d/P:%d>\n",
				pn->pn_str.data, state->cur.line, state->cur.i);
		}
	}

	return rv;
}


static int cpg_match_token(struct cpg_state *state, int id, void *aux)
{
	struct peg_grammar *peg = state->peg;
	struct cpg_cursor oc = state->cur;
	int c;

	abort_unless(NODE_VALID(peg, id, PEG_IDENTIFIER));

	if ( state->debug ) {
		pad(state);
		fprintf(stderr, "T>:'%d' at <L:%d/P:%d>\n",
			id, state->cur.line, state->cur.i);
	}

	c = cpg_getc(state);
	if ( c != EOF &&  c == PEG_TOKEN_ID(NODE(peg, id)->pi_def) ) {
		if ( state->debug ) {
			pad(state);
			fprintf(stderr, "TS:'%d' at <L:%d/P:%d>\n",
				id, state->cur.line, state->cur.i);
		}
		return 1;
	} else {
		state->cur = oc;
		if ( state->debug ) {
			pad(state);
			fprintf(stderr, "TF:'%d' at <L:%d/P:%d>\n",
				id, state->cur.line, state->cur.i);
		}
		return 0;
	}
}


static int cpg_match_literal(struct cpg_state *state, int lit, void *aux)
{
	struct peg_grammar *peg = state->peg;
	struct cpg_cursor oc = state->cur;
	struct peg_node *pn = NODE(peg, lit);
	int c;
	int i;

	if ( state->debug ) {
		pad(state);
		fprintf(stderr, "L>:'%s' at <L:%d/P:%d>\n",
			pn->pn_str.data, state->cur.line, state->cur.i);
	}

	for ( i = 0 ; i < pn->pl_value.len ; ++i ) {
		c = cpg_getc(state);
		if ( c == EOF || c != pn->pl_value.data[i] )
			goto fail;
	}

	if ( state->debug ) {
		pad(state);
		fprintf(stderr, "LS:'%s' at <L:%d/P:%d>\n",
			pn->pn_str.data, state->cur.line, state->cur.i);
	}

	return 1;

fail:
	if ( state->debug ) {
		pad(state);
		fprintf(stderr, "LF:'%s' at <L:%d/P:%d>\n",
			pn->pn_str.data, state->cur.line, state->cur.i);
	}
	state->cur = oc;
	return 0;

}


static int cpg_match_class(struct cpg_state *state, int cls, void *aux)
{
	struct peg_grammar *peg = state->peg;
	struct cpg_cursor oc = state->cur;
	int c;

	abort_unless(NODE_VALID(peg, cls, PEG_CLASS));

	if ( state->debug ) {
		pad(state);
		fprintf(stderr, "C>:'%d' at <L:%d/P:%d>\n",
			cls, state->cur.line, state->cur.i);
	}

	c = cpg_getc(state);
	if ( c != EOF && cset_contains(NODE(peg, cls)->pc_cset, c) ) {
		if ( state->debug ) {
			pad(state);
			fprintf(stderr, "CS:'%d' at <L:%d/P:%d>\n",
				cls, state->cur.line, state->cur.i);
		}
		return 1;
	} else {
		state->cur = oc;
		if ( state->debug ) {
			pad(state);
			fprintf(stderr, "CF:'%d' at <L:%d/P:%d>\n",
				cls, state->cur.line, state->cur.i);
		}
		return 0;
	}
}


void cpg_set_debug_level(struct cpg_state *state, int level)
{
	state->debug = level;
}


int cpg_parse(struct cpg_state *state, void *in, void *aux)
{
	int rv;
	state->in = in;
	rv = cpg_match_identifier(state, state->peg->start_node, aux);
	if ( rv >= 0 && (state->cur.i < state->eof) )
		rv = -1;
	return rv;
}


void cpg_reset(struct cpg_state *state)
{
	if ( state->buf != NULL )
		memset(state->buf, '\0', state->buflen);
	state->cur.i = 1;
	state->cur.line = 1;
	state->readidx = 1;
	state->eof = (uint)-1;
}


void cpg_fini(struct cpg_state *state)
{
	free(state->buf);
	state->buf = NULL;
	state->buflen = 0;
	state->in = NULL;
	state->readidx = 0;
	state->peg = NULL;
	state->getc = NULL;
	state->eof = 0;
	state->debug = 0;
}


