#include <cat/cat.h>
#include <stdarg.h>
#include <limits.h>
#include <cat/emit.h>
#include <cat/emit_format.h>
#if CAT_USE_STDLIB
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


enum {
	ARG_REG,
	ARG_HALF,
	ARG_LONG,
	ARG_LONGLONG,
	ARG_LONGDBL
};


struct format_params {
	int fmtchar;
	int precision;
	int minwidth;
	int alternate;
	int posspace;
	int rightjust;
	int alwayssign;
	int zerofill;
	int argsize;
	int capver;
};


struct va_list_s {
	va_list ap;
};


#ifndef va_copy
#ifndef __va_copy
#error "Neither va_copy, nor __va_copy are defined!"
#else
#define va_copy(d,s) __va_copy(d,s)
#endif
#endif

typedef int (*format_f)(struct emitter *em, struct format_params *fp,
						struct va_list_s *app, int *flen);


#define EMIT_CHAR(em, ch)                               \
	do {                                            \
		if ( ((em)->emit_state != EMIT_EOS) &&	\
		     (emit_char((em), (ch)) < 0) )	\
			return -1;                      \
	} while(0)
			

#define EMIT_STRING(em, ch)				\
	do {						\
		if ( ((em)->emit_state != EMIT_EOS) &&  \
		     (emit_string((em), (ch)) < 0) )	\
			return -1;			\
	} while(0)
			

static int Emit_n_char(struct emitter *em, uchar ch, int times)
{
	while ( times > 0 ) {
		if ( ((em)->emit_state != EMIT_EOS) && 
		     (emit_char((em), (ch)) < 0) )
			return -1;
		--times;
	}
	return 0;
}


static void init_format_params(struct format_params *fp)
{
	abort_unless(fp);
	fp->fmtchar = ' ';
	fp->precision = -1;
	fp->minwidth = -1;
	fp->alternate = 0;
	fp->posspace = 0;
	fp->rightjust = 1;
	fp->alwayssign = 0;
	fp->zerofill = 0;
	fp->argsize = ARG_REG;
	fp->capver = 0;
}


static int is_fmt_char(char ch)
{
	return (isalpha(ch) || (ch == '%')) &&
		(ch != 'h') && (ch != 'l') && (ch != 'L');
}


static int get_format_flags(const char **fmtp, struct format_params *fp,
		            struct va_list_s *app)
{
	const char *fmt;
	char *fp2;
	int pastperiod = 0;
	int pastprec = 0;
	int nextc;
	int argval;
	long val;

	abort_unless(fmtp && *fmtp && fp);

	fmt = *fmtp;

	while ( !is_fmt_char(nextc = *fmt) ) {

		/* handle precision or width case */
		if ( isdigit(nextc) && (nextc != '0' || pastperiod) ) {
			if ( pastprec )
				return -1;

			val = strtol(fmt, &fp2, 10);
			fmt = (const char *)fp2;
			if ( val > INT_MAX )
				return -1;

			if ( pastperiod ) {
				if ( fp->precision > 0 )
					return -1;
				fp->precision = val;
			} else {
				if ( fp->minwidth > 0 )
					return -1;
				fp->minwidth = val;
			}
			continue;
		}

		/* handle remaining format modifiers */
		switch ( nextc ) {
			case '\0': 
				return -1;
			case '-':
				fp->rightjust = 0;
				break;
			case '+':
				fp->alwayssign = 1;
				break;
			case ' ':
				fp->posspace = 1;
				break;
			case '0':
				fp->zerofill = 1;
				break;
			case '#':
				fp->alternate = 1;
				break;
			case '*':
				if ( (argval = va_arg(app->ap, int)) < 0 )
					return -1;
				if ( pastperiod ) {
					if ( fp->precision > 0 )
						return -1;
					fp->precision = argval;
				} else {
					if ( fp->minwidth > 0 )
						return -1;
					fp->minwidth = argval;
				}
				break;
			case '.':
				if ( pastperiod )
					return -1;
				pastperiod = 1;
				break;
			case 'h':
				if ( fp->argsize != ARG_REG )
					return -1;
				fp->argsize = ARG_HALF;
				break;
			case 'l':
				if ( fp->argsize != ARG_REG )
					return -1;
#if CAT_HAS_LONGLONG
				if ( *(fmt+1) == 'l' ) {
					++fmt;
					fp->argsize = ARG_LONGLONG;
				} else {
					fp->argsize = ARG_LONG;
				}
#else
				fp->argsize = ARG_LONG;
#endif /* CAT_HAS_LONGLONG */
				break;
			case 'L':
				if ( fp->argsize != ARG_REG )
					return -1;
				fp->argsize = ARG_LONGDBL;
				break;
			default:
				return -1;
		}

		fmt++;
	}

