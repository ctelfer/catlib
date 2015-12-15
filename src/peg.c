#include <cat/cat.h>
#include <cat/aux.h>
#include <cat/str.h>
#include <cat/peg.h>
#include <stdlib.h>
#include <string.h>
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
 * Suffix <- Primary (QUESTION / STAR / PLUS)? Codeblock?
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

#define STR(_peg, _cursor) ((_peg)->input + (_cursor)->pos)
#define CHAR(_peg, _cursor) ((_peg)->input[(_cursor)->pos])
#define CHARI(_peg, _cursor, _off) ((_peg)->input[(_cursor)->pos + (_off)])


static int initialized = 0;
static byte_t cs_space[32];
static byte_t cs_eol[32];
static byte_t cs_id_start[32];
static byte_t cs_id_cont[32];
static byte_t cs_special[32];
static byte_t cs_digit0to2[32];
static byte_t cs_digit0to7[32];
static byte_t cs_ccode[32];


static void peg_node_free(union peg_node_u *n)
{
	if ( n == NULL )
		return;

	abort_unless(n->node.type >= PEG_DEFINITION &&
		     n->node.type <= PEG_CODE);

	switch ( n->node.type ) {
	case PEG_DEFINITION:
		peg_node_free((union peg_node_u *)n->def.id);
		peg_node_free((union peg_node_u *)n->def.expr);
		l_rem(&n->def.ln);
		break;
	case PEG_EXPRESSION:
		peg_node_free((union peg_node_u *)n->expr.seq);
		break;
	case PEG_SEQUENCE:
		peg_node_free((union peg_node_u *)n->seq.pri);
		peg_node_free((union peg_node_u *)n->seq.next);
		break;
	case PEG_PRIMARY:
		peg_node_free((union peg_node_u *)n->pri.match);
		peg_node_free((union peg_node_u *)n->pri.next);
		peg_node_free((union peg_node_u *)n->pri.action);
		break;
	case PEG_IDENTIFIER:
		--n->id.refcnt;
		if ( n->id.refcnt <= 0 ) {
			free(n->id.name.data);
			n->id.name.data = NULL;
			n->id.name.len = 0;
			rb_rem(&n->id.rbn);
			l_rem(&n->id.ln);
		} else {
			return;
		}
		break;
	case PEG_LITERAL:
		free(n->lit.value.data);
		n->lit.value.data = NULL;
		n->lit.value.len = 0;
		break;
	case PEG_CODE:
		free(n->act.action.code.data);
		n->act.action.code.data = NULL;
		n->act.action.code.len = 0;
		break;
	}

	free(n);
}


static void *peg_node_new(struct peg_grammar *peg, int type,
			  struct peg_cursor *pc, uint len, uint nlines)
{
	union peg_node_u *n;
	n = calloc(sizeof(*n), 1);
	if ( n == NULL )
		return NULL;
	n->node.type = type;
	n->node.loc = *pc;
	n->node.len = len;
	n->node.nlines = nlines;
	n->node.id = peg->next_id++;
	return n;
}


static void skip_space(struct peg_grammar *peg, struct peg_cursor *pc)
{
	int in_comment = 0;

	while ( in_comment || CHAR(peg, pc) == '#' || 
		cset_contains(cs_space, CHAR(peg, pc)) ) {
		if ( CHAR(peg, pc) == '#' ) {
			in_comment = 1;
		} else if ( cset_contains(cs_eol, CHAR(peg, pc)) ) {
			pc->line += 1;
			if ( CHAR(peg, pc) == '\r' && 
			     CHARI(peg, pc, 1) == '\n' )
				pc->pos += 1;
			in_comment = 0;
		}
		pc->pos += 1;
	}
}


static int string_match(struct peg_grammar *peg, const char *pat,
			struct peg_cursor *pc)
{
	uint plen;

	plen = strlen(pat);
	if ( strncmp(STR(peg, pc), pat, plen) == 0 ) {
		pc->pos += plen;
		skip_space(peg, pc);
		return 1;
	} else {
		return 0;
	}
}


static int parse_id(struct peg_grammar *peg, struct peg_cursor *pc,
		    struct peg_id **idp)
{
	struct peg_id *id;
	struct raw name;
	struct rbnode *rbn;
	int d;

	if ( !cset_contains(cs_id_start, CHAR(peg, pc)) )
		return 0;

