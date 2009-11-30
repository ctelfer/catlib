#include <stdio.h>
#include <cat/catstr.h>

const char *foo()
{
	CS_DECLARE_Q(static, str1, 32);
	CS_DECLARE(str2, 16);
	cs_set_cstr(&str2, "hello world");
	cs_format_d(&str1, "%s: %d\n", cs_to_cstr(&str2), 5);
	return cs_to_cstr(&str1);
}

int main(int argc, char *argv[])
{
	puts(foo());
	return 0;
}
