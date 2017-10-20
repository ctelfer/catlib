#include <cat/cat.h>
#include <cat/aux.h>
#include <cat/str.h>
#include <cat/peg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

/*
 * This is the PEG grammar format in PEG format:
 *
 * # Hierarchical syntax
 * Grammar <- Spacing Definition+ EndOfFile
 * Definition <- Identifier LEFTARROW Expression
 * Expression <- Sequence (SLASH Sequence)*
 * Sequence <- Prefix*
 * Prefix <- (AND / NOT)? Suffix 
 *    # XXX Codeblock? added: see below XXX
 * Suffix <- Primary (QUESTION / STAR / PLUS)? (Codeblock / ActionLabel)?
 * Primary <- Identifier !LEFTARROW
 *            / OPEN Expression CLOSE
 *            / Literal / Class / DOT
 *
 * # XXX
 * # Added for parser generation: contains the C code to run on match
 * #
 * Codeblock <- '{' (Codenonstring / Literal / CComment)* '}'
 * Codenonstring <- !["'] . 
 * CComment <- "/ *" (!"* /" .)* "* /"
 *   # No spaces-^----------^------^
 *   # Added to embed in C comments
 * ActionLabel <- ':' Identifier
 * # XXX
 *
 * # Lexical syntax
 * Identifier <- IdentStart IdentCont* Spacing
 * IdentStart <- [a-zA-Z_]
 * IdentCont <- IdentStart / [0-9]
 * Literal <- [’] (![’] Char)* [’] Spacing
 *            / ["] (!["] Char)* ["] Spacing
 * Class <- ’[’ (!’]’ Range)* ’]’ Spacing
 * Range <- Char ’-’ Char / Char
 * Char <- ’\\’ [nrt’"\[\]\\]
 *         / ’\\’ [0-2][0-7][0-7]
 *         / ’\\’ [0-7][0-7]?
 *         / !’\\’ .
 *
 * LEFTARROW <- ’<-’ Spacing
 * SLASH <- ’/’ Spacing
 * AND <- ’&’ Spacing
 * NOT <- ’!’ Spacing
 * QUESTION <- ’?’ Spacing
 * STAR <- ’*’ Spacing
 * PLUS <- ’+’ Spacing
 * OPEN <- ’(’ Spacing
 * CLOSE <- ’)’ Spacing
 * DOT <- ’.’ Spacing
 * Spacing <- (Space / Comment)*
 * Comment <- ’#’ (!EndOfLine .)* EndOfLine
 * Space <- ’ ’ / ’\t’ / EndOfLine
 * EndOfLine <- ’\r\n’ / ’\n’ / ’\r’
 * EndOfFile <- !.
 */

#define STR(_pgp, _cursor) ((_pgp)->input + (_cursor)->pos)
#define CHAR(_pgp, _cursor) ((_pgp)->input[(_cursor)->pos])
#define CHARI(_pgp, _cursor, _off) ((_pgp)->input[(_cursor)->pos + (_off)])
#define node2idx(_peg, _node) ((_node) - (_peg)->nodes)
#define NODE(_peg, _i) (&(_peg)->nodes[(_i)])

static int initialized = 0;
static byte_t cs_space[32];
static byte_t cs_eol[32];
static byte_t cs_id_start[32];
static byte_t cs_id_cont[32];
static byte_t cs_special[32];
static byte_t cs_digit0to2[32];
static byte_t cs_digit0to7[32];
static byte_t cs_ccode[32];


static struct peg_node *find_id(struct peg_grammar *peg, struct raw *r)
{
	int i;
	for ( i = 0; i < peg->max_nodes; ++i )
		if ( pn_is_type(peg, i, PEG_IDENTIFIER) &&
		     !cmp_raw(r, &NODE(peg, i)->pi_name) )
		       return NODE(peg, i);
	return NULL;
}


static void free_str(struct raw *r)
{
	free(r->data);
	r->data = NULL;
	r->len = 0;
}


static void peg_node_free(struct peg_grammar *peg, int nn)
{
	struct peg_node *pn;

	if ( nn < 0 || nn >= peg->max_nodes )
		return;

	pn = NODE(peg, nn);
	switch ( pn->pn_type ) {
	case PEG_DEFINITION:
		peg_node_free(peg, pn->pd_id);
		peg_node_free(peg, pn->pd_expr);
		break;
	case PEG_SEQUENCE:
		peg_node_free(peg, pn->ps_pri);
		peg_node_free(peg, pn->pn_next);
		break;
	case PEG_PRIMARY:
		peg_node_free(peg, pn->pp_match);
		peg_node_free(peg, pn->pn_next);
		if ( pn->pp_action != PEG_ACT_NONE )
			free_str(&pn->pp_label);
		break;
	case PEG_IDENTIFIER:
		--pn->pi_refcnt;
		if ( pn->pi_refcnt > 0 )
			return;
		free_str(&pn->pi_name);
		break;
	case PEG_LITERAL:
		free_str(&pn->pl_value);
		break;
	case PEG_CLASS:
		free_str(&pn->pc_cset_raw);
		break;
	default:
		return;
	}

	abort_unless(peg->num_nodes > 0);
	--peg->num_nodes;
	pn->pn_type = PEG_NONE;
}


