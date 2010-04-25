#include <cat/cat.h>
#include <cat/csv.h>
#include <cat/grow.h>

#include <string.h>


void csv_init(struct csv_state *csv, getchar_f gc, void *gcctx)
{
	abort_unless(csv && gc);
	memset(csv, 0, sizeof(*csv));
	csv->csv_getc = gc;
	csv->csv_gcctx = gcctx;
	csv->csv_last = -1;
}


int csv_next(struct csv_state *csv, char *p, size_t len, size_t *rlen)
{
	size_t i = 0;
	getchar_f getc = csv->csv_getc;
	void *gcctx = csv->csv_gcctx;
	int c, code = CSV_OK, prquote;

	if ( !csv || !p || len-- < 2 )
		return CSV_ERR;

	if ( csv->csv_done > 0 )
		return CSV_EOF;
	else if ( csv->csv_done < 0 )
		return CSV_ERR;

	do {
		if ( csv->csv_last < 0 ) {
			c = (*getc)(gcctx);
		} else {
			c = csv->csv_last;
			csv->csv_last = -1;
		}

		if ( c < 0 ) {
			if ( c == CSV_GETC_EOF ) {
				csv->csv_done = 1;
				code = CSV_REC;
			} else {
				abort_unless(c == CSV_GETC_ERR);
				csv->csv_done = -1;
				code = CSV_ERR;
			}
			break;
		} else if ( c == '"' ) {
			csv->csv_inquote = !csv->csv_inquote;
			prquote = !csv->csv_inquote && csv->csv_sawquote;
			csv->csv_sawquote = 1;
			if ( !prquote )
				continue;
		} else {
			csv->csv_sawquote = 0;
		}

		if ( c == ',' && !csv->csv_inquote ) {
			code = CSV_FLD;
		} else if ( c == '\n' && !csv->csv_inquote ) {
			code = CSV_REC;
		} else {
			if ( i == len ) {
				csv->csv_last = c;
				if ( c == '"' )
					csv->csv_inquote = !csv->csv_inquote;
				code = CSV_CNT;
			} else {
				*p++ = c;
				++i;
			}
		}
	} while ( code == CSV_OK );

	if ( i == 0 && csv->csv_done > 0 )
		code = CSV_EOF;

	if ( code != CSV_EOF ) {
		*p = '\0';
		if ( code != CSV_CNT ) {
			csv->csv_inquote = 0;
			csv->csv_sawquote = 0;
		}
		if ( rlen ) 
			*rlen = i;
	}

	return code;
}


int csv_clear_field(struct csv_state *csv)
{
	char buf[256];
	int code;
	while ( (code = csv_next(csv, buf, sizeof(buf), 0)) == CSV_CNT )
		;
	return code;
}