	fp->fmtchar = nextc;
	*fmtp = fmt + 1;

	return 0;
}


static void reverse_string(char *s, size_t len)
{
	char *e, c;
	abort_unless(s);
	if ( len == 0 )
		return;
	e = s + len - 1;

	while ( s < e ) {
		c = *s;
		*s = *e;
		*e = c;
		++s;
		--e;
	}
}


static int fmt_char(struct emitter *em, struct format_params *fp, 
		    struct va_list_s *app, int *flen)
{
	char ch;

	abort_unless(em && fp && app && flen);

	if ( fp->argsize != ARG_REG )
		return -1;

	ch = va_arg(app->ap, int);

	if ( fp->minwidth > 0 && fp->rightjust ) {
		if ( Emit_n_char(em, ' ', fp->minwidth - 1) < 0 )
			return -1;
	}
	EMIT_CHAR(em, ch);
	if ( fp->minwidth > 0 && !fp->rightjust ) {
		if ( Emit_n_char(em, ' ', fp->minwidth - 1) < 0 )
			return -1;
	}

	if ( fp->minwidth > 0 )
		*flen = fp->minwidth;
	else
		*flen = 1;

	return 0;
}


static int fmt_str_help(char *s, struct emitter *em, struct format_params *fp, 
		        int *flen)
{
	size_t slen;

	slen = strlen(s);
	abort_unless(slen <= INT_MAX);

	if ( fp->precision > 0 && fp->precision < slen )
		slen = fp->precision;

	if ( fp->minwidth > 0 && fp->rightjust && fp->minwidth > slen ) {
		if ( Emit_n_char(em, ' ', fp->minwidth - slen) < 0 )
			return -1;
	}
	EMIT_STRING(em, s);
	if ( fp->minwidth > 0 && !fp->rightjust && fp->minwidth > slen ) {
		if ( Emit_n_char(em, ' ', fp->minwidth - slen) < 0 )
			return -1;
	}

	if ( fp->minwidth > 0 && fp->minwidth > slen )
		*flen = fp->minwidth;
	else
		*flen = slen;

	return 0;
}


static int fmt_str(struct emitter *em, struct format_params *fp,
		   struct va_list_s *app, int *flen)
{
	char *s;

	abort_unless(em && fp && app && flen);

	if ( fp->argsize != ARG_REG )
		return -1;

	s = va_arg(app->ap, char *);

	return fmt_str_help(s, em, fp, flen);
}


#if CAT_HAS_LONGLONG
#define CAT_MAXUTYPE ulonglong
#define CAT_MAXSTYPE long long
#else /* CAT_HAS_LONGLONG */
#define CAT_MAXUTYPE ulong
#define CAT_MAXSTYPE long
#endif /* CAT_HAS_LONGLONG */


static CAT_MAXUTYPE get_u_arg(struct format_params *fp, struct va_list_s *app)
{
	CAT_MAXUTYPE v = 0;

	abort_unless(fp && app);

	if ( fp->argsize == ARG_LONGDBL )
		return -1;
	else if ( fp->argsize == ARG_REG )
		v = va_arg(app->ap, uint);
	else if ( fp->argsize == ARG_LONG )
		v = va_arg(app->ap, ulong);
	else if ( fp->argsize == ARG_HALF )
		v = va_arg(app->ap, uint);
#if CAT_HAS_LONGLONG
	else if ( fp->argsize == ARG_LONGLONG )
		v = va_arg(app->ap, ulonglong);
#endif /* CAT_HAS_LONGLONG */
	else
		abort_unless(0);

	return v;
}


