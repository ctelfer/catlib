#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cat/cat.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <cat/optparse.h>
#include <cat/peg.h>
#include <cat/cpg.h>
#include <cat/str.h>

#define NODE(_peg, _i) (&(_peg)->nodes[_i])
#define NODE_TYPE(_peg, _i) ((_peg)->nodes[_i].pn_type)
#define NODE_ID(_peg, _pn) ((_pn) - ((_peg)->nodes))
#define HASCALLBACK(_pn) \
	((_pn)->pn_type == PEG_PRIMARY &&	\
	 ((_pn)->pp_action == PEG_ACT_CODE ||	\
	  (_pn)->pp_action == PEG_ACT_LABEL))


struct clopt options[] = {
	CLOPT_I_NOARG('h', NULL, "print help"),
	CLOPT_I_NOARG('H', NULL, "generate header file"),
	CLOPT_I_STRING('o', NULL, "outfn",
		       "Output file name base (defaults \"cpeg\")"),
	CLOPT_I_STRING('d', NULL, "outdir",
		       "Output directory (defaults to input base path)"),
	CLOPT_I_STRING('p', NULL, "prefix",
		       "Output prefix filename (defaults to 'cpeg')"),
	CLOPT_I_NOARG('t', NULL, 
		      "Automatically generate tokens for uresolved IDs"),
	CLOPT_I_STRING('T', NULL, "tokpfx",
		       "Token prefix (defaults to 'CPEGTOK_')"),
};
struct clopt_parser oparse =
CLOPTPARSER_INIT(options, array_length(options));

const char *progname;
const char *in_fname;
int has_dname_param = 0;
int create_header = 0;
char out_dname[256] = "";
char out_froot[256] = "cpeg";
char out_fname_c[256];
char out_fname_h[256];
char out_fbase_h[256];
const char *prefix = "cpeg";
const char *tokpfx = "CPEGTOK_";
FILE *infile;
FILE *outfile_c;
FILE *outfile_h;
uint hlines;
int parse_flags = 0;


void usage(const char *estr)
{
	char str[4096];
	if ( estr != NULL )
		fprintf(stderr, "%s\n", estr);
	optparse_print(&oparse, str, sizeof(str));
	fprintf(stderr, "usage: %s [options] INFILE\n", progname);
	fprintf(stderr, "Options:\n%s\n", str);
	fprintf(stderr, "\nFile format is:\n"
	                "\t<HEADER>\n"
	                "\t%%%%\n"
	                "\t<GRAMMAR>\n"
	                "\t[%%%%\n"
	                "\t<TRAILER>]\n"
			"\tThe header may not contain the string '%%%%\\n'\n"
			"\tThe trailer (including delimiter) is optional\n");
	exit(1);
}


void build_filenames(void)
{
	size_t size;
	char *s;

	if ( !has_dname_param ) {
		s = strrchr(in_fname, '/');
		if ( s != NULL ) {
			size = s - in_fname + 1;
			if ( size >= sizeof(out_dname) - 1 )
				err("Output path too long\n");
			memcpy(out_dname, in_fname, size);
			out_dname[size] = '/';
			out_dname[size + 1] = '\0';
		}
	}

	size = sizeof(out_fname_c);
	if ( str_copy(out_fname_c, out_dname, size) >= size ||
	     str_cat(out_fname_c, out_froot, size) >= size ||
	     str_cat(out_fname_c, ".c", size) >= size )
		err("Output filename too long\n");

	size = sizeof(out_fbase_h);
	if ( str_copy(out_fbase_h, out_froot, size) >= size ||
	     str_cat(out_fbase_h, ".h", size) >= size )
		err("Output filename too long\n");

	size = sizeof(out_fname_h);
	if ( str_copy(out_fname_h, out_dname, size) >= size ||
	     str_cat(out_fname_h, out_fbase_h, size) >= size )
		err("Output filename too long\n");
}