static int peg_node_new(struct peg_grammar *peg, int type, uint line)
{
	struct peg_node *new_nodes;
	struct peg_node *pn;
	size_t osize, nsize;
	uint i;
	uint start;

	if ( peg->num_nodes == peg->max_nodes ) {
		if ( !peg->dynamic )
			return -1;
		start = peg->max_nodes;
		if ( peg->max_nodes == 0 ) {
			peg->max_nodes = 8;
			nsize = 16 * sizeof(struct peg_node);
		} else {
			osize = peg->max_nodes * sizeof(struct peg_node);
			nsize = osize * 2;
			if ( nsize < osize || peg->max_nodes > INT_MAX / 2 )
				return -1;
		}
		new_nodes = realloc(peg->nodes, nsize);
		if ( new_nodes == NULL )
			return -1;
		peg->nodes = new_nodes;
		peg->max_nodes *= 2;
		for ( i = start; i < peg->max_nodes; ++i )
			NODE(peg, i)->pn_type = PEG_NONE;
	}

	abort_unless(pn_is_type(peg, peg->num_nodes, PEG_NONE));
	i = peg->num_nodes++;
	pn = NODE(peg, i);
	pn->pn_type = type;
	pn->pn_line = line;
	pn->pn_next = -1;
	pn->pn_subnode = -1;
	pn->pn_status = 0;
	pn->pn_action_cb = NULL;
	return i;
}


static void skip_space(struct peg_grammar_parser *pgp, struct peg_cursor *pc)
{
	int in_comment = 0;

	while ( in_comment || CHAR(pgp, pc) == '#' || 
		cset_contains(cs_space, CHAR(pgp, pc)) ) {
		if ( CHAR(pgp, pc) == '#' ) {
			in_comment = 1;
		} else if ( cset_contains(cs_eol, CHAR(pgp, pc)) ) {
			pc->line += 1;
			if ( CHAR(pgp, pc) == '\r' && 
			     CHARI(pgp, pc, 1) == '\n' )
				pc->pos += 1;
			in_comment = 0;
		}
		pc->pos += 1;
	}
}


static int string_match(struct peg_grammar_parser *pgp, const char *pat,
			struct peg_cursor *pc)
{
	uint plen;

	plen = strlen(pat);
	abort_unless(pgp->input_len >= pc->pos);
	if ( plen > pgp->input_len - pc->pos )
		return 0;
	if ( strncmp(STR(pgp, pc), pat, plen) == 0 ) {
		pc->pos += plen;
		skip_space(pgp, pc);
		return 1;
	} else {
		return 0;
	}
}


static int copy_str(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		    size_t len, struct raw *r)
{
	r->data = malloc(len + 1);
	if ( r->data == NULL ) {
		pgp->err = PEG_ERR_NOMEM;
		return -1;
	}
	r->len = len;
	memcpy(r->data, STR(pgp, pc), len);
	r->data[len] = '\0';
	return 0;
}


static int parse_id(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
                    int *idp)
{
	int id;
        struct peg_node *pn;
	struct peg_grammar *peg = pgp->peg;
        struct raw name;

        if ( !cset_contains(cs_id_start, CHAR(pgp, pc)) )
                return 0;

        name.data = (char *)STR(pgp, pc);
        name.len = 1 + str_spn(name.data + 1, cs_id_cont);
	pn = find_id(peg, &name);
        if ( pn != NULL ) {
		++pn->pi_refcnt;
        } else {
                id = peg_node_new(peg, PEG_IDENTIFIER, pc->line);
                if ( id < 0 ) {
                        pgp->err = PEG_ERR_NOMEM;
                        return -1;
                }
		pn = NODE(peg, id);
                if ( copy_str(pgp, pc, name.len, &pn->pi_name) < 0 ) {
                        pgp->err = PEG_ERR_NOMEM;
                        peg_node_free(peg, id);
                        return -1;
                }
                pn->pi_def = -1;
                pn->pi_refcnt = 1;
        }

        *idp = node2idx(peg, pn);
        pc->pos += name.len;
        skip_space(pgp, pc);
        return 1;
}


static int parse_id_and_not_arrow(struct peg_grammar_parser *pgp,
				  struct peg_cursor *pc, int *idp)
{
	int rv;
	struct peg_cursor npc1 = *pc;
	struct peg_cursor npc2;
	int id;