static CAT_MAXSTYPE get_s_arg(struct format_params *fp, struct va_list_s *app)
{
	CAT_MAXSTYPE v = 0;

	abort_unless(fp && app);

	if ( fp->argsize == ARG_LONGDBL )
		return -1;
	else if ( fp->argsize == ARG_REG )
		v = va_arg(app->ap, int);
	else if ( fp->argsize == ARG_LONG )
		v = va_arg(app->ap, long);
	else if ( fp->argsize == ARG_HALF )
		v = va_arg(app->ap, int);
#if CAT_HAS_LONGLONG
	else if ( fp->argsize == ARG_LONGLONG )
		v = va_arg(app->ap, long long);
#endif /* CAT_HAS_LONGLONG */
	else
		abort_unless(0);

	return v;
}


static int fmt_int(struct emitter *em, struct format_params *fp,
		   struct va_list_s *app, int *flen)
{
	CAT_MAXSTYPE v;
	char buf[sizeof(v) * 3 + 1] = { 0 };
	int i, ndigits, tnlen, plen = 0, neg = 0;

	abort_unless(em && fp && app && flen);

	v = get_s_arg(fp, app);
	if ( v < 0 || fp->posspace || fp->alwayssign )
		plen = 1;
	if ( v < 0 )
		neg = 1;

	/* Pull off the first digit and divide by 10 so we don't have to */
	/* worry about how -INT_MIN == INTMIN in 2s compliment */
	if ( v == 0 ) {
		buf[0] = '0';
	} else {
		int firstd = v % 10;
		if ( firstd < 0 )
			firstd = -firstd;
		buf[0] = '0' + firstd;
	}
	v /= 10;
	/* convert to a positive number */
	if ( v < 0 )
		v = -v;

	/* pull off the remaining digits (in reverse order) */
	i = 1;
	while ( v > 0 ) {
		int xd = v % 10;
		buf[i++] = xd + '0';
		v /= 10;
	}
	/* flip it back to the correct order */
	reverse_string(buf, i);

	ndigits = i;
	if ( fp->precision >= 0 && fp->precision > ndigits )
		ndigits = fp->precision;
	if ( fp->minwidth >= 0 && fp->minwidth > ndigits + plen  &&
			 fp->zerofill )
		ndigits = fp->minwidth - plen;
	tnlen = ndigits + plen;

	if ( fp->minwidth >= 0 && fp->minwidth > tnlen && fp->rightjust )
		if ( Emit_n_char(em, ' ', fp->minwidth - tnlen) < 0 )
			return -1;

	if ( neg ) {
		EMIT_CHAR(em, '-');
	} else if ( fp->alwayssign ) {
		EMIT_CHAR(em, '+');
	} else if ( fp->posspace ) {
		EMIT_CHAR(em, ' ');
	}

	if ( ndigits > i ) {
		if ( Emit_n_char(em, '0', ndigits - i) < 0 )
			return -1;
	}

	EMIT_STRING(em, buf);

	if ( fp->minwidth >= 0 && fp->minwidth > tnlen && !fp->rightjust )
		if ( Emit_n_char(em, ' ', fp->minwidth - tnlen) < 0 )
			return -1;

	if ( fp->minwidth >= 0 && fp->minwidth > tnlen )
		*flen = fp->minwidth;
	else
		*flen = tnlen;

	return 0;
}