void parse_args(int argc, char *argv[])
{
	int rv;
	struct clopt *opt;
	size_t n;

	infile = stdin;
	progname = argv[0];

	optparse_reset(&oparse, argc, argv);
	while ( !(rv = optparse_next(&oparse, &opt)) ) {
		switch (opt->ch) {
		case 'd':
			has_dname_param = 1;
			n = str_copy(out_dname, opt->val.str_val,
				     sizeof(out_dname));
			if ( n >= sizeof(out_dname) - 1 )
				err("output directory name too long\n");
			out_dname[n] = '/';
			out_dname[n + 1] = '\0';
			break;
		case 'h':
			usage(NULL);
			break;
		case 'H':
			create_header = 1;
			break;
		case 'o':
			n = str_copy(out_froot, opt->val.str_val,
				     sizeof(out_froot));
			if ( n >= sizeof(out_froot) )
				err("output file name too long\n");
			break;
		case 'p':
			prefix = opt->val.str_val;
			if ( strlen(prefix) == 0 )
				err("Empty parser prefix!");
			break;
		case 't':
			parse_flags |= PEG_GEN_TOKENS;
			break;
		case 'T':
			tokpfx = opt->val.str_val;
			break;
		}
	}
	if ( rv != argc - 1 )
		usage(oparse.errbuf);

	in_fname = argv[rv];

	build_filenames();

	infile = fopen(in_fname, "r");
	if (infile == NULL)
		errsys("Error opening file %s: ", in_fname);

	outfile_c = fopen(out_fname_c, "w");
	if (outfile_c == NULL)
		errsys("Error opening file %s: ", out_fname_c);

	if ( create_header ) {
		outfile_h = fopen(out_fname_h, "w");
		if (outfile_h == NULL)
			errsys("Error opening file %s: ", out_fname_h);
	}
}


#define BUFINCR 4096


void read_file(FILE *fp, struct raw *r)
{
	size_t n;
	size_t ss = 1;

	r->data = NULL;
	r->len = 0;
	do {
		ss += BUFINCR;
		r->data = erealloc(r->data, ss);
		n = fread(r->data + r->len, 1, BUFINCR, fp);
		if ( ferror(fp) )
			errsys("error reading input file: ");
		r->len += n;
	} while ( n == BUFINCR );

	r->data[r->len] = '\0';
	r->len;
}


void parse_header(struct raw *fstr, struct raw *head, char **gstrp)
{
	char *s, *lp;

	s = strstr(fstr->data, "%%\n");
	if ( s == NULL )
		err("Grammar file missing header delimiter '%%%%\\n'\n");
	head->data = fstr->data;
	head->len = (byte_t *)s - fstr->data;
	s += 3;
	*gstrp = s;

	hlines = 0;
	for ( lp = strchr(fstr->data, '\n'); lp < s; lp = strchr(lp + 1, '\n') )
		++hlines;
}


void parse_grammar_and_tail(struct raw *fstr, char *start,
			    struct peg_grammar_parser *pgp,
			    struct peg_grammar *peg, struct raw *tail)
{
	int rv;
	uint hdr_num_chars = (byte_t *)start - fstr->data;
	char ebuf[256];

	rv = peg_parse(pgp, peg, start, fstr->len - hdr_num_chars, parse_flags);
	if ( rv < 0 ) {
		pgp->eloc.pos += hdr_num_chars;
		pgp->eloc.line += hlines;
		err("%s\n", peg_err_string(pgp, ebuf, sizeof(ebuf)));
	}

	tail->data = start + pgp->len;
	if ( tail->data[0] == '\0' ) {
		tail->len = 0;
		return;
	} else {
		if ( strncmp(tail->data, "%%\n", 3) != 0 )
			err("Grammar did not end in %%%% on its own line.\n"
			    "Successful parse only to line %u\n",
			    hlines + pgp->nlines);
		tail->data += 3;
		tail->len = fstr->len - (tail->data - fstr->data);
	}
}


void emit_action(struct peg_grammar *peg, int nn)
{
	struct peg_node *pn = NODE(peg, nn);
	if ( pn->pp_action == PEG_ACT_CODE ) {
		fprintf(outfile_c,
			"static int __%s_peg_action%d(int __%s_node, "
			"struct raw *%s_text, void *%s_ctx)\n"
			"{ int %s_error = 0;\n",
			prefix, nn, prefix, prefix, prefix, prefix);
		fprintf(outfile_c, "#line %u \"%s\"\n", pn->pn_line + hlines,
			in_fname);
		fwrite(pn->pp_code.data, 1, pn->pp_code.len, outfile_c);
		fprintf(outfile_c, "\nreturn %s_error; }\n", prefix);
	} else {
		/* forward declaration for callback */
		abort_unless(pn->pp_action == PEG_ACT_LABEL);
		fprintf(outfile_c,
			"static int %s(int __%s_node, "
			"struct raw *%s_text, void *%s_ctx);\n",
			pn->pp_label.data, prefix, prefix, prefix);
	}
}