	name.data = (char *)STR(peg, pc);
	name.len = 1 + str_spn(name.data + 1, cs_id_cont);
	rbn = rb_lkup(&peg->id_table, &name, &d);
	if ( d == CRB_N ) {
		id = container(rbn, struct peg_id, rbn);
		++id->refcnt;
	} else {
		id = peg_node_new(peg, PEG_IDENTIFIER, pc, name.len, 1);
		if ( id == NULL ) {
			peg->err = PEG_ERR_NOMEM;
			return -1;
		}
		id->name.data = malloc(name.len + 1);
		if ( name.data == NULL ) {
			peg_node_free((union peg_node_u *)id);
			peg->err = PEG_ERR_NOMEM;
			return -1;
		}
		memcpy(id->name.data, name.data, name.len);
		id->name.len = name.len;
		id->name.data[name.len] = '\0'; /* sanity */
		rb_ninit(&id->rbn, &id->name);
		rb_ins(&peg->id_table, &id->rbn, rbn, d);
		l_enq(&peg->id_list, &id->ln);
		id->def = NULL;
		id->refcnt = 1;
	}

	*idp = id;
	pc->pos += name.len;
	skip_space(peg, pc);
	return 1;
}


static int parse_id_and_not_arrow(struct peg_grammar *peg,
				  struct peg_cursor *pc, struct peg_id **idp)
{
	int rv;
	struct peg_cursor npc1 = *pc;
	struct peg_cursor npc2;
	struct peg_id *id;

	rv = parse_id(peg, &npc1, &id);
	if ( rv <= 0 )
		return rv;
	npc2 = npc1;
	if ( string_match(peg, "<-", &npc2) ) {
		peg_node_free((union peg_node_u *)id);
		return 0;
	}

	*pc = npc1;
	*idp = id;
	return 1;
}


static int parse_expr(struct peg_grammar *peg, struct peg_cursor *pc,
		      struct peg_expr **exprp);


static int parse_paren_expr(struct peg_grammar *peg, struct peg_cursor *pc,
			    struct peg_expr **exprp)
{
	int rv;
	struct peg_cursor npc = *pc;
	struct peg_expr *expr = NULL;

	if ( !string_match(peg, "(", &npc) )
		return 0;

	rv = parse_expr(peg, &npc, &expr);
	if ( rv < 0 )
		return -1;

	if ( rv == 0 )
		goto err;

	if ( !string_match(peg, ")", &npc) )
		goto err;

	*pc = npc;
	*exprp = expr;
	return 1;

err:
	if ( expr != NULL )
		peg_node_free((union peg_node_u *)expr);
	peg->eloc = npc;
	peg->err = PEG_ERR_BAD_PAREXPR;
	return -1;
}


static int parse_char(struct peg_grammar *peg, struct peg_cursor *pc,
		      int *cp)
{
	int c;
	struct peg_cursor npc = *pc;

	c = CHAR(peg, &npc);
	if ( c == '\0' )
		return 0;

	if ( c != '\\' ) {
		pc->pos += 1;
		if ( cset_contains(cs_eol, c) ) {
			pc->line += 1;
			if ( c == '\r' && CHAR(peg, pc) == '\n' )
				pc->pos += 1;
		}
		*cp = c;
		return 1;
	} 

	npc.pos += 1;
	c = CHAR(peg, &npc);
	if ( c == '\0' ) {
		peg->err = PEG_ERR_BAD_CHAR;
		peg->eloc = npc;
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
		peg->err = PEG_ERR_BAD_CHAR;
		peg->eloc = *pc;
		return -1;
	}

	if ( cset_contains(cs_digit0to2, c) &&
	     cset_contains(cs_digit0to7, CHARI(peg, &npc, 1)) && 
	     cset_contains(cs_digit0to7, CHARI(peg, &npc, 2)) ) {
		c = ((c - '0') << 6) |
		    ((CHARI(peg, &npc, 1) - '0') << 3) | 
		    (CHARI(peg, &npc, 2) - '0');
		npc.pos += 3;
	} else {
		c = c - '0';
		npc.pos += 1;
		if ( cset_contains(cs_digit0to7, CHAR(peg, &npc)) ) {
			c = (c << 3) | (CHAR(peg, &npc) - '0');
			npc.pos += 1;
		}
	}

	*pc = npc;
	*cp = c;
	return 1;
}


