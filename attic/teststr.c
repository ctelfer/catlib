/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/cat.h>
#include <cat/str.h>
#include <cat/aux.h>
#include <cat/err.h>
#include <cat/mem.h>
#include <string.h>

int main(int argc, char *argv[])
{
	struct cstring *s1, *s2, *s3, *s4, *s5, *s6, *s7;
	const char *foo = "howdy there folks";
	const char *bar = "another random string";
	const char *pat = " fo";
	char *p;
	int r, start, end;

	s1 = str_new("Hello World", -1, &estdcmem);
	s2 = str_copy(s1);
	s3 = str_slice(s1, 3, 8, 1);
	s4 = str_new(NULL, 4, &estdcmem);
	if (str_grow(&s4, strlen(foo) + 1) < 0)
		errsys("Couldn't grow string to length %d", strlen(foo) + 1);
	else
		printf("s4's length is now %d\n", s4->buf.len);
	str_scopy(s4, foo);

	s5 = str_fnew(&estdcmem, "String 4 is (%s) len = %d",
		      s4->buf.data, str_len(s4));

	s6 = str_new(NULL, 900, &estdcmem);
	str_set(s6, bar, strlen(bar)+1);
	s7 = str_splice(s2, 3, 8, s6, 1);
	printf("String 1 = %s\n", str_data(s1));
	printf("String 2 = %s\n", str_data(s2));
	printf("String 3 = %s\n", str_data(s3));
	printf("String 4 = %s\n", str_data(s4));
	printf("String 5 = %s\n", str_data(s5));
	printf("String 6 = %s\n", str_data(s6));
	printf("String 7 = %s\n", str_data(s7));
	if ( str_cmp(s1, s2) == 0 )
		printf("String 1 == String 2\n");
	else
		printf("String 1 != String 2\n");

	r = str_cmp(s1, s3);
	if ( r == 0 )
		printf("String 1 == String 3\n");
	else if ( r < 0 )
		printf("String 1 < String 3\n");
	else
		printf("String 1 > String 3\n");

	p = strstr(str_data(s2), "llo");
	printf("\"llo\" is at %d,%d in String 2\n", p - str_data(s2),
	       p - str_data(s2) + strlen("llo"));

	start = str_find(s4, pat, -1);
	end = strlen(pat) + start;
	if ( start > 0 )
		printf("(%s) is at (%d,%d) in (%s) *len = -1*\n", 
		       pat, start, end, str_data(s4));
	else
		printf("(%s) is not in (%s) *len = -1*\n", pat, str_data(s4));

	start = str_find(s4, pat, strlen(pat));
	end = strlen(pat) + start;
	if ( start > 0 )
		printf("(%s) is at (%d,%d) in (%s) *len = %d*\n", 
		       pat, start, end, str_data(s4), strlen(pat));
	else
		printf("(%s) is not in (%s) *len = %d*\n", 
		       pat, str_data(s4), strlen(pat));

	start = str_find(s5, pat, strlen(pat));
	end = strlen(pat) + start;
	if ( start > 0 )
		printf("(%s) is at (%d,%d) in (%s)\n", 
		       pat, start, end, str_data(s5));
	else
		printf("(%s) is not in (%s)\n", pat, str_data(s5));

	printf("Freeing them all\n");
	str_free(s1); str_free(s2); str_free(s3);
	str_free(s4); str_free(s5); str_free(s6);
	str_free(s7);
	return 0;
}