	rv = parse_id(pgp, &npc1, &id);
	if ( rv <= 0 )
		return rv;
	npc2 = npc1;
	if ( string_match(pgp, "<-", &npc2) ) {
		peg_node_free(pgp->peg, id);
		return 0;
	}

	*pc = npc1;
	*idp = id;
	return 1;
}


static int parse_expr(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		      int *seqp);


static int parse_paren_expr(struct peg_grammar_parser *pgp,
			    struct peg_cursor *pc, int *exprp)
{
	int rv;
	struct peg_cursor npc = *pc;
	int expr = -1;

	if ( !string_match(pgp, "(", &npc) )
		return 0;

	rv = parse_expr(pgp, &npc, &expr);
	if ( rv < 0 )
		return -1;
	if ( rv == 0 )
		goto err;
	if ( !string_match(pgp, ")", &npc) )
		goto err;

	*pc = npc;
	*exprp = expr;
	return 1;

err:
	peg_node_free(pgp->peg, expr);
	pgp->err = PEG_ERR_BAD_PAREXPR;
	pgp->eloc = npc;
	return -1;
}


static int parse_char(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		      int *cp)
{
	int c;
	struct peg_cursor npc = *pc;

	c = CHAR(pgp, &npc);
	if ( c == '\0' )
		return 0;

	if ( c != '\\' ) {
		pc->pos += 1;
		if ( cset_contains(cs_eol, c) ) {
			pc->line += 1;
			if ( c == '\r' && CHAR(pgp, pc) == '\n' )
				pc->pos += 1;
		}
		*cp = c;
		return 1;
	} 

	npc.pos += 1;
	c = CHAR(pgp, &npc);
	if ( c == '\0' ) {
		pgp->err = PEG_ERR_BAD_CHAR;
		pgp->eloc = npc;
		return -1;
	}

	if ( cset_contains(cs_special, c) ) {
		switch (c) {
		case 'n': *cp = '\n'; break;
		case 'r': *cp = '\r'; break;
		case 't': *cp = '\t'; break;
		case '\'': *cp = '\''; break;
		case '"': *cp = '"'; break;
		case '[': *cp = '['; break;
		case ']': *cp = ']'; break;
		case '\\': *cp = '\\'; break;
		}
		npc.pos += 1;
		*pc = npc;
		return 1;
	}

	if ( !cset_contains(cs_digit0to7, c) ) {
		pgp->err = PEG_ERR_BAD_CHAR;
		pgp->eloc = *pc;
		return -1;
	}

	if ( cset_contains(cs_digit0to2, c) &&
	     cset_contains(cs_digit0to7, CHARI(pgp, &npc, 1)) && 
	     cset_contains(cs_digit0to7, CHARI(pgp, &npc, 2)) ) {
		c = ((c - '0') << 6) |
		    ((CHARI(pgp, &npc, 1) - '0') << 3) | 
		    (CHARI(pgp, &npc, 2) - '0');
		npc.pos += 3;
	} else {
		c = c - '0';
		npc.pos += 1;
		if ( cset_contains(cs_digit0to7, CHAR(pgp, &npc)) ) {
			c = (c << 3) | (CHAR(pgp, &npc) - '0');
			npc.pos += 1;
		}
	}

	*pc = npc;
	*cp = c;
	return 1;
}


static int parse_literal(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
			 int *litp)
{
	struct peg_grammar *peg = pgp->peg;
	struct peg_cursor npc = *pc;
	int nn;
	struct peg_node *pn;
	int c;
	int rv;
	uint i;
	char quote;
	struct raw value;

	quote = CHAR(pgp, &npc);
	if ( quote != '"' && quote != '\'' )
		return 0;
	npc.pos += 1;

	value.len = 0;
	do { 
		rv = parse_char(pgp, &npc, &c);
		if ( rv < 0 )
			goto err;
		if ( rv == 0 ) {
			pgp->err = PEG_ERR_BAD_LITERAL;
			goto err;
		}
		value.len += 1;
	} while ( c != quote );

	nn = peg_node_new(peg, PEG_LITERAL, pc->line);
	if ( nn < 0 ) {
		pgp->err = PEG_ERR_NOMEM;
		return -1;
	}
	value.data = malloc(value.len);
	if ( value.data == NULL ) {
		pgp->err = PEG_ERR_NOMEM;
		peg_node_free(peg, nn);
		return -1;
	}

	/* now copy/translate the string for real since we know */
	/* its true length and that it decodes correctly. */
	npc.pos = pc->pos + 1;
	npc.line = pc->line;
	for ( i = 0; i < value.len - 1; ++i ) {
		rv = parse_char(pgp, &npc, &c);
		abort_unless(rv > 0); /* tested above */
		value.data[i] = c;
	}
	value.data[i] = '\0';

	pn = NODE(peg, nn);
	pn->pl_value = value;
	pn->pl_value.len -= 1;

	pc->pos = npc.pos + 1; /* skip last quote */
	pc->line = npc.line;
	*litp = nn;
	skip_space(pgp, pc);
	return 1;

err:
	pgp->eloc = npc;
	return -1;
}