static int fmt_u(struct emitter *em, struct format_params *fp, int *flen, 
		 int radix, char *apfx, CAT_MAXUTYPE v)
{
	char buf[sizeof(v) * 8 + 1] = { 0 };
	int i, ndigits, tnlen, plen = 0;

	abort_unless(em && fp && flen && apfx);
	abort_unless(radix >= 2 && radix <= 36);

	/* compute the alternate prefix length */
	if ( fp->alternate && (fp->fmtchar != 'p' || v != 0) ) 
		plen = strlen(apfx);

	if ( v == 0 )
		buf[0] = '0';
	i = 0;
	while ( v > 0 ) {
		int xd = v % radix;
		if ( xd < 10 )
			buf[i++] = xd + '0';
		else if ( fp->capver )
			buf[i++] = xd - 10 + 'A';
		else
			buf[i++] = xd - 10 + 'a';
		v /= radix;
	}
	if ( i == 0 )
		i = 1;
	reverse_string(buf, i);

	ndigits = i;
	if ( fp->precision >= 0 && fp->precision > ndigits )
		ndigits = fp->precision;
	if ( fp->minwidth >= 0 && 
			 fp->minwidth > ndigits + (fp->alternate ? plen : 0) &&
			 fp->zerofill )
		ndigits = fp->minwidth - (fp->alternate ? plen : 0);
	tnlen = ndigits + (fp->alternate ? plen : 0);

	if ( fp->minwidth >= 0 && fp->minwidth > tnlen && fp->rightjust )
		if ( Emit_n_char(em, ' ', fp->minwidth - tnlen) < 0 )
			return -1;

	if ( fp->alternate ) {
		int i, ch;
		for ( i = 0 ; i < plen ; ++i ) {
			ch = fp->capver ? toupper(apfx[i]) : apfx[i];
			EMIT_CHAR(em, ch);
		}
	}

	if ( ndigits > i )
		if ( Emit_n_char(em, '0', ndigits - i) < 0 )
			return -1;

	EMIT_STRING(em, buf);

	if ( fp->minwidth >= 0 && fp->minwidth > tnlen && !fp->rightjust )
		if ( Emit_n_char(em, '0', fp->minwidth - tnlen) < 0 )
			return -1;

	if ( fp->minwidth >= 0 && fp->minwidth > tnlen )
		*flen = fp->minwidth;
	else
		*flen = tnlen;

	return 0;
}


static int fmt_hex(struct emitter *em, struct format_params *fp,
		   struct va_list_s *app, int *flen)
{
	abort_unless(em && fp && app && flen);
	return fmt_u(em, fp, flen, 16, "0x", get_u_arg(fp, app));
}


static int fmt_oct(struct emitter *em, struct format_params *fp,
		   struct va_list_s *app, int *flen)
{
	abort_unless(em && fp && app && flen);
	return fmt_u(em, fp, flen, 8, "0", get_u_arg(fp, app));
}


static int fmt_uint(struct emitter *em, struct format_params *fp,
		    struct va_list_s *app, int *flen)
{
	abort_unless(em && fp && app && flen);
	return fmt_u(em, fp, flen, 10, "", get_u_arg(fp, app));
}



static int fmt_binary(struct emitter *em, struct format_params *fp,
		      struct va_list_s *app, int *flen)
{
	abort_unless(em && fp && app && flen);
	return fmt_u(em, fp, flen, 2, "0b", get_u_arg(fp, app));
}


static int get_double_arg(struct format_params *fp, struct va_list_s *app, 
		          long double *rv)
{
	long double v;

	abort_unless(fp && app && rv);

	if ( fp->argsize == ARG_LONGDBL )
		v = va_arg(app->ap, long double);
	else if ( fp->argsize == ARG_REG )
		v = va_arg(app->ap, double);
	else
		return -1;

	*rv = v;
	return 0;
}


#define isNaN(d) ((d) != (d))
#define isinf(d) ((FLOAT_MIN / (d)) == 0.0)
#define MAX_POWER 8192
#define INF (MAX_POWER + 1)
#define NEGINF (-MAX_POWER - 1)

/* We can improve on this by using precomputed tables of exponentially */
/* increasing value and then locate where the value falls in the table */
/* The divide out the lower bound for the value, keep the exponent and repeat */
static int find_power(long double d) 
{
	int e = 0;
	int neg = 0;

	if ( d == 0.0 )
		return 0;

	if ( d < 0 ) {
		d = 0.0 - d;
		neg = 1;
	}

	if ( d >= 10.0 ) { 
		while ( d >= 10.0 ) {
			d /= 10.0;
			e++;
			/* XXX calculate this constant or do a better test */
			/* for infinity (e.g. float_min / e == 0) */
			if ( e > MAX_POWER )
				return (neg ? NEGINF : INF);
		}
	} else if ( d < 1.0 ) { 
		while ( d < 1.0 ) {
			d *= 10.0;
			e--;
			abort_unless(e >= -MAX_POWER);
		}
	}

	return e;
}


static long double p10(int power) 
{
	long double val = 1.0;
	long double mul = 10.0;
	abort_unless(power >= 0);
	while ( power > 0 ) {
		if ( (power & 1) == 1 )
			val *= mul;
		mul *= mul;
		power >>= 1;
	}
	return val;
}