static int parse_literal(struct peg_grammar *peg, struct peg_cursor *pc,
			 struct peg_literal **litp)
{
	struct peg_cursor npc = *pc;
	struct peg_literal *lit;
	int c;
	int rv;
	uint i;
	char quote;
	struct raw value;

	quote = CHAR(peg, &npc);
	if ( quote != '"' && quote != '\'' )
		return 0;
	npc.pos += 1;

	value.len = 0;
	do { 
		rv = parse_char(peg, &npc, &c);
		if ( rv < 0 )
			goto err;
		if ( rv == 0 ) {
			peg->err = PEG_ERR_BAD_LITERAL;
			goto err;
		}
		value.len += 1;
	} while ( c != quote );

	lit = peg_node_new(peg, PEG_LITERAL, pc, npc.pos - pc->pos, 
			   npc.line - pc->line + 1);
	if ( lit == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		return -1;
	}
	value.data = malloc(value.len);
	if ( value.data == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		peg_node_free((union peg_node_u *)lit);
		return -1;
	}

	/* now copy/translate the string for real since we know */
	/* its true length and that it decodes correctly. */
	npc.pos = pc->pos + 1;
	npc.line = pc->line;
	for ( i = 0; i < value.len - 1; ++i ) {
		rv = parse_char(peg, &npc, &c);
		abort_unless(rv > 0); /* tested above */
		value.data[i] = c;
	}
	value.data[i] = '\0';
	lit->value = value;

	pc->pos = npc.pos + 1; /* skip last quote */
	pc->line = npc.line;
	*litp = lit;
	skip_space(peg, pc);
	return 1;

err:
	peg->eloc = npc;
	return -1;
}


static int class_add_char(struct peg_grammar *peg, struct peg_cursor *pc,
			  struct peg_class *cls)
{
	struct peg_cursor npc = *pc;
	int c1;
	int c2;
	int rv;

	rv = parse_char(peg, &npc, &c1);
	if ( rv < 0 )
		return -1;
	if ( rv == 0 ) {
		peg->err = PEG_ERR_BAD_CLASS;
		goto err;
	}

	if ( CHAR(peg, &npc) != '-' ) {
		cset_add(cls->cset, c1);
	} else {
		npc.pos += 1;
		rv = parse_char(peg, &npc, &c2);
		if ( rv < 0 )
			return -1;
		if ( rv == 0 ) {
			peg->err = PEG_ERR_BAD_RANGE;
			goto err;
		}
		while ( c1 <= c2 ) {
			cset_add(cls->cset, c1);
			++c1;
		}
	}

	*pc = npc;
	return 1;

err:
	peg->eloc = *pc;
	return -1;

}


static int parse_class(struct peg_grammar *peg, struct peg_cursor *pc,
		       struct peg_class **clsp)
{
	struct peg_cursor npc = *pc;
	char c;
	struct peg_class *cls;

	if ( CHAR(peg, &npc) != '[' && CHAR(peg, &npc) != '.' )
		return 0;

	cls = peg_node_new(peg, PEG_CLASS, pc, 1, 1);
	if ( cls == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		return -1;
	}

	/* treat . as a special class that matches almost everying */
	if ( CHAR(peg, &npc) == '.' ) {
		cset_fill(cls->cset);
		cset_rem(cls->cset, '\0');
		goto match;
	} else {
		cset_clear(cls->cset);
		npc.pos += 1;
	}

	while ( (c = CHAR(peg, &npc)) != ']' && c != '\0' )
		if ( class_add_char(peg, &npc, cls) < 0 )
			return -1;

	if ( c == '\0' )
		goto err;

match:
	npc.pos += 1;
	cls->node.len = npc.pos - pc->pos;
	cls->node.nlines = npc.line - pc->line + 1;
	*pc = npc;
	*clsp = cls;
	skip_space(peg, pc);
	return 1;

err:
	peg->eloc = npc;
	peg->err = PEG_ERR_BAD_CLASS;
	return -1;
}


static int parse_slash_char(struct peg_grammar *peg, struct peg_cursor *pc)
{
	int c = CHAR(peg, pc);

	if ( !cset_contains(cs_digit0to7, c) ) {
		if ( c == '\0' )
			return -1;
		pc->pos += 1;
	} else {
		if ( cset_contains(cs_digit0to2, c) && 
		     cset_contains(cs_digit0to7, CHARI(peg, pc, 1)) && 
		     cset_contains(cs_digit0to7, CHARI(peg, pc, 2)) ) {
			pc->pos += 3;
		} else if ( cset_contains(cs_digit0to7, CHARI(peg, pc, 1)) ) {
			pc->pos += 2;
		} else {
			pc->pos += 1;
		}
	}

	return 0;
}