#define MUST_ESCAPE(_c) \
	((_c) == '"' || (_c) == '\\')


void emit_initializer(struct peg_grammar *peg, int nn)
{
	int i;
	int c;
	char buf[256] = "";
	struct peg_node *pn = NODE(peg, nn);

	fprintf(outfile_c, "{ %d, {", pn->pn_type);

	if ( pn->pn_type == PEG_LITERAL || pn->pn_type == PEG_CLASS ) {
		fprintf(outfile_c, "%u, (byte_t *)\"", (uint)pn->pn_str.len);
		for ( i = 0; i < pn->pn_str.len; ++i ) {
			c = pn->pn_str.data[i];
			if ( isprint(c) && !isspace(c) && !MUST_ESCAPE(c) )
				fputc(c, outfile_c);
			else
				fprintf(outfile_c, "\\x%02x", c);
		}
		fprintf(outfile_c, "\"}, ");
	} else if ( HASCALLBACK(pn) ) {
		if ( pn->pp_action == PEG_ACT_CODE )
			snprintf(buf, sizeof(buf), "__%s_peg_action%d", prefix,
				 nn);
		else
			str_copy(buf, pn->pp_label.data, sizeof(buf));
		fprintf(outfile_c, "%u, (byte_t *)\"%s\"}, ", (uint)strlen(buf),
			buf);
	} else {
		fprintf(outfile_c, "0, NULL}, ");
	}

	fprintf(outfile_c, "%d, %d, %d, ", pn->pn_next, pn->pn_subnode,
		pn->pn_line + hlines);

	if ( HASCALLBACK(pn) )
		fprintf(outfile_c, "%d, %d, %d, &%s},\n", PEG_ACT_CALLBACK,
			pn->pn_flag1, pn->pn_flag2, buf);
	else
		fprintf(outfile_c, "%d, %d, %d, NULL},\n", pn->pn_status,
			pn->pn_flag1, pn->pn_flag2);
}


void emit_prolog(struct peg_grammar *peg)
{
	int i;

	/* generate action routines */
	for ( i = 0; i < peg->num_nodes; ++i )
		if ( HASCALLBACK(NODE(peg, i)) )
			emit_action(peg, i);

	/* generate static parsing array */
	fprintf(outfile_c, "struct peg_node __%s_peg_nodes[%d] = {\n",
		prefix, peg->num_nodes);
	for ( i = 0; i < peg->num_nodes; ++i )
		emit_initializer(peg, i);
	fprintf(outfile_c, "};\n"
			   "static struct peg_grammar __%s_peg_grammar = {\n"
			   "\t__%s_peg_nodes,\n"
			   "\t%d,\n"
			   "\t%d,\n"
			   "\t%d,\n"
			   "\t%d,\n"
			   "\t0,\n"
			   "};\n\n",
		prefix, prefix, peg->num_nodes, peg->num_nodes, peg->num_tokens,
		peg->start_node);
}


void emit_forward_defs(FILE *fp, struct peg_grammar *peg)
{
	fprintf(fp,
		"struct %s_parser {\n"
		"\tstruct cpg_state pstate;\n"
		"};\n\n",
		prefix);

	fprintf(fp,
"int %s_init(struct %s_parser *p, int (*getc)(void *));\n\n",
prefix, prefix);

	fprintf(fp,
"int %s_parse(struct %s_parser *p, void *in, void *aux);\n\n",
prefix, prefix);

	fprintf(fp, "void %s_reset(struct %s_parser *p);\n\n",
		prefix, prefix);

	fprintf(fp, "void %s_fini(struct %s_parser *p);\n\n",
		prefix, prefix);

	if ( peg->num_tokens > 0 )
		fprintf(fp, "const char *%s_tok_to_str(int id);\n\n", prefix);
}


int find_token(struct peg_grammar *peg, int idx)
{
	int i;
	for ( i = 0; i < peg->num_nodes; ++i )
		if ( peg->nodes[i].pn_type == PEG_IDENTIFIER &&
		     peg->nodes[i].pi_def == PEG_TOKEN_IDX(idx) )
			return i;
	return -1;
}