static int class_add_char(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
			  byte_t *cset)
{
	struct peg_cursor npc = *pc;
	int c1;
	int c2;
	int rv;

	rv = parse_char(pgp, &npc, &c1);
	if ( rv < 0 )
		return -1;
	if ( rv == 0 ) {
		pgp->err = PEG_ERR_BAD_CLASS;
		goto err;
	}

	if ( CHAR(pgp, &npc) != '-' ) {
		cset_add(cset, c1);
	} else {
		npc.pos += 1;
		rv = parse_char(pgp, &npc, &c2);
		if ( rv < 0 )
			return -1;
		if ( rv == 0 ) {
			pgp->err = PEG_ERR_BAD_RANGE;
			goto err;
		}
		while ( c1 <= c2 ) {
			cset_add(cset, c1);
			++c1;
		}
	}

	*pc = npc;
	return 1;

err:
	pgp->eloc = *pc;
	return -1;

}


static int parse_class(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		       int *clsp)
{
	struct peg_grammar *peg = pgp->peg;
	struct peg_cursor npc = *pc;
	char c;
	int nn;
	struct peg_node *cls;
	uchar *cset;

	if ( CHAR(pgp, &npc) != '[' && CHAR(pgp, &npc) != '.' )
		return 0;

	nn = peg_node_new(peg, PEG_CLASS, pc->line);
	if ( nn < 0 ) {
		pgp->err = PEG_ERR_NOMEM;
		return -1;
	}
	cset = malloc(32);
	if ( cset == NULL ) {
		peg_node_free(peg, nn);
		pgp->err = PEG_ERR_NOMEM;
		return -1;
	}

	/* treat . as a special class that matches almost everying */
	if ( CHAR(pgp, &npc) == '.' ) {
		cset_fill(cset);
		cset_rem(cset, '\0');
		goto match;
	} else {
		cset_clear(cset);
		npc.pos += 1;
	}

	while ( (c = CHAR(pgp, &npc)) != ']' && c != '\0' )
		if ( class_add_char(pgp, &npc, cset) < 0 )
			return -1;

	if ( c == '\0' ) {
		pgp->err = PEG_ERR_BAD_CLASS;
		pgp->eloc = npc;
		free(cset);
		peg_node_free(peg, nn);
		return -1;
	}

match:
	cls = NODE(peg, nn);
	cls->pc_cset = cset;
	cls->pc_cset_size = 32;
	npc.pos += 1;
	*pc = npc;
	*clsp = nn;
	skip_space(pgp, pc);
	return 1;
}


static int parse_slash_char(struct peg_grammar_parser *pgp,
			    struct peg_cursor *pc)
{
	int c = CHAR(pgp, pc);

	if ( !cset_contains(cs_digit0to7, c) ) {
		if ( c == '\0' )
			return -1;
		pc->pos += 1;
	} else {
		if ( cset_contains(cs_digit0to2, c) && 
		     cset_contains(cs_digit0to7, CHARI(pgp, pc, 1)) && 
		     cset_contains(cs_digit0to7, CHARI(pgp, pc, 2)) ) {
			pc->pos += 3;
		} else if ( cset_contains(cs_digit0to7, CHARI(pgp, pc, 1)) ) {
			pc->pos += 2;
		} else {
			pc->pos += 1;
		}
	}

	return 0;
}