static int parse_code(struct peg_grammar *peg, struct peg_cursor *pc,
		      struct peg_action **actionp)
{
	struct peg_cursor npc = *pc;
	struct peg_action *action;
	uint n;
	uint brace_depth;
	struct raw *r;

	if ( CHAR(peg, &npc) != '{' )
		return 0;
	npc.pos += 1;

	action = peg_node_new(peg, PEG_CODE, pc, 1, 1);
	if ( action == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		return -1;
	}
	brace_depth = 1;

	do {
		n = str_spn(STR(peg, &npc), cs_ccode);
		npc.pos += n;

		if ( CHAR(peg, &npc) == '"' ) {
			/* parse double quote */
			npc.pos += 1;
			while ( CHAR(peg, &npc) != '"' ) {
				if ( CHAR(peg, &npc) == '\r' ) {
					npc.pos += 1;
					npc.line += 1;
					if ( CHAR(peg, &npc) == '\n' )
						npc.pos += 1;
				} else if ( CHAR(peg, &npc) == '\n' ) {
					npc.pos += 1;
					npc.line += 1;
				} else if ( CHAR(peg, &npc) == '\\' ) {
					npc.pos += 1;
					if ( parse_slash_char(peg, &npc) < 0 )
						goto err;
				} else if ( CHAR(peg, &npc) == '\0' ) {
					goto err;
				} else {
					npc.pos += 1;
				}
			}
			npc.pos += 1;
		} else if ( CHAR(peg, &npc) == '\'' ) {
			/* parse single quote char */
			npc.pos += 1;
			if ( CHAR(peg, &npc) == '\\' ) {
				npc.pos += 1;
				if ( parse_slash_char(peg, &npc) < 0 )
					goto err;
			} else {
				npc.pos += 1;
			}
			if ( CHAR(peg, &npc) != '\'' )
				goto err;
			npc.pos += 1;
		} else if ( CHAR(peg, &npc) == '/' ) {
			if ( CHARI(peg, &npc, 1) != '*' ) {
				npc.pos += 1;
				continue;
			}
			/* parse comment */
			npc.pos += 2;
			while ( CHAR(peg, &npc) != '*' || 
			        CHARI(peg, &npc, 1) != '/' ) {
				if ( CHAR(peg, &npc) == '\r' ) {
					npc.pos += 1;
					npc.line += 1;
					if ( CHAR(peg, &npc) == '\n' )
						npc.pos += 1;
				} else if ( CHAR(peg, &npc) == '\n' ) {
					npc.pos += 1;
					npc.line += 1;
				} else if ( CHAR(peg, &npc) == '\0' ) {
					goto err;
				} else {
					npc.pos += 1;
				}
			}
			npc.pos += 2;
		} else if ( CHAR(peg, &npc) == '\r' ) {
			npc.pos += 1;
			npc.line += 1;
			if ( CHAR(peg, &npc) == '\n' )
				npc.pos += 1;
		} else if ( CHAR(peg, &npc) == '\n' ) {
			npc.pos += 1;
			npc.line += 1;
		} else if ( CHAR(peg, &npc) == '\0' ) {
			goto err;
		} else if ( CHAR(peg, &npc) == '{' ) {
			brace_depth += 1;
			npc.pos += 1;
		} else if ( CHAR(peg, &npc) == '}' ) {
			brace_depth -= 1;
			npc.pos += 1;
		} else {
			abort_unless(0); /* should never happen! */
		}
	} while ( brace_depth > 0 );

	npc.pos += 1;
	action->node.len = npc.pos - pc->pos;
	action->node.nlines = npc.line - pc->line + 1;
	r = &action->action.code;
	r->len = action->node.len;
	r->data = malloc(action->node.len + 1);
	if ( r->data == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		peg_node_free((union peg_node_u *)action);
		return -1;
	}
	memcpy(r->data, STR(peg, pc), r->len);
	r->data[r->len] = '\0';
	*pc = npc;
	*actionp = action;
	skip_space(peg, pc);
	return 1;

err:
	peg->err = PEG_ERR_BAD_CODE;
	peg->eloc = npc;
	peg_node_free((union peg_node_u *)action);
	return -1;
}


