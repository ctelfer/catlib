/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <cat/crypto.h>
#include <cat/raw.h>

void test_arc4()
{
  struct raw r1, r2;
  struct arc4ctx arc4;
  int i, j, match;

  unsigned char test_vectors[3][3][32] = {
    {"Key", "Plaintext", "\xBB\xF3\x16\xE8\xD9\x40\xAF\x0A\xD3"},
    {"Wiki", "pedia", "\x10\x21\xBF\x04\x20"},
    {"Secret", "Attack at dawn", 
      "\x45\xA0\x1F\x64\x5F\xC3\x5B\x38\x35\x52\x54\x4B\x9B\xF5"}
  };
  unsigned char out[32];

  r2.data = out;
  r2.len = 32;

  match = 1;
  for ( i = 0; i < 3; ++i ) {
    arc4_init(&arc4, str_to_raw(&r1, test_vectors[i][0], 0));
    arc4_encrypt(&arc4, str_to_raw(&r1, test_vectors[i][1], 0), &r2);
    printf("RC4|  Plaintext:'%s', Key:'%s', Ciphertext:'",
           test_vectors[i][0], test_vectors[i][1]);
    for ( j = 0; j < r1.len; ++j ) {
      printf("%02x", out[j]);
      match = match && (out[j] == test_vectors[i][2][j]);
    }
    printf("' (%s)\n", match ? "match" : "no match");
  }
}

int main(int argc, char *argv[])
{
  test_arc4();
  return 0;
}
