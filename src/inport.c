#include <cat/inport.h>

int readchar(struct inport *in, char *ch)
{
	abort_unless(in && ch);
	return (*in->read)(in, ch);
}


static int string_inport_readchar(struct inport *in, char *ch)
{
	struct string_inport *sin = (struct string_inport *)in;
	if ( sin->cur >= sin->end )
		return READCHAR_END;
	*ch = *sin->cur;
	sin->cur += 1;
	return READCHAR_CHAR;
}


void string_inport_init(struct string_inport *sin, const char *s)
{
	abort_unless(sin && s);
	sin->in.read = &string_inport_readchar;
	sin->start = s;
	sin->cur = s;
	while ( *s != '\0' ) s++;
	sin->end = s;
}


void string_inport_reset(struct string_inport *sin)
{
	abort_unless(sin);
	sin->cur = sin->start;
}


void null_inport_init(struct inport *in)
{
}


struct inport null_inport = { 0 };