#define STRIP_ALT	1
#define STRIP_FFMT	2
static int adjust_precision(long double v, int power, int prec, int flags)
{
	int i;
	int zlen = 0;
	uchar digit;

	abort_unless(prec >= 0);

	if ( v < 0 )
		v = 0.0 - v;

	if ( prec == 0 )
		return 0;

	if ( flags & STRIP_FFMT ) {
		if ( power < 0 ) {
			prec += -power - 1;
		} else {
			if ( prec <= power + 1 )
				return 0;
			prec -= power + 1;
		}
	} else {
		if ( power < 0 )
			v *= p10(-power);
		else if ( power > 0 ) 
			v /= p10(power);
		--prec;
	}

	if ( flags & STRIP_ALT )
		return prec;

	digit = 0;
	for ( i = 0; i < prec; ++i ) {
		v *= 10.0;
		v -= digit * 10.0;
		digit = (uchar)v % 10;
		if ( digit == 0 )
			zlen++;
		else
			zlen = 0;
	}

	return prec - zlen;
}


static int fmt_double_help(long double v, struct emitter *em, 
		           struct format_params *fp, int *flen)
{
	int prec = 6;
	int signlen = 0;
	int periodlen = 0;
	int power, opower;
	int spaces = 0;
	int tlen;
	uchar digit;

	abort_unless(em && fp && flen);

	if ( isNaN(v) )
		return fmt_str_help("NaN", em, fp, flen);

	if ( fp->precision >= 0 )
		prec = fp->precision;

	opower = power = find_power(v);
	if ( power == INF )
		return fmt_str_help("inf", em, fp, flen);
	else if ( power == NEGINF )
		return fmt_str_help("-inf", em, fp, flen);

	if ( toupper(fp->fmtchar) == 'G' ) {
		int flags = STRIP_FFMT;
		if ( fp->alternate )
			flags |= STRIP_ALT;
		prec = adjust_precision(v, power, prec, flags);
	}
	signlen = (fp->alwayssign || v < 0 || fp->posspace) ? 1 : 0;
	periodlen = (prec == 0 && !fp->alternate) ? 0 : 1;

	tlen = (power <= 0) ? 1 : power + 1;
	tlen += prec + periodlen + signlen;
	if ( fp->minwidth > tlen )
		spaces = fp->minwidth - tlen;

	/* Right justification space padding */
	if ( spaces > 0 && fp->rightjust && !fp->zerofill ) {
		if ( Emit_n_char(em, ' ', spaces) < 0 )
			return -1;
	}

	/* Add sign */
	if ( v < 0.0 ) {
		EMIT_CHAR(em, '-');
		v = 0.0 - v;
	} else if ( fp->alwayssign ) {
		EMIT_CHAR(em, '+');
	} else if ( fp->posspace ) {
		EMIT_CHAR(em, ' ');
	}

	if ( spaces > 0 && fp->rightjust && fp->zerofill ) {
		if ( Emit_n_char(em, '0', spaces) < 0 )
			return -1;
	}

	/* integral portion */
	if ( power >= 0 ) {
		/* Normalize to exponential form */
		/* XXX This is a highly inaccurate way to generate the digits */
		/* but since this library has NO knowledge of unerlying */
		/* representation, this is the best I have for now */
		v /= p10(power);
		while ( power >= 0 ) {
			digit = (uchar)v  % 10;
			EMIT_CHAR(em, digit + '0');
			v *= 10.0;
			v -= digit * 10.0;
			--power;
		}
	} else {
		EMIT_CHAR(em, '0');
		v *= 10.0;
	}

	if ( prec > 0 || fp->alternate ) {
		EMIT_CHAR(em, '.');
	}

	/* fractional portion */
	while ( prec > 0 ) {
		digit = (uchar)v % 10;
		EMIT_CHAR(em, digit + '0');
		v *= 10.0;
		v -= digit * 10.0;
		--prec;
	}
	
	/* left justification spaces */
	if ( spaces > 0 && !fp->rightjust ) {
		if ( Emit_n_char(em, ' ', spaces) < 0 )
			return -1;
	}

	*flen = tlen + spaces;

	return 0;
}


