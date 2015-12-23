#include <cat/cat.h>
#include <cat/emalloc.h>
#include <cat/err.h>
#include <cat/optparse.h>
#include <cat/peg.h>
#include <cat/cpg.h>
#include <cat/str.h>
#include <string.h>
#include <stdlib.h>

#define NODE(_peg, _i) (&(_peg)->nodes[_i])
#define NODE_TYPE(_peg, _i) ((_peg)->nodes[_i].pn_type)
#define NODE_ID(_peg, _pn) ((_pn) - ((_peg)->nodes))

struct clopt options[] = {
	CLOPT_I_NOARG('h', NULL, "print help"),
	CLOPT_I_NOARG('H', NULL, "generate header file"),
	CLOPT_I_STRING('d', NULL, "outdir",
		       "Output directory (defaults to input base path)"),
	CLOPT_I_STRING('p', NULL, "prefix",
		       "Output prefix filename (defaults to 'cpeg')"),
};
struct clopt_parser oparse =
CLOPTPARSER_INIT(options, array_length(options));

const char *progname;
const char *in_fname;
int has_dname_param = 0;
int create_header = 0;
char out_dname[256] = "";
char out_fname_c[256];
char out_fname_h[256];
const char *prefix = "cpeg";
FILE *infile;
FILE *outfile_c;
FILE *outfile_h;


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


void get_filenames(void)
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
	     str_cat(out_fname_c, prefix, size) >= size || 
	     str_cat(out_fname_c, ".c", size) >= size )
		err("Output filename too long");

	size = sizeof(out_fname_h);
	if ( str_copy(out_fname_h, out_dname, size) >= size ||
	     str_cat(out_fname_h, prefix, size) >= size || 
	     str_cat(out_fname_h, ".h", size) >= size )
		err("Output filename too long");
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
		case 'p':
			prefix = opt->val.str_val;
			if ( strlen(prefix) == 0 )
				err("Empty parser prefix!");
			break;
		}
	}
	if ( rv != argc - 1 )
		usage(oparse.errbuf);

	in_fname = argv[rv];

	get_filenames();

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


void parse_header(struct raw *fstr, struct raw *head, char **gstrp, uint *hl)
{
	char *s, *lp;

	s = strstr(fstr->data, "%%\n");
	if ( s == NULL )
		err("Grammar file missing header delimiter '%%%%\\n'\n");
	head->data = fstr->data;
	head->len = (byte_t *)s - fstr->data;
	s += 3;
	*gstrp = s;

	*hl = 1;
	for ( lp = strchr(fstr->data, '\n'); lp != *gstrp - 1;
	      lp = strchr(lp + 1, '\n') )
		*hl++;
}


void parse_grammar_and_tail(struct raw *fstr, char *start, uint hl,
			    struct peg_grammar_parser *pgp,
			    struct peg_grammar *peg, struct raw *tail)
{
	int rv;
	uint hdr_num_chars = (byte_t *)start - fstr->data;
	char ebuf[256];

	rv = peg_parse(pgp, peg, start, fstr->len - hdr_num_chars);
	if ( rv < 0 ) {
		pgp->eloc.pos += hdr_num_chars;
		pgp->eloc.line += hl;
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
			    hl + pgp->nlines);
		tail->data += 3;
		tail->len = fstr->len - (tail->data - fstr->data);
	}
}


void emit_action(struct peg_grammar *peg, int nn)
{
	struct peg_node *pn = NODE(peg, nn);
	fprintf(outfile_c,
		"static int __%s_peg_action%d(int __%s_node, "
		"struct raw *%s_text, void *%s_ctx)\n",
		prefix, nn, prefix, prefix, prefix);
	fwrite(pn->pp_code.data, 1, pn->pp_code.len, outfile_c);
	fprintf(outfile_c, "\n\n");
}


void emit_initializer(struct peg_grammar *peg, int nn)
{
	int i;
	int c;
	char buf[256] = "";
	struct peg_node *pn = NODE(peg, nn);

	fprintf(outfile_c, "{ %d, {", pn->pn_type);

	if ( pn->pn_type == PEG_LITERAL || pn->pn_type == PEG_CLASS ) {
		fprintf(outfile_c, "%u, \"", (uint)pn->pn_str.len);
		for ( i = 0; i < pn->pn_str.len; ++i ) {
			c = pn->pn_str.data[i];
			if ( isprint(c) && !isspace(c) )
				fputc(c, outfile_c);
			else
				fprintf(outfile_c, "\\x%02x", c);
		}
		fprintf(outfile_c, "\"}, ");
	} else if ( pn->pn_type == PEG_PRIMARY &&
		    pn->pp_action == PEG_ACT_CODE ) {
		snprintf(buf, sizeof(buf), "__%s_peg_action%d", prefix, nn);
		fprintf(outfile_c, "%u, \"%s\"}, ", (uint)strlen(buf), buf);
	} else {
		fprintf(outfile_c, "0, NULL}, ", pn->pn_type);
	}

	fprintf(outfile_c, "%d, %d, ", pn->pn_next, pn->pn_subnode);

	if ( pn->pn_type == PEG_PRIMARY && pn->pp_action == PEG_ACT_CODE ) {
		fprintf(outfile_c, "%d, %d, %d, &%s},\n", PEG_ACT_CALLBACK,
			pn->pn_flag1, pn->pn_flag2, buf); 
	} else {
		fprintf(outfile_c, "%d, %d, %d, NULL},\n", pn->pn_status,
			pn->pn_flag1, pn->pn_flag2);
	}
}


void emit_prolog(struct peg_grammar *peg)
{
	int i;

	/* generate action routines */
	for ( i = 0; i < peg->num_nodes; ++i ) {
		if ( NODE_TYPE(peg, i) != PEG_PRIMARY )
			continue;
		if ( NODE(peg, i)->pp_action != PEG_ACT_CODE )
			continue;
		emit_action(peg, i);
	}

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
			   "\t0,\n"
			   "};\n\n",
		prefix, prefix, peg->num_nodes, peg->num_nodes,
		peg->start_node);
}


void write_struct_def(FILE *fp)
{
	fprintf(fp,
"struct %s_parser {\n"
"\tstruct cpg_state pstate;\n"
"};\n\n",
prefix);
}


void emit_parse_functions(void)
{
	if ( !create_header )
		write_struct_def(outfile_c);

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
}


void generate_parser(struct raw *head, struct peg_grammar *peg,
		     struct raw *tail)
{
	fprintf(outfile_c, "#include <cat/peg.h>\n"
			   "#include <cat/cpg.h>\n"
			   "#include <stdio.h>\n"
			   "#include <string.h>\n");
	if ( create_header )
		fprintf(outfile_c, "#include \"pp.h\"\n");
	fwrite(head->data, 1, head->len, outfile_c);
	emit_prolog(peg);
	emit_parse_functions();
	if ( tail->len > 0 )
		fwrite(tail->data, 1, tail->len, outfile_c);
	fclose(outfile_c);
}


void generate_header(void)
{
	/* TODO */
	fclose(outfile_h);
}


int main(int argc, char *argv[])
{
	char *gstr;
	char *tstr;
	struct raw fstr;
	struct raw head;
	struct raw tail;
	uint hl;
	struct peg_grammar_parser pgp;
	struct peg_grammar peg;

	parse_args(argc, argv);
	read_file(infile, &fstr);
	parse_header(&fstr, &head, &gstr, &hl);
	parse_grammar_and_tail(&fstr, gstr, hl, &pgp, &peg, &tail);
	generate_parser(&head, &peg, &tail);
	if ( create_header )
		generate_header();
	return 0;
}
