#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <cat/pack.h>
#include <cat/raw.h>


int main(int argc, char *argv[])
{
  unsigned char buffer[5000];
  struct raw r1, r2;
  char c = -1;
  unsigned char uc = 1;
  short s = -20;
  unsigned short us = 20;
  long l = -400000;
  unsigned long ul = 400000;
#if CAT_HAS_LONGLONG
  long long ll = -8000000000ll;
  unsigned long long ull = 8000000000ll;
#endif /* CAT_HAS_LONGLONG */
  char *string1 = "Hello world!", strbuf1[50], *string2 = "Hi again!",
       strbuf2[50];

#if CAT_HAS_LONGLONG
  char *bfmt = "rbHBwJWh%dbj";
#else /* CAT_HAS_LONG_LONG */
  char *bfmt = "rbHBwWh%db";
#endif /* CAT_HAS_LONGLONG */
  char fmt[50];
  int len, i;


  printf("Original data:\n");
  printf("Bytes are %d (%02x) and %u (%02x)\n", c, c & 0xFF, uc, uc);
  printf("Halves are %d (%04x) and %u (%04x)\n", s, s & 0xFFFF, us, us);
  printf("Words are %ld (%08lx) and %lu (%08lx)\n", l, l, ul, ul);
#if CAT_HAS_LONGLONG
  printf("Jumbos are %lld (%016llx) and %llu (%016llx)\n", ll, ll, ull, ull);
#endif /* CAT_HAS_LONGLONG */
  printf("String1 = %s\n", string1);
  printf("String2 = %s\n", string2);
  sprintf(fmt, bfmt, strlen(string2)+1);
  printf("Format string = %s\n", fmt);

  len = packlen(fmt, str_to_raw(&r1, string1, 1));

  printf("Packing will contain %d bytes (raw contains %d)\n", len, (int)r1.len);
  assert(len > 0);

#if CAT_HAS_LONGLONG
  len = pack(buffer, sizeof(buffer), fmt, &r1, uc, s, c, ul, ll, l, us, 
	     string2, ull);
#else /* CAT_HAS_LONGLONG */
  len = pack(buffer, sizeof(buffer), fmt, &r1, uc, s, c, ul, l, us, string2);
#endif /* CAT_HAS_LONGLONG */
  assert(len > 0);

  printf("Packed %d bytes\n", len);

  printf("bytes : ");
  for ( i = 0 ; i < len ; ++i ) 
    printf("%02x ", buffer[i]);
  printf("\n\n");

  printf("clearing values!\n");
  c = uc = 0; 
  s = us = 0; 
  l = ul = 0; 
#if CAT_HAS_LONGLONG
  ll = ull = 0;
#endif /* CAT_HAS_LONGLONG */
  strbuf2[0] = '\0';


#if CAT_HAS_LONGLONG
  len = unpack(buffer + r1.len, sizeof(buffer)-r1.len, fmt+1, &uc, &s, &c, &ul,
	       &ll, &l, &us, strbuf2, &ull);
#else /* CAT_HAS_LONGLONG */
  len = unpack(buffer + r1.len, sizeof(buffer)-r1.len, fmt+1, &uc, &s, &c, &ul,
	       &l, &us, strbuf2);
#endif /* CAT_HAS_LONGLONG */

  printf("Unpacked %d bytes: \n", len);
  assert(len > 0);
  printf("Bytes are %d (%02x) and %u (%02x)\n", c, c & 0xff, uc, uc & 0xff);
  printf("Halves are %d (%04x) and %u (%04x)\n",s, s & 0xffff, us, us & 0xffff);
  printf("Words are %ld (%08lx) and %lu (%08lx)\n", l, l, ul, ul);
#if CAT_HAS_LONGLONG
  printf("Jumbos are %lld (%016llx) and %llu (%016llx)\n", ll, ll, ull, ull);
#endif /* CAT_HAS_LONGLONG */
  printf("String1 = %s\n", buffer);
  printf("String2 = %s\n", strbuf2);

  printf("\n\n");
  printf("hton16(0x2211) = %x\n", hton16(0x2211));
  printf("ntoh16(0x1122) = %x\n", ntoh16(0x1122));
  printf("hton32(0x44332211) = %x\n", hton32(0x44332211));
  printf("ntoh32(0x11223344) = %x\n", ntoh32(0x11223344));
#if CAT_HAS_LONGLONG
  printf("ntoh64(0x1122334455667788) = %llx\n", 
         (ullong)ntoh64(0x1122334455667788ll));
  printf("hton64(0x8877665544332211) = %llx\n", 
	 (ullong)ntoh64(0x8877665544332211ll));
#endif /* CAT_HAS_LONGLONG */

  return 0;
}