static int parse_code(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		      struct raw *r)
{
	struct peg_cursor npc = *pc;
	uint n;
	uint brace_depth;
	size_t len;

	if ( CHAR(pgp, &npc) != '{' )
		return 0;
	npc.pos += 1;
	brace_depth = 1;

	do {
		n = str_spn(STR(pgp, &npc), cs_ccode);
		npc.pos += n;

		if ( CHAR(pgp, &npc) == '"' ) {
			/* parse double quote */
			npc.pos += 1;
			while ( CHAR(pgp, &npc) != '"' ) {
				if ( CHAR(pgp, &npc) == '\r' ) {
					npc.pos += 1;
					npc.line += 1;
					if ( CHAR(pgp, &npc) == '\n' )
						npc.pos += 1;
				} else if ( CHAR(pgp, &npc) == '\n' ) {
					npc.pos += 1;
					npc.line += 1;
				} else if ( CHAR(pgp, &npc) == '\\' ) {
					npc.pos += 1;
					if ( parse_slash_char(pgp, &npc) < 0 )
						goto err;
				} else if ( CHAR(pgp, &npc) == '\0' ) {
					goto err;
				} else {
					npc.pos += 1;
				}
			}
			npc.pos += 1;
		} else if ( CHAR(pgp, &npc) == '\'' ) {
			/* parse single quote char */
			npc.pos += 1;
			if ( CHAR(pgp, &npc) == '\\' ) {
				npc.pos += 1;
				if ( parse_slash_char(pgp, &npc) < 0 )
					goto err;
			} else {
				npc.pos += 1;
			}
			if ( CHAR(pgp, &npc) != '\'' )
				goto err;
			npc.pos += 1;
		} else if ( CHAR(pgp, &npc) == '/' ) {
			if ( CHARI(pgp, &npc, 1) != '*' ) {
				npc.pos += 1;
				continue;
			}
			/* parse comment */
			npc.pos += 2;
			while ( CHAR(pgp, &npc) != '*' || 
			        CHARI(pgp, &npc, 1) != '/' ) {
				if ( CHAR(pgp, &npc) == '\r' ) {
					npc.pos += 1;
					npc.line += 1;
					if ( CHAR(pgp, &npc) == '\n' )
						npc.pos += 1;
				} else if ( CHAR(pgp, &npc) == '\n' ) {
					npc.pos += 1;
					npc.line += 1;
				} else if ( CHAR(pgp, &npc) == '\0' ) {
					goto err;
				} else {
					npc.pos += 1;
				}
			}
			npc.pos += 2;
		} else if ( CHAR(pgp, &npc) == '\r' ) {
			npc.pos += 1;
			npc.line += 1;
			if ( CHAR(pgp, &npc) == '\n' )
				npc.pos += 1;
		} else if ( CHAR(pgp, &npc) == '\n' ) {
			npc.pos += 1;
			npc.line += 1;
		} else if ( CHAR(pgp, &npc) == '\0' ) {
			goto err;
		} else if ( CHAR(pgp, &npc) == '{' ) {
			brace_depth += 1;
			npc.pos += 1;
		} else if ( CHAR(pgp, &npc) == '}' ) {
			brace_depth -= 1;
			npc.pos += 1;
		} else {
			abort_unless(0); /* should never happen! */
		}
	} while ( brace_depth > 0 );

	len = npc.pos - pc->pos;
	if ( copy_str(pgp, pc, len, r) < 0 )
		return -1;
	*pc = npc;
	skip_space(pgp, pc);
	return 1;

err:
	pgp->err = PEG_ERR_BAD_CODE;
	pgp->eloc = npc;
	return -1;
}


static int parse_action_label(struct peg_grammar_parser *pgp,
			      struct peg_cursor *pc, struct raw *r)
{
	struct peg_cursor npc = *pc;
	size_t len;

	if ( !string_match(pgp, ":", &npc) )
		return 0;

	if ( !cset_contains(cs_id_start, CHAR(pgp, &npc)) )
		return 0;

	len = 1 + str_spn(STR(pgp, &npc) + 1, cs_id_cont);
	if ( copy_str(pgp, &npc, len, r) < 0 )
		return -1;

	npc.pos += len;
	*pc = npc;
	skip_space(pgp, pc);
	return 1;
}


static int parse_primary(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
			 int *prip)
{
	struct peg_grammar *peg = pgp->peg;
	int pri;
	int rv;
	int match = -1;
	struct peg_cursor npc = *pc;
	int prefix = PEG_ATTR_NONE;
	int suffix = PEG_ATTR_NONE;
	int action = PEG_ACT_NONE;
	struct raw r = { 0, NULL };
	struct peg_node *pn;

	if ( string_match(pgp, "&", &npc) )
		prefix = PEG_ATTR_AND;
	else if ( string_match(pgp, "!", &npc) )
		prefix = PEG_ATTR_NOT;

	if ( (rv = parse_id_and_not_arrow(pgp, &npc, &match)) != 0 ) {
		if ( rv < 0 )
			goto err;
	} else if ( (rv = parse_paren_expr(pgp, &npc, &match)) != 0 ) {
		if ( rv < 0 )
			goto err;
	} else if ( (rv = parse_literal(pgp, &npc, &match)) != 0 ) {
		if ( rv < 0 )
			goto err;
	} else if ( (rv = parse_class(pgp, &npc, &match)) != 0 ) {
		if ( rv < 0 )
			goto err;
	} else {
		if ( prefix == PEG_ATTR_NONE )
			return 0;
		pgp->err = PEG_ERR_BAD_PRIMARY;
		pgp->eloc = *pc;
		return -1;
	}

	pri = peg_node_new(peg, PEG_PRIMARY, pc->line);
	if ( pri < 0 ) {
		pgp->err = PEG_ERR_NOMEM;
		goto err;
	}

	if ( string_match(pgp, "?", &npc) )
		suffix = PEG_ATTR_QUESTION;
	else if ( string_match(pgp, "*", &npc) )
		suffix = PEG_ATTR_STAR;
	else if ( string_match(pgp, "+", &npc) )
		suffix = PEG_ATTR_PLUS;
	else
		suffix = PEG_ATTR_NONE;

	rv = parse_code(pgp, &npc, &r);
	if ( rv < 0 )
		goto err;
	if ( rv > 0 ) {
		action = PEG_ACT_CODE;
	} else {
		rv = parse_action_label(pgp, &npc, &r);
		if ( rv < 0 )
			goto err;
		if ( rv > 0 )
			action = PEG_ACT_LABEL;
	}

	pn = NODE(peg, pri);
	pn->pn_next = -1;
	pn->pp_match = match;
	pn->pp_prefix = prefix;
	pn->pp_suffix = suffix;
	pn->pp_action = action;
	pn->pn_action_cb = NULL;
	pn->pp_code = r;

	*pc = npc;
	*prip = pri;
	return 1;

err:
	peg_node_free(peg, match);
	return -1;
}