static int parse_primary(struct peg_grammar *peg, struct peg_cursor *pc,
			 struct peg_primary **prip)
{
	struct peg_primary *pri;
	struct peg_id *id;
	struct peg_expr *expr;
	struct peg_literal *lit;
	struct peg_class *cls;
	struct peg_cursor npc = *pc;
	struct peg_action *action;
	int rv;
	int prefix = PEG_ATTR_NONE;
	union peg_node_u *match = NULL;

	if ( string_match(peg, "&", &npc) )
		prefix = PEG_ATTR_AND;
	else if ( string_match(peg, "!", &npc) )
		prefix = PEG_ATTR_NOT;

	if ( (rv = parse_id_and_not_arrow(peg, &npc, &id)) != 0 ) {
		if ( rv < 0 )
			goto err;
		match = (union peg_node_u *)id;
	} else if ( (rv = parse_paren_expr(peg, &npc, &expr)) != 0 ) {
		if ( rv < 0 )
			goto err;
		match = (union peg_node_u *)expr;
	} else if ( (rv = parse_literal(peg, &npc, &lit)) != 0 ) {
		if ( rv < 0 )
			goto err;
		match = (union peg_node_u *)lit;
	} else if ( (rv = parse_class(peg, &npc, &cls)) != 0 ) {
		if ( rv < 0 )
			goto err;
		match = (union peg_node_u *)cls;
	} else {
		if ( prefix == PEG_ATTR_NONE ) {
			*prip = NULL;
			return 0;
		}
		peg->eloc = *pc;
		peg->err = PEG_ERR_BAD_PRIMARY;
		return -1;
	}

	pri = peg_node_new(peg, PEG_PRIMARY, pc, 0, 1);
	if ( pri == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		goto err;
	}

	pri->prefix = prefix;
	pri->match = match;
	pri->suffix = PEG_ATTR_NONE;
	pri->next = NULL;
	pri->action = NULL;

	if ( string_match(peg, "?", &npc) )
		pri->suffix = PEG_ATTR_QUESTION;
	else if ( string_match(peg, "*", &npc) )
		pri->suffix = PEG_ATTR_STAR;
	else if ( string_match(peg, "+", &npc) )
		pri->suffix = PEG_ATTR_PLUS;

	rv = parse_code(peg, &npc, &action);
	if ( rv < 0 )
		goto err;
	if ( rv > 0 )
		pri->action = action;

	pri->node.len = npc.pos - pc->pos;
	pri->node.nlines = npc.line - pc->line + 1;
	*pc = npc;
	*prip = pri;
	return 1;

err:
	if ( match != NULL )
		peg_node_free((union peg_node_u *)match);
	return -1;
}


static int parse_seq(struct peg_grammar *peg, struct peg_cursor *pc,
		     struct peg_seq **seqp)
{
	struct peg_seq *seq;
	struct peg_primary *pri;
	struct peg_primary *npri;
	struct peg_cursor npc = *pc;
	int rv;

	seq = peg_node_new(peg, PEG_SEQUENCE, pc, 0, 1);
	if ( seq == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		return -1;
	}

	seq->pri = NULL;
	rv = parse_primary(peg, &npc, &pri);
	if ( rv < 0 )	/* always -1 or 1 */
		goto err;
	seq->pri = pri;

	while ( rv != 0 ) {
		if ( (rv = parse_primary(peg, &npc, &npri)) < 0 )
			goto err;
		if ( rv != 0 ) {
			pri->next = npri;
			pri = npri;
		}
	}

	seq->node.len = npc.pos - pc->pos;
	seq->node.nlines = npc.line - pc->line + 1;
	*pc = npc;
	*seqp = seq;
	return 1;

err:
	while ( seq->pri != NULL ) {
		pri = seq->pri;
		seq->pri = pri->next;
		peg_node_free((union peg_node_u *)pri);
	}
	peg_node_free((union peg_node_u *)seq);

	return -1;
}


static int parse_expr(struct peg_grammar *peg, struct peg_cursor *pc,
		      struct peg_expr **exprp)
{
	struct peg_cursor npc = *pc;
	struct peg_expr *expr;
	struct peg_seq *seq;
	struct peg_seq *nseq;
	int rv;

	rv = parse_seq(peg, &npc, &seq);
	if ( rv <= 0 )
		return rv;

