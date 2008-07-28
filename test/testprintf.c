#include <cat/cat.h>
#include <cat/emit.h>
#include <cat/str.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>



int ntests = 0, npassed = 0;

void test_printf(const char *fmt, ...)
{
	va_list ap, ap2;
	char buf1[256];
	char buf2[256];
	int rv1, rv2, passed;

	printf("format = |%s|\n", fmt);

	va_start(ap, fmt);
	rv1 = vsnprintf(buf1, sizeof(buf1), fmt, ap);
	va_end(ap);
	printf("native: rv = %d, val = |%s|\n", rv1, buf1);
	fflush(stdout);

	va_start(ap2, fmt);
	rv2 = str_vfmt(buf2, sizeof(buf2), fmt, ap2);
	va_end(ap2);
	printf("catlib: rv = %d, val = |%s|\n", rv2, buf2);
	fflush(stdout);

	++ntests;
	passed = ((strcmp(buf1, buf2) == 0) && (rv1 == rv2));
	if ( passed )
		++npassed;

	printf("result -> %s\n\n", passed ?  "match!" : "no match" );
}


void test_extension(const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	int rv;
	printf("format = |%s|\n", fmt);
	va_start(ap, fmt);
	rv = str_vfmt(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	printf("catlib: rv = %d, val = |%s|\n\n", rv, buf);
	fflush(stdout);
}


int main(int argc, char *argv[])
{
	test_printf("hi");
	test_printf("%d", 5);
	test_printf("%3d", 5);
	test_printf("%-3d", 5);
	test_printf("abc - %s - def", "goodbye");
	test_printf("pointer %p", (void *)&argc);
	test_printf("null pointer %p", NULL);
	test_printf("%f", 1.0);
	test_printf("%+f", 1.01);
	test_printf("%f", .01);
	test_printf("% f", .01);
	test_printf("%0f", .01);
	test_printf("%7.3f", .01);
	test_printf("%7.3f", .0123);
	test_printf("%07.3f", .0123);
	test_printf("%7.3e", .0123);
	test_printf("%.3e", 123000.25);
	test_printf("%.10E", 123000.25);
	test_printf("%f", 1.0 / 0.0);
	test_printf("%f", -1.0 / 0.0);
	test_printf("hell%c w%crld", 'o', 'o');
	test_printf("%o", 0765);
	test_printf("%X", 0xcafebabe);
	test_printf("%x", 0xdeadbeef);
	test_printf("%u", 82);
	test_printf("%u", -10);
	test_printf("%7.3g", .01);
	test_printf("%7.3G", .0123);
	test_printf("%.3g", 123000.25);
	test_printf("%.0g", 123000.25);
	printf("Extensions\n");
	test_extension("%032b", 0xabcdef38);
	test_extension("%032b", 0x58);
	test_extension("%b", 0x58);

	printf("%d / %d tests passed\n", npassed, ntests);
	return npassed == ntests ? 0 : ntests;
}