static int parse_seq(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		     int *seqp)
{
	struct peg_grammar *peg = pgp->peg;
	int seq;
	int pri;
	int nn;
	struct peg_cursor npc = *pc;
	int rv;

	seq = peg_node_new(peg, PEG_SEQUENCE, pc->line);
	if ( seq < 0 ) {
		pgp->err = PEG_ERR_NOMEM;
		return -1;
	}

	rv = parse_primary(pgp, &npc, &pri);
	if ( rv < 0 )
		goto err;
	NODE(peg, seq)->ps_pri = pri;

	while ( rv != 0 ) {
		if ( (rv = parse_primary(pgp, &npc, &nn)) < 0 )
			goto err;
		if ( rv != 0 ) {
			NODE(peg, pri)->pn_next = nn;
			pri = nn;
		}
	}

	*pc = npc;
	*seqp = seq;
	return 1;

err:
	peg_node_free(peg, seq);
	return -1;
}


static int parse_expr(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		      int *exprp)
{
	struct peg_grammar *peg = pgp->peg;
	struct peg_cursor npc = *pc;
	int expr = -1;
	int snn;
	int nn;
	int rv;

	rv = parse_seq(pgp, &npc, &expr);
	if ( rv <= 0 )
		return rv;

	snn = expr;
	while ( string_match(pgp, "/", &npc) ) {
		rv = parse_seq(pgp, &npc, &nn);
		if ( rv < 0 )
			goto err;
		if ( rv == 0 ) {
			pgp->err = PEG_ERR_BAD_EXPR;
			goto err;
		}
		NODE(peg, snn)->pn_next = nn;
		snn = nn;
	}

	*pc = npc;
	*exprp = expr;
	return 1;

err:
	pgp->eloc = npc;
	peg_node_free(peg, expr);
	return -1;
}


static int parse_def(struct peg_grammar_parser *pgp, struct peg_cursor *pc,
		     int *defp)
{
	struct peg_grammar *peg = pgp->peg;
	struct peg_cursor npc = *pc;
	int id = -1;
	int expr = -1;
	int def;
	struct peg_node *pn;
	int rv;

	rv = parse_id(pgp, &npc, &id);
	if ( rv <= 0 )
		return rv;

	if ( NODE(peg, id)->pi_def >= 0 ) {
		pgp->err = PEG_ERR_DUP_DEF;
		pgp->eloc = *pc;
		return -1;
	}

	if ( !string_match(pgp, "<-", &npc) ) {
		peg_node_free(peg, id);
		return 0;
	}

	if ( (rv = parse_expr(pgp, &npc, &expr)) <= 0 ) {
		if ( rv == 0 )
			pgp->err = PEG_ERR_BAD_DEF;
		pgp->eloc = npc;
		peg_node_free(peg, id);
		return -1;
	}

	def = peg_node_new(peg, PEG_DEFINITION, pc->line);
	if ( def < 0 ) {
		peg_node_free(peg, id);
		peg_node_free(peg, expr);
		pgp->err = PEG_ERR_NOMEM;
		return -1;
	}

	pn = NODE(peg, def);
	pn->pd_id = id;
	pn->pd_expr = expr;
	pn = NODE(peg, id);
	pn->pi_def = def;

	*pc = npc;
	if ( defp != NULL )
		*defp = def;
	return 1;
}


static int check_unresolved_ids(struct peg_grammar_parser *pgp)
{
	struct peg_grammar *peg = pgp->peg;
	int i;

	for ( i = 0; i < peg->max_nodes; ++i ) {
		if ( pn_is_type(peg, i, PEG_IDENTIFIER) &&
		     NODE(peg, i)->pi_def < 0 ) {
			pgp->err = PEG_ERR_UNDEF_ID;
			pgp->eloc.pos = i;
			snprintf(pgp->unknown_id, sizeof(pgp->unknown_id), "%s",
				 NODE(peg, pgp->eloc.pos)->pi_name.data);
			return -1;
		}
	}
	return 0;
}