	rv = -1;
	expr = peg_node_new(peg, PEG_EXPRESSION, pc, 1, 1);
	if ( expr == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		peg_node_free((union peg_node_u *)seq);
		return -1;
	}
	expr->seq = seq;

	while ( string_match(peg, "/", &npc) ) {
		rv = parse_seq(peg, &npc, &nseq);
		if ( rv < 0 )
			goto err;
		if ( rv == 0 ) {
			peg->err = PEG_ERR_BAD_EXPR;
			goto err;
		}
		seq->next = nseq;
		seq = nseq;
	}

	expr->node.len = npc.pos - pc->pos;
	expr->node.nlines = npc.line - pc->line + 1;
	*pc = npc;
	*exprp = expr;
	return 1;

err:
	peg->eloc = npc;
	peg_node_free((union peg_node_u *)expr);
	return -1;
}


static int parse_def(struct peg_grammar *peg, struct peg_cursor *pc,
		     struct peg_def **defp)
{
	struct peg_cursor npc = *pc;
	struct peg_id *id = NULL;
	struct peg_expr *expr = NULL;
	struct peg_def *def;
	int rv;

	rv = parse_id(peg, &npc, &id);
	if ( rv <= 0 )
		return rv;

	if ( id->def != NULL ) {
		peg->err = PEG_ERR_DUP_DEF;
		peg->eloc = *pc;
		goto err;
	}

	if ( !string_match(peg, "<-", &npc) ) {
		peg_node_free((union peg_node_u *)id);
		return 0;
	}

	if ( (rv = parse_expr(peg, &npc, &expr)) <= 0 ) {
		if ( rv == 0 )
			peg->err = PEG_ERR_BAD_DEF;
		peg->eloc = npc;
		goto err;
	}

	def = peg_node_new(peg, PEG_DEFINITION, pc, npc.pos - pc->pos,
			   npc.line - pc->line + 1);
	if ( def == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		goto err;
	}

	def->id = id;
	def->expr = expr;
	id->def = def;
	l_enq(&peg->def_list, &def->ln);

	*pc = npc;
	if ( defp != NULL )
		*defp = def;
	return 1;

err:
	if ( id != NULL )
		peg_node_free((union peg_node_u *)id);
	if ( expr != NULL )
		peg_node_free((union peg_node_u *)expr);
	return -1;
}


static int check_unresolved_ids(struct peg_grammar *peg)
{
	struct list *ln;
	struct peg_id *id;

	l_for_each(ln, &peg->id_list) {
		id = container(ln, struct peg_id, ln);
		if ( id->def == NULL ) {
			peg->err = PEG_ERR_UNDEF_LITERAL;
			peg->eloc = id->node.loc;
			return -1;
		}
	}
	return 0;
}


int peg_parse(struct peg_grammar *peg, const char *string, uint len)
{
	struct peg_def *def;
	struct peg_cursor pc = { 0, 1 };
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

	peg->input = string;
	l_init(&peg->def_list);
	rb_init(&peg->id_table, &cmp_raw);
	l_init(&peg->id_list);
	peg->next_id = 1;

	skip_space(peg, &pc);

	rv = parse_def(peg, &pc, &def);
	if ( rv <= 0 ) {
		peg_free_nodes(peg);
		return rv;
	}

	peg->start = def;

	while ( pc.pos < len ) {
		rv = parse_def(peg, &pc, NULL);
		if ( rv < 0 ) {
			peg_free_nodes(peg);
			return -1;
		}
		if ( rv == 0 )
			break;
	}

	if ( check_unresolved_ids(peg) < 0 ) {
		peg_free_nodes(peg);
		return -1;
	}

	peg->len = pc.pos;
	peg->nlines = pc.line;

	return 1;
}


void peg_free_nodes(struct peg_grammar *peg)
{
	struct list *ln;
	struct list *x;
	struct peg_def *def;
	l_for_each_safe(ln, x, &peg->def_list) {
		def = container(ln, struct peg_def, ln);
		abort_unless(def->node.type == PEG_DEFINITION);
		l_rem(&def->ln);
		peg_node_free((union peg_node_u *)def);
	}
}


void peg_reset(struct peg_grammar *peg)
{
	abort_unless(peg != NULL);
	peg_free_nodes(peg);
	peg->input = NULL;
	peg->start = NULL;
	peg->err = PEG_ERR_NONE;
	peg->eloc.pos = 0;
	peg->eloc.line= 0;
}