void emit_parse_functions(struct peg_grammar *peg)
{
	int i;
	int id;

	if ( !create_header )
		emit_forward_defs(outfile_c, peg);

	fprintf(outfile_c,
		"static int __%s_getc(void *fp) { return fgetc(fp); }\n\n"
		"int %s_init(struct %s_parser *p, int (*getc)(void *)) {\n"
		"  if (getc == NULL) getc = &__%s_getc;\n"
		"  return cpg_init(&p->pstate, &__%s_peg_grammar, getc);\n"
		"}\n\n", prefix, prefix, prefix, prefix, prefix);

	fprintf(outfile_c,
		"int %s_parse(struct %s_parser *p, void *in, void *aux) {\n"
		"  return cpg_parse(&p->pstate, in, aux);\n"
		"}\n\n", prefix, prefix);

	fprintf(outfile_c,
		"void %s_reset(struct %s_parser *p) {\n"
		"  cpg_reset(&p->pstate);\n"
		"}\n\n", prefix, prefix);

	fprintf(outfile_c,
		"void %s_fini(struct %s_parser *p) {\n"
		"  cpg_fini(&p->pstate);\n"
		"}\n\n", prefix, prefix);

	if ( peg->num_tokens > 0 ) {
		fprintf(outfile_c, "\nstatic char *tokstrs[] = {\n");
		for ( i = 0; i < peg->num_tokens; ++i ) {
			id = find_token(peg, i);
			abort_unless(id >= 0);
			fprintf(outfile_c, "\"%s\",\n",
				NODE(peg, id)->pi_name.data);
		}
		fprintf(outfile_c, "NULL};\n\n");
		fprintf(outfile_c,
			"const char *%s_tok_to_str(int id) {\n"
			"  if ( id >= PEG_TOK_FIRST && id < PEG_TOK_FIRST + %d )\n"
			"    return tokstrs[id - PEG_TOK_FIRST];\n"
			"  else\n"
			"    return \"unknown token\";\n"
			"}\n\n", prefix, peg->num_tokens);
	}
}


void generate_parser(struct raw *head, struct peg_grammar *peg,
		     struct raw *tail)
{
	uint tl = 1;
	uchar *lp;
	fprintf(outfile_c, "#include <cat/peg.h>\n"
			   "#include <cat/cpg.h>\n"
			   "#include <stdio.h>\n"
			   "#include <string.h>\n");
	if ( create_header )
		fprintf(outfile_c, "#include \"%s\"\n", out_fbase_h);
	fprintf(outfile_c, "#line 0 \"%s\"\n", in_fname);
	fwrite(head->data, 1, head->len, outfile_c);
	emit_prolog(peg);
	emit_parse_functions(peg);
	if ( tail->len > 0 ) {
		for ( lp = strchr(head->data, '\n'); lp < tail->data;
		      lp = strchr(lp + 1, '\n') )
			tl++;
		fprintf(outfile_c, "#line %u \"%s\"\n", tl, in_fname);
		fwrite(tail->data, 1, tail->len, outfile_c);
	}
	fclose(outfile_c);
}


void emit_token_defs(FILE *fp, struct peg_grammar *peg)
{
	int i;
	struct peg_node *pn;

	for ( i = 0; i < peg->num_nodes; ++i ) {
		pn = NODE(peg, i);
		if ( pn->pn_type != PEG_IDENTIFIER ||
		     !PEG_IDX_IS_TOKEN(peg, pn->pi_def) )
			continue;
		fprintf(outfile_h, "#define %s%s %d\n", tokpfx,
			pn->pi_name.data, PEG_TOKEN_ID(pn->pi_def));
	}
}


void generate_header(struct peg_grammar *peg)
{
	fprintf(outfile_h, "#ifndef __%s_h\n", prefix);
	fprintf(outfile_h, "#define __%s_h\n\n", prefix);
	fprintf(outfile_h, "#include <cat/peg.h>\n"
			   "#include <cat/cpg.h>\n\n");
	emit_forward_defs(outfile_h, peg);
	emit_token_defs(outfile_h, peg);
	fprintf(outfile_h, "#endif /* __%s_h */\n", prefix);
	fclose(outfile_h);
}


int main(int argc, char *argv[])
{
	char *gstr;
	char *tstr;
	struct raw fstr;
	struct raw head;
	struct raw tail;
	struct peg_grammar_parser pgp;
	struct peg_grammar peg;

	parse_args(argc, argv);
	read_file(infile, &fstr);
	parse_header(&fstr, &head, &gstr);
	parse_grammar_and_tail(&fstr, gstr, &pgp, &peg, &tail);
	generate_parser(&head, &peg, &tail);
	if ( create_header )
		generate_header(&peg);
	peg_free_nodes(&peg);

	return 0;
}
