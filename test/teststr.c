#include <cat/str.h>
#include <cat/err.h>
#include <cat/stduse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int allrandom = 0;


void print_raw_hex(void *dprm, size_t len)
{
	size_t i;
	unsigned char *d = dprm;

	for ( i = 0 ; i < len ; ++i,++d ) 
		printf("%02x", *d);
	printf("\n");
}


void test_regular_str()
{
	char buf1[8], *cp;
	size_t ret;
	int irv;

	cp = "Hello World";
	ret = str_copy(buf1, cp, sizeof(buf1));
	printf("src:/%s/, dst:/%s/, rv = %u\n", buf1, cp, (uint)ret);
	
	cp = " World";
	str_copy(buf1, "Hello", sizeof(buf1));
	ret = str_cat(buf1, cp, sizeof(buf1));
	printf("src:/%s/, dst:/%s/, rv = %u\n", buf1, cp, (uint)ret);
	
	irv = str_fmt(buf1, sizeof(buf1), "Hi! %d: %s\n", (int)ret, cp);
	printf("formatted: /%s/ ret: /%d/\n", buf1, irv);

	cp = "    Hello World";
	printf("space span of /%s/ = %d\n", cp, (int)str_span(cp, " "));

	cp = "Hello World";
	printf("non-space span of /%s/ = %d\n", cp, (int)str_cspan(cp, " "));

	cp = "Hello World";
	printf("first instance of /%s/ in /%s/ is at offset /%u/\n", 
		"o", cp, (uint)(str_findc(cp, "o") - cp));
}


unsigned long bytemasks[6] = { 
	0x0000007f,	/* 7 */
	0x000007ff,	/* 11 */
	0x0000ffff,	/* 16 */
	0x001fffff,	/* 21 */
	0x03ffffff,	/* 26 */
	0x7fffffff,	/* 31 */
};


int nbpermute[6] = { 4, 0, 3, 2, 1, 5 };


#define BLEN	10
void test_utf8_codec()
{
	unsigned long longs[BLEN], olongs[BLEN];
	char buf[BLEN * 6];
	const char *errp;
	unsigned short shorts[BLEN], oshorts[BLEN];
	size_t i, nch, clen;
	int nb, elen, maxclen;

	for ( i = 0; i < BLEN-1 ; ++i ) {
		if ( allrandom )
			nb = (unsigned long)random() % 6L;
		else
			nb = nbpermute[i % 6];
		olongs[i] = (unsigned long)random() & bytemasks[nb];
		nb %= 3;
		oshorts[i] = (unsigned long)random() & bytemasks[nb];
	}
	olongs[BLEN-1] = 0;
	oshorts[BLEN-1] = 0;

	printf("\n---Longs---\n");
	printf("utf32 before encode:\n");
	print_raw_hex(olongs, sizeof(olongs));
	if ( (elen = utf32_to_utf8(buf, sizeof(buf), olongs, BLEN)) < 0 )
		err("Error in utf32_to_utf8() encoding\n");

	if ( elen != strlen(buf) + 1 ) 
		err("Returned encoded length does not match strlen()\n");
	printf("utf8 after encode:\n");
	print_raw_hex(buf, elen);

	printf("Validating encoded form ...");
	if ( utf8_validate(buf, elen, &errp, 0) < 0 )
		err("encoded utf8 string not valid at pos: %u!\n", 
		    (uint)(errp - buf));
	printf("valid!\n");

	clen = utf8_nchars(buf, elen, &maxclen);
	if ( clen != BLEN )
		err("Encoded length doesn't seem to match original: %u vs %u\n",
		    BLEN, (uint)clen);
	printf("Character lengths match (%u).  Max char len = %d\n", (uint)clen,
	       maxclen);

	printf("re-decoding to utf32\n");
	if ( utf8_to_utf32(longs, BLEN, buf, elen) < 0 )
		err("Error decoding back to utf32");

	printf("utf8 after decode:\n");
	print_raw_hex(longs, sizeof(longs));
	for ( i = 0 ; i < BLEN ; ++i )
		if ( longs[i] != olongs[i] )
			err("Long # %d differs\n", i);
	printf("Decoded correctly!\n");


	printf("\n---Shorts---\n");

	printf("utf16 before encode:\n");
	print_raw_hex(oshorts, sizeof(oshorts));
	if ( (elen = utf16_to_utf8(buf, sizeof(buf), oshorts, BLEN)) < 0 )
		err("Error in utf26_to_utf8() encoding\n");

	if ( elen != strlen(buf) + 1 ) 
		err("Returned encoded length does not match strlen()\n");
	printf("utf8 after encode:\n");
	print_raw_hex(buf, elen);

	printf("Validating encoded form ...");
	if ( utf8_validate(buf, elen, &errp, 0) < 0 )
		err("encoded utf8 string not valid at pos: %u\n",
		    (uint)(errp - buf));
	printf("valid!\n");

	clen = utf8_nchars(buf, elen, &maxclen);
	if ( clen != BLEN )
		err("Encoded length doesn't seem to match original: %u vs %u\n",
		    BLEN, (uint)clen);
	printf("Character lengths match (%u).  Max char len = %d\n", (uint)clen,
	       maxclen);

	printf("re-decoding to utf16\n");
	if ( utf8_to_utf16(shorts, BLEN, buf, strlen(buf) + 1) < 0 )
		err("Error decoding back to utf16");

	printf("utf8 after decode:\n");
	print_raw_hex(shorts, sizeof(shorts));
	for ( i = 0 ; i < BLEN ; ++i )
		if ( shorts[i] != oshorts[i] )
			err("Short # %d differs\n", i);
	printf("Decoded correctly!\n");
}


void test_stduse_str()
{
}


int main(int argc, char *argv[])
{
	if ( argc > 1 && strcmp(argv[1], "-r") == 0) {
		allrandom = 1;
		srandom(time(NULL));
	} else
		srandom(0);
	test_regular_str();
	test_utf8_codec();
	test_stduse_str();

	return 0;
}