static int fmt_double(struct emitter *em, struct format_params *fp,
		      struct va_list_s *app, int *flen)
{
	long double v;
	abort_unless(em && fp && app && flen);
	if ( get_double_arg(fp, app, &v) < 0 )
		return -1;
	return fmt_double_help(v, em, fp, flen);

}


static int fmt_exp_double_help(long double v, struct emitter *em, 
		               struct format_params *fp, int *flen)
{
	int prec = 6;
	int signlen = 0;
	int periodlen = 0;
	int explen = 4;
	int power, opower;
	int spaces = 0;
	int tlen;
	int i;
	uchar digit;
	char exp[8];

	abort_unless(em && fp && flen);

	if ( isNaN(v) )
		return fmt_str_help("NaN", em, fp, flen);

	if ( fp->precision >= 0 )
		prec = fp->precision;

	opower = power = find_power(v);
	if ( power == INF )
		return fmt_str_help("inf", em, fp, flen);
	else if ( power == NEGINF )
		return fmt_str_help("-inf", em, fp, flen);

	if ( power < 0 )
		power = -power;
	while ( power > 99 ) {
		explen++;
		power /= 10;
	}
	if ( toupper(fp->fmtchar) == 'G' ) {
		int flags = 0;
		if ( fp->alternate )
			flags |= STRIP_ALT;
		prec = adjust_precision(v, power, prec, fp->alternate);
	}
	signlen = (fp->alwayssign || v < 0.0 || fp->posspace) ? 1 : 0;
	periodlen = (prec == 0 && !fp->alternate) ? 0 : 1;

	tlen = 1 + periodlen + prec + signlen + explen;
	if ( fp->minwidth > tlen )
		spaces = fp->minwidth - tlen;

	/* Right justification space padding */
	if ( spaces > 0 && fp->rightjust && !fp->zerofill ) {
		if ( Emit_n_char(em, ' ', spaces) < 0 )
			return -1;
	}

	/* Add sign */
	if ( v < 0.0 ) {
		EMIT_CHAR(em, '-');
		v = 0.0 - v;
	} else if ( fp->alwayssign ) {
		EMIT_CHAR(em, '+');
	} else if ( fp->posspace ) {
		EMIT_CHAR(em, ' ');
	}

	if ( spaces > 0 && fp->rightjust && fp->zerofill ) {
		if ( Emit_n_char(em, '0', spaces) < 0 )
			return -1;
	}

	power = opower;
	if ( power > 0 )
		v /= p10(power);
	else
		v *= p10(-power);

	/* print first digit */
	digit = (uchar)v  % 10;
	EMIT_CHAR(em, digit + '0');
	v *= 10.0;
	v -= digit * 10.0;

	if ( prec > 0 || fp->alternate )
		EMIT_CHAR(em, '.');

	while ( prec > 0 ) {
		digit = (uchar)v  % 10;
		EMIT_CHAR(em, digit + '0');
		v *= 10.0;
		v -= digit * 10.0;
		--prec;
	}

	EMIT_CHAR(em, isupper(fp->fmtchar) ? 'E' : 'e');

	power = opower;
	EMIT_CHAR(em, (power < 0) ? '-' : '+');
	
	if ( power < 0 )
		power =  -power;
	if ( power < 10 )
		EMIT_CHAR(em, '0');

	if ( power == 0 ) {
		exp[0] = '0';
		i = 1;
	} else {
		for ( i = 0 ; power > 0 ; ++i, power /= 10 )
			exp[i] = (power % 10) + '0';
	}
	exp[i] = '\0';
	reverse_string(exp, i);

	EMIT_STRING(em, exp);

	*flen = tlen + spaces;

	return 0;
}


static int fmt_exp_double(struct emitter *em, struct format_params *fp,
		          struct va_list_s *app, int *flen)
{
	long double v;
	abort_unless(em && fp && app && flen);
	if ( get_double_arg(fp, app, &v) < 0 )
		return -1;
	return fmt_exp_double_help(v, em, fp, flen);

}


static int fmt_variable_double(struct emitter *em, struct format_params *fp,
		               struct va_list_s *app, int *flen)
{
	long double v;
	int power;
	int prec = 6;

	abort_unless(em && fp && app && flen);
	if ( get_double_arg(fp, app, &v) < 0 )
		return -1;

	if ( fp->precision >= 0 )
		prec = fp->precision;
	power = find_power(v);