const char *peg_err_message(int err)
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
		"Undefined literal",
		"Erroneous class",
		"Erroneous character",
		"Erroneous character range",
		"Erroneous code block",
	};
	if ( err < PEG_ERR_NONE || err > PEG_ERR_LAST )
		return "Unknown PEG error";
	return peg_error_strings[err];
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


static void print_class(FILE *out, struct peg_class *cls)
{
	int i;
	int l;

	fprintf(out, "[");
	for ( l = -1, i = 0; i < 256; ++i ) {
		if ( cset_contains(cls->cset, i) ) {
			if ( l == -1 ) {
				if ( isprint(i) && !isspace(i) )
					fprintf(out, "%c", i);
				else
					fprintf(out, "\\%0d", i);
				l = i;
			}
		} else {
			if ( l >= 0 && i > l + 1 ) {
				if ( i > l + 2 )
					fprintf(out, "-");
				if ( isprint(i - 1) && !isspace(i - 1) )
					fprintf(out, "%c", i - 1);
				else
					fprintf(out, "\\%0d", i - 1);
			}
			l = -1;
		}
	}
	fprintf(out, "] ");
}


static void print_node(struct peg_grammar *peg, union peg_node_u *n, int seq,
		       int depth, FILE *out)
{

	char sbuf[256];

	if ( n == NULL )
		return;

	switch ( n->node.type ) {
	case PEG_DEFINITION:
		fprintf(out, "%s <- ", n->def.id->name.data); 
		print_node(peg, (union peg_node_u *)n->def.expr, 0, 1, out);
		fprintf(out, "\n");
		break;

	case PEG_EXPRESSION:
		print_node(peg, (union peg_node_u *)n->expr.seq, 0, depth, out);
		break;

	case PEG_SEQUENCE:
		fprintf(out, "\n");
		fprintf(out, "%s", indent(sbuf, sizeof(sbuf), depth));
		if ( seq > 0 ) fprintf(out, "/ ");
		print_node(peg, (union peg_node_u *)n->seq.pri, seq, depth + 1,
			   out);
		print_node(peg, (union peg_node_u *)n->seq.next, seq + 1,
			   depth, out);
		break;

	case PEG_PRIMARY:
		fprintf(out, "%s", (n->pri.prefix == PEG_ATTR_NONE) ? "" : 
		                   (n->pri.prefix == PEG_ATTR_AND) ? "&( " : 
		                   (n->pri.prefix == PEG_ATTR_NOT) ? "!( " :
				   "BAD_PREFIX!"); 
		print_node(peg, n->pri.match, seq, depth, out);
		fprintf(out, "%s", (n->pri.suffix == PEG_ATTR_NONE) ? "" : 
		                   (n->pri.suffix== PEG_ATTR_QUESTION) ? "?" : 
		                   (n->pri.suffix == PEG_ATTR_STAR) ? "*" :
		                   (n->pri.suffix == PEG_ATTR_PLUS) ? "+" :
				   "BAD_SUFFIX!");
		fprintf(out, "%s", (n->pri.prefix == PEG_ATTR_NONE) ? " " : 
				   ") ");
		if ( n->pri.action != NULL )
			print_node(peg, (union peg_node_u *)n->pri.action, seq,
				   depth + 1, out);
		print_node(peg, (union peg_node_u *)n->pri.next, seq, depth,
			   out);
		break;
		
	case PEG_IDENTIFIER:
		fprintf(out, "%s ", n->id.name.data);
		break;

	case PEG_LITERAL:
		fprintf(out, "'%s' ", n->lit.value.data);
		break;

	case PEG_CLASS:
		print_class(out, &n->cls);
		break;

	case PEG_CODE:
		fprintf(out, "\n");
		fprintf(out, "%s", indent(sbuf, sizeof(sbuf), depth));
		fprintf(out, "CODE BLOCK:\n");
		fprintf(out, "%s", indent(sbuf, sizeof(sbuf), depth));
		fprintf(out, "%s", n->act.action.code.data);
		break;
	}

}


void peg_print(struct peg_grammar *peg, FILE *out)
{
	struct list *ln;
	struct peg_def *def;
	l_for_each(ln, &peg->def_list) {
		def = container(ln, struct peg_def, ln);
		print_node(peg, (union peg_node_u *)def, 0, 0, out);
	}
	fprintf(out, "\n");
}