/* TODO: boundary check str_spn() calls to check against overflows. */
int peg_parse(struct peg_grammar_parser *pgp, struct peg_grammar *peg,
	      const char *string, uint len)
{
	struct peg_cursor pc = { 0, 1 };
	int def;
	int rv;

	if ( !initialized ) {
		cset_init_accept(cs_space, " \t\r\n");
		cset_init_accept(cs_eol, "\r\n");
		cset_init_accept(cs_id_start, 
			         "abcdefghijklmnopqrstuvwxyz"
			         "ABCDEFGHIJKLMNOPQRSTUVWXYZ_");
		cset_init_accept(cs_id_cont, 
			         "abcdefghijklmnopqrstuvwxyz"
			         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			         "_0123456789");
		cset_init_accept(cs_special, "nrt'\"[]\\");
		cset_init_accept(cs_digit0to2, "012");
		cset_init_accept(cs_digit0to7, "01234567");
		cset_init_reject(cs_ccode, "\"\'/{}\r\n");
		cset_rem(cs_ccode, '\0');
		initialized = 1;
	}

	peg->nodes = NULL;
	peg->max_nodes = 0;
	peg->num_nodes = 0;
	peg->start_node = -1;
	peg->dynamic = 1;

	pgp->input = string;
	pgp->input_len = len;
	pgp->len = 0;
	pgp->nlines = 1;
	pgp->peg = peg;
	pgp->err = 0;
	pgp->eloc = pc;

	skip_space(pgp, &pc);

	rv = parse_def(pgp, &pc, &def);
	if ( rv <= 0 ) {
		peg_free_nodes(peg);
		return rv;
	}

	peg->start_node = NODE(peg, def)->pd_id;

	while ( pc.pos < len ) {
		rv = parse_def(pgp, &pc, NULL);
		if ( rv < 0 ) {
			peg_free_nodes(peg);
			return -1;
		}
		if ( rv == 0 )
			break;
	}

	if ( check_unresolved_ids(pgp) < 0 ) {
		peg_free_nodes(peg);
		return -1;
	}

	pgp->len = pc.pos;
	pgp->nlines = pc.line;

	return 1;
}


void peg_free_nodes(struct peg_grammar *peg)
{
	int i;
	for ( i = 0; i < peg->max_nodes; ++i )
		peg_node_free(peg, i);
	if ( peg->dynamic ) {
		free(peg->nodes);
		peg->nodes = NULL;
		peg->max_nodes = 0;
		peg->num_nodes = 0;
		peg->start_node = 0;
		peg->dynamic = 0;
	}
}


char *peg_err_string(struct peg_grammar_parser *pgp, char *buf, size_t blen)
{
	static const char *peg_error_strings[] = {
		"No error",
		"Ran out of memory during parsing",
		"Erroneous non-terminal definition",
		"Duplicate definition of a non-terminal",
		"Erroneous expression",
		"Erroneous primary match",
		"Erroneous parenthesized expression",
		"Erroneous literal",
		"Undefined identifier",
		"Erroneous class",
		"Erroneous character",
		"Erroneous character range",
		"Erroneous code block",
	};
	if ( pgp->err < PEG_ERR_NONE || pgp->err > PEG_ERR_LAST ) {
		str_copy(buf, "Unknown PEG error", blen);
	} else if ( pgp->err == PEG_ERR_UNDEF_ID ) {
		snprintf(buf, blen, "Undefined identifier %s",
			 pgp->unknown_id);
	} else {
		snprintf(buf, blen, "%s on line %u / position %u",
			 peg_error_strings[pgp->err], pgp->eloc.line,
			 pgp->eloc.pos);
	}
	return buf;
}


static char *indent(char *buf, uint bsize, uint depth)
{
	uint i;
	buf[0] = '\0';
	bsize /= 2;
	for ( i = 0; i < depth; ++i ) {
		if ( i >= bsize - 1 )
			break;
		buf[2*i] = ' ';
		buf[2*i + 1] = ' ';
	}
	buf[2*i] = '\0';
	return buf;
}


void printchar(FILE *out, int i)
{
	if ( isprint(i) && !isspace(i) )
		fprintf(out, "%c", i);
	else if ( i == '\n' )
		fprintf(out, "\\n");
	else if ( i == '\r' )
		fprintf(out, "\\r");
	else if ( i == '\t' )
		fprintf(out, "\\t");
	else if ( i == ' ' )
		fprintf(out, " ");
	else
		fprintf(out, "\\%03o", i);
}