	if ( power < -4 || power >= prec )
		return fmt_exp_double_help(v, em, fp, flen);
	else 
		return fmt_double_help(v, em, fp, flen);
}


static int fmt_ptr(struct emitter *em, struct format_params *fp,
		   struct va_list_s *app, int *flen)
{
	CAT_MAXUTYPE v;

	abort_unless(em && fp && app && flen);

	if ( fp->argsize != ARG_REG )
		return -1;

	v = (CAT_MAXUTYPE)va_arg(app->ap, void *);
	/* Adjust for size differences in case (void *) is treated as signed */
	/* before promoting it to a CAT_MAXUTYPE! This assumes a flat address */
	/* model which won't be correct for obscure hardware XXX */
	if ( sizeof(CAT_MAXUTYPE) > sizeof(void *) ) {
		CAT_MAXUTYPE mask = 0;
		mask = ~mask;
		mask >>= (sizeof(CAT_MAXUTYPE) - sizeof(void *)) * CHAR_BIT;
		v &= mask;
	}
	if ( v == 0 )
		return fmt_str_help("(nil)", em, fp, flen);

	fp->alternate = 1;
	return fmt_u(em, fp, flen, 16, "0x", v);
}


struct {
	char		fmtchar;
	format_f	formatter;
} format_tab[] = { 
	{ 'd', fmt_int },
	{ 'i', fmt_int },
	{ 'o', fmt_oct },
	{ 'x', fmt_hex },
	{ 'X', fmt_hex },
	{ 'u', fmt_uint },
	{ 'b', fmt_binary },
	{ 'c', fmt_char },
	{ 's', fmt_str },
	{ 'f', fmt_double },
	{ 'e', fmt_exp_double },
	{ 'E', fmt_exp_double },
	{ 'g', fmt_variable_double },
	{ 'G', fmt_variable_double },
	{ 'p', fmt_ptr },
};
static const int format_tab_len = sizeof(format_tab) / sizeof(format_tab[0]);


static format_f find_formatter(struct format_params *fp) 
{
	int i;
	uchar fmtchar;

	abort_unless(fp);
	fmtchar = fp->fmtchar;

	for ( i = 0; i < format_tab_len; ++i ) {
		if ( fmtchar == format_tab[i].fmtchar ) {
			if ( isupper(fmtchar) )
				fp->capver = 1;
			return format_tab[i].formatter;
		}
	}

	return NULL;
}


static int emit_format_help(struct emitter *em, const char **fmtp,
		            struct va_list_s *app, int *flen)
{
	struct format_params fp;
	format_f formatter;

	abort_unless(em && fmtp && *fmtp && app && flen);

	init_format_params(&fp);

	if ( get_format_flags(fmtp, &fp, app) < 0 )
		return -1;

	/* handle the %% case */
	if ( fp.fmtchar == '%' ) {
		EMIT_CHAR(em, '%');
		*flen = 1;
		return 0;
	} else if ( fp.fmtchar == '\0' )
		return -1;

	if ( (formatter = find_formatter(&fp)) == NULL )
		return -1;

	return (*formatter)(em, &fp, app, flen);
}


int emit_vformat(struct emitter *em, const char *fmt, va_list ap)
{
	int flen, len = 0;
	struct va_list_s val;
	const char *fs;

	va_copy(val.ap, ap);

	abort_unless(em && fmt);
	fs = fmt;
	while ( *fmt != '\0' ) {
		if ( *fmt == '%' ) {
			if ( fs != fmt ) {
				abort_unless(len < INT_MAX);
				emit_raw(em, fs, fmt - fs);
			}
			fmt++;
			if ( emit_format_help(em, &fmt, &val, &flen) < 0 )
				return -1;
			abort_unless(flen <= INT_MAX - len);
			len += flen;
			fs = fmt;
		} else {
			++fmt;
			++len;
		}
	}
	if ( fs != fmt ) {
		abort_unless(len < INT_MAX);
		emit_raw(em, fs, fmt - fs);
	}

	return len;
}


int emit_format(struct emitter *em, const char *fmt, ...)
{
	va_list ap;
	int rv;
	va_start(ap, fmt);
	rv = emit_vformat(em, fmt, ap);
	va_end(ap);
	return rv;
}
