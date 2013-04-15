/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <string.h>

#include <cat/crypto.h>

void test_arc4()
{
  struct arc4ctx arc4;
  int i, j, match;
  ulong len;

  unsigned char test_vectors[3][3][32] = {
    {"Key", "Plaintext", "\xBB\xF3\x16\xE8\xD9\x40\xAF\x0A\xD3"},
    {"Wiki", "pedia", "\x10\x21\xBF\x04\x20"},
    {"Secret", "Attack at dawn", 
      "\x45\xA0\x1F\x64\x5F\xC3\x5B\x38\x35\x52\x54\x4B\x9B\xF5"}
  };
  unsigned char out[32];

  match = 1;
  for ( i = 0; i < 3; ++i ) {
    arc4_init(&arc4, test_vectors[i][0], strlen(test_vectors[i][0]));
    len = strlen(test_vectors[i][1]);
    arc4_encrypt(&arc4, test_vectors[i][1], out, len);
    printf("RC4|  Plaintext:'%s', Key:'%s', Ciphertext:'",
           test_vectors[i][0], test_vectors[i][1]);
    for ( j = 0; j < len; ++j ) {
      printf("%02x", out[j]);
      match = match && (out[j] == test_vectors[i][2][j]);
    }
    printf("' (%s)\n", match ? "match" : "no match");
  }
}


void test_sha256()
{
  struct sha256ctx s256;
  char str1[] = "abc";
  char str2[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  byte_t out[32];
  byte_t res1[32] = "\xBA\x78\x16\xBF\x8F\x01\xCF\xEA\x41\x41\x40\xDE"
	 	    "\x5D\xAE\x22\x23\xB0\x03\x61\xA3\x96\x17\x7A\x9C"
		    "\xB4\x10\xFF\x61\xF2\x00\x15\xAD";
  byte_t res2[32] = "\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26\x93"
	            "\x0C\x3E\x60\x39\xA3\x3C\xE4\x59\x64\xFF\x21\x67"
		    "\xF6\xEC\xED\xD4\x19\xDB\x06\xC1";
  int i;

  sha256(str1, sizeof(str1)-1, out);
  printf("sha256 hash of \"%s\" is:\n\t", str1);
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", out[i]);
  printf("\n");
  printf("Expected value is:\n\t");
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", res1[i]);
  printf("\n");
  if (memcmp(out, res1, 32))
	  printf("FAILURE!\n");
  else
	  printf("SUCCESS\n");
  printf("\n");

  sha256(str2, sizeof(str2)-1, out);
  printf("sha256 hash of \"%s\" is: \n\t", str2);
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", out[i]);
  printf("\n");
  printf("Expected value is:\n\t");
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", res2[i]);
  printf("\n");
  if (memcmp(out, res2, 32))
	  printf("FAILURE!\n");
  else
	  printf("SUCCESS\n");
}

int main(int argc, char *argv[])
{
  test_arc4();
  test_sha256();
  return 0;
}