static void print_class(FILE *out, struct peg_node *cls)
{
	int i;
	int l;

	fprintf(out, "[");
	for ( l = -1, i = 0; i < 256; ++i ) {
		if ( cset_contains(cls->pc_cset, i) ) {
			if ( l == -1 ) {
				printchar(out, i);
				l = i;
			}
		} else {
			if ( l >= 0 && i > l + 1 ) {
				if ( i > l + 2 )
					fprintf(out, "-");
				printchar(out, i - 1);
			}
			l = -1;
		}
	}
	fprintf(out, "] ");
}


static void print_node(struct peg_grammar *peg, int i, int seq,
		       int depth, FILE *out)
{
	int j;
	int aggregate;
	char sbuf[256];
	struct peg_node *pn;

	if ( i < 0 || i >= peg->max_nodes )
		return;

	pn = NODE(peg, i);
	switch ( pn->pn_type ) {
	case PEG_DEFINITION:
		fprintf(out, "%s <-\n", NODE(peg, pn->pd_id)->pi_name.data); 
		fprintf(out, "%s", indent(sbuf, sizeof(sbuf), 1));
		print_node(peg, pn->pd_expr, 0, 1, out);
		fprintf(out, "\n");
		break;

	case PEG_SEQUENCE:
		if ( seq > 0 ) {
			fprintf(out, "\n");
			fprintf(out, "%s", indent(sbuf, sizeof(sbuf), depth));
			fprintf(out, "/ ");
		}
		print_node(peg, pn->ps_pri, seq, depth + 1, out);
		print_node(peg, pn->pn_next, seq + 1, depth, out);
		break;

	case PEG_PRIMARY:
		aggregate = 0;
		if ( seq == 0 &&
		     (NODE(peg, pn->pp_match)->pn_type == PEG_SEQUENCE) )
			aggregate = 1;
		fprintf(out, "%s", (pn->pp_prefix == PEG_ATTR_NONE) ? "" : 
		                   (pn->pp_prefix == PEG_ATTR_AND) ? "&" : 
		                   (pn->pp_prefix == PEG_ATTR_NOT) ? "!" :
				   "BAD_PREFIX!"); 
		if ( aggregate )
			fprintf(out, "( ");
		print_node(peg, pn->pp_match, seq, depth, out);
		if ( aggregate )
			fprintf(out, " )");
		fprintf(out, "%s", (pn->pp_suffix == PEG_ATTR_NONE) ? " " : 
		                   (pn->pp_suffix== PEG_ATTR_QUESTION) ? "? " : 
		                   (pn->pp_suffix == PEG_ATTR_STAR) ? "* " :
		                   (pn->pp_suffix == PEG_ATTR_PLUS) ? "+ " :
				   "BAD_SUFFIX!");
		if ( pn->pp_action != PEG_ACT_NONE ) {
			if ( pn->pp_action == PEG_ACT_CODE ) {
				fprintf(out, "\n");
				fprintf(out, "%s", 
					indent(sbuf, sizeof(sbuf), depth));
				fprintf(out, "CODE BLOCK:\n");
				fprintf(out, "%s", indent(sbuf, sizeof(sbuf),
					depth));
				fprintf(out, "%s", pn->pp_code.data);
			} else {
				fprintf(out, "(action_id: %s) ",
					pn->pp_label.data);
			}
		}
		print_node(peg, pn->pn_next, seq, depth, out);
		break;
		
	case PEG_IDENTIFIER:
		fprintf(out, "%s ", pn->pi_name.data);
		break;

	case PEG_LITERAL:
		fprintf(out, "'");
		for ( j = 0; j < pn->pl_value.len; ++j )
			printchar(out, pn->pl_value.data[j]);
		fprintf(out, "' ");
		break;

	case PEG_CLASS:
		print_class(out, pn);
		break;
	}
}


void peg_print(struct peg_grammar *peg, FILE *out)
{
	int i;
	for ( i = 0; i < peg->max_nodes; ++i )
		if ( pn_is_type(peg, i, PEG_DEFINITION) )
			print_node(peg, i, 0, 0, out);
	fprintf(out, "\n");
}


int peg_action_set(struct peg_grammar *peg, char *label, peg_action_f cb)
{
	int n = 0;
	int i;
	struct peg_node *pn;

	for ( i = 0; i < peg->max_nodes; ++i ) {
		if ( !pn_is_type(peg, i, PEG_PRIMARY) )
			continue;
		pn = NODE(peg, i);
		if ( pn->pp_action == PEG_ACT_LABEL &&
		     !strcmp(label, pn->pp_label.data) ) {
			pn->pp_action = PEG_ACT_CALLBACK;
			pn->pn_action_cb = cb;
			++n;
		}
	}
	return n;
}
