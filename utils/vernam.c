/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <cat/stduse.h>

#define MAXLINE	256

void decode(char *c1, char *c2, int add, int off)
{
	char out[MAXLINE], *op = out;
	int i, v1, v2, outc;

	while ( *c1 != '\0' && *c2 != '\0' ) {
		while ( *c1 != '\0' && !isalpha(*c1) )
			c1++;
		while ( *c2 != '\0' && !isalpha(*c2) )
			c2++;
		if ( *c1 == '\0' || *c2 == '\0' )
			break;

		v1 = toupper(*c1) - 'A';
		v2 = toupper(*c2) - 'A' + off;
		if (!add) {
			outc = v1 - v2;
			while ( outc < 0 )
				outc += 26;
			outc += 'A';
		} else {
			outc = (v1 + v2) % 26 + 'A';
		}
		/* 
		printf("%c (%2d), %c (%2d) -> %c (%2d)\n", toupper(*c1), v1, 
			toupper(*c2), v2, outc, outc - 'A' + 1);
		*/
		*op++ = outc;
		c1++;
		c2++;
	}

	*op = '\0';
	printf("Output: add = %d, off = %d\n%s\n\n", add, off, out);
}

int main(int argc, char *argv[])
{
	char line1[MAXLINE];
	char line2[MAXLINE];
	int off;

	if ( fgets(line1, sizeof(line1), stdin) == NULL )
		err("no first line!\n");
	if ( fgets(line2, sizeof(line2), stdin) == NULL )
		err("no first line!\n");

	for ( off = 0 ; off < 26 ; ++off )
		decode(line1, line2, 1, off);
	for ( off = 0 ; off < 26 ; ++off )
		decode(line1, line2, 0, off);

	return 0;
}
