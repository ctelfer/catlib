#include <cat/cat.h>
#include <cat/aux.h>
#include <cat/str.h>
#include <cat/peg.h>
#include <stdlib.h>
#include <string.h>

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
		l_rem(&n->def.le);
		rb_rem(&n->def.rbe);
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
		peg_node_free((union peg_node_u *)n->pri.code);
		break;
	case PEG_IDENTIFIER:
		l_rem(&n->id.le);
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

	if ( !cset_contains(cs_id_start, CHAR(peg, pc)) )
		return 0;

	id = peg_node_new(peg, PEG_IDENTIFIER, pc, 1, 1);
	if ( id == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		return -1;
	}

	while ( cset_contains(cs_id_cont, CHARI(peg, pc, id->node.len)) )
		id->node.len += 1;

	id->name.data = (char *)STR(peg, pc);
	id->name.len = id->node.len;
	l_enq(&peg->id_list, &id->le);

	pc->pos += id->node.len;
	*idp = id;
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
	char quote;

	quote = CHAR(peg, &npc);
	if ( quote != '"' && quote != '\'' )
		return 0;
	npc.pos += 1;

	do { 
		rv = parse_char(peg, &npc, &c);
		if ( rv < 0 )
			goto err;
		if ( rv == 0 ) {
			peg->err = PEG_ERR_BAD_LITERAL;
			goto err;
		}
	} while ( c != quote );

	lit = peg_node_new(peg, PEG_LITERAL, pc, npc.pos - pc->pos, 
			   npc.line - pc->line + 1);
	if ( lit == NULL ) {
		peg->err = PEG_ERR_NOMEM;
		return -1;
	}

	*pc = npc;
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
		      struct peg_code **codep)
{
	struct peg_cursor npc = *pc;
	struct peg_code *code;
	uint n;
	uint brace_depth;

	if ( CHAR(peg, &npc) != '{' )
		return 0;
	npc.pos += 1;

	code = peg_node_new(peg, PEG_CODE, pc, 1, 1);
	if ( code == NULL ) {
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
	code->node.len = npc.pos - pc->pos;
	code->node.nlines = npc.line - pc->line + 1;
	*pc = npc;
	*codep = code;
	skip_space(peg, pc);
	return 1;

err:
	peg->err = PEG_ERR_BAD_CODE;
	peg->eloc = npc;
	peg_node_free((union peg_node_u *)code);
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
	struct peg_code *code;
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
	pri->code = NULL;

	if ( string_match(peg, "?", &npc) )
		pri->suffix = PEG_ATTR_QUESTION;
	else if ( string_match(peg, "*", &npc) )
		pri->suffix = PEG_ATTR_STAR;
	else if ( string_match(peg, "+", &npc) )
		pri->suffix = PEG_ATTR_PLUS;

	rv = parse_code(peg, &npc, &code);
	if ( rv < 0 )
		goto err;
	if ( rv > 0 )
		pri->code = code;

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
	struct rbnode *rbn;
	int d;
	int rv;

	rv = parse_id(peg, &npc, &id);
	if ( rv <= 0 )
		return rv;

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
	rb_ninit(&def->rbe, &id->name);

	rbn = rb_lkup(&peg->def_table, &id->name, &d);
	if ( d == CRB_N ) {
		peg->err = PEG_ERR_DUP_DEF;
		peg->eloc = def->node.loc;
		goto err;
	}
	rb_ins(&peg->def_table, &def->rbe, rbn, d);
	l_enq(&peg->def_list, &def->le);

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


static int resolve_ids(struct peg_grammar *peg)
{
	struct list *ln;
	struct peg_id *id;
	int d;
	struct rbnode *rbn;

	l_for_each(ln, &peg->id_list) {
		id = container(ln, struct peg_id, le);
		rbn = rb_lkup(&peg->def_table, &id->name, &d);
		if ( d != CRB_N ) {
			peg->err = PEG_ERR_UNDEF_LITERAL;
			peg->eloc = id->node.loc;
			return -1;
		}
		id->def = container(rbn, struct peg_def, rbe);
	}
	return 0;
}


int peg_parse(struct peg_grammar *peg, const char *string)
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
	peg->inlen = strlen(string);
	l_init(&peg->def_list);
	rb_init(&peg->def_table, &cmp_raw);
	l_init(&peg->id_list);
	peg->next_id = 1;

	skip_space(peg, &pc);

	rv = parse_def(peg, &pc, &def);
	if ( rv <= 0 ) {
		peg_free_nodes(peg);
		return rv;
	}

	peg->start = def;

	while ( pc.pos < peg->inlen ) {
		rv = parse_def(peg, &pc, NULL);
		if ( rv < 0 ) {
			peg_free_nodes(peg);
			return -1;
		}
		if ( rv == 0 )
			break;
	}

	if ( resolve_ids(peg) < 0 ) {
		peg_free_nodes(peg);
		return -1;
	}

	return 1;
}


void peg_free_nodes(struct peg_grammar *peg)
{
	struct list *ln;
	struct peg_def *def;
	l_for_each(ln, &peg->def_list) {
		def = container(ln, struct peg_def, le);
		abort_unless(def->node.type == PEG_DEFINITION);
		l_rem(&def->le);
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


static char *node2str(char *buf, uint bsize, struct peg_grammar *peg,
		      struct peg_node *node)
{
	uint len = node->len;
	if ( len > bsize - 1 )
		len = bsize - 1;
	memcpy(buf, peg->input + node->loc.pos, node->len);
	buf[len] = '\0';
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


static void print_node(struct peg_grammar *peg, union peg_node_u *n, int seq,
		       int depth, FILE *out)
{

	char sbuf[256];

	if ( n == NULL )
		return;

	switch ( n->node.type ) {
	case PEG_DEFINITION:
		fprintf(out, "%s <- ", node2str(sbuf, sizeof(sbuf),
					        peg, &n->def.id->node));
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
		if ( n->pri.code != NULL )
			print_node(peg, (union peg_node_u *)n->pri.code, seq,
				   depth + 1, out);
		print_node(peg, (union peg_node_u *)n->pri.next, seq, depth,
			   out);
		break;
		
	case PEG_IDENTIFIER:
	case PEG_LITERAL:
	case PEG_CLASS:
		fprintf(out, "%s ", node2str(sbuf, sizeof(sbuf), peg,
					     &n->node));
		break;

	case PEG_CODE:
		fprintf(out, "\n");
		fprintf(out, "%s", indent(sbuf, sizeof(sbuf), depth));
		fprintf(out, "CODE BLOCK:\n");
		fprintf(out, "%s", indent(sbuf, sizeof(sbuf), depth));
		fprintf(out, "%s", node2str(sbuf, sizeof(sbuf), peg,
					    &n->node));
		break;
	}

}


void peg_print(struct peg_grammar *peg, FILE *out)
{
	struct list *ln;
	struct peg_def *def;
	l_for_each(ln, &peg->def_list) {
		def = container(ln, struct peg_def, le);
		print_node(peg, (union peg_node_u *)def, 0, 0, out);
	}
	fprintf(out, "\n");
}
