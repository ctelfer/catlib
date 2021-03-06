/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <cat/dynmem.h>

#define NREPS 100000
int nreps = NREPS;

#define TEST(op) \
	{					\
	  struct timeval start, stop;		\
	  double usec, nspo;			\
	  int i;				\
						\
	  gettimeofday(&start, NULL);		\
	  for ( i = 0 ; i < nreps; ++i )	\
	    { op; }				\
	  gettimeofday(&stop, NULL);		\
	  usec = (stop.tv_sec - start.tv_sec) * 1000000 + \
		 (stop.tv_usec - start.tv_usec);\
	  nspo = 1000 * usec / nreps;		\
	  printf("%-40s\t%f\n", #op, nspo);	\
	  fflush(stdout);			\
	}

#define HEAD printf("%-40s\tnanoseconds\n", "Op"); \
	     printf("%-40s\t-----------\n", "--");

unsigned long Memory1[1024 * 1024 * 2];
unsigned long Memory2[1024 * 1024 * 2];
struct dynmem Dm;
struct tlsf Tlsf;

void test_malloc2(size_t siz)
{
  struct timeval start, stop;
  double usec, nspo;
  int i;
  void *objs[NREPS];
  char str[256];

  gettimeofday(&start, NULL);
  for ( i = 0 ; i < nreps; ++i )
    objs[i] = malloc(siz);
  for ( i = 0 ; i < nreps; ++i )
    free(objs[i]);
  gettimeofday(&stop, NULL);
  usec = (stop.tv_sec - start.tv_sec) * 1000000 +
	 (stop.tv_usec - start.tv_usec);
  nspo = 1000 * usec / nreps;
  snprintf(str, sizeof(str), "malloc(%u)/free(%u)", (unsigned)siz,
	   (unsigned)siz);
  printf("%-40s\t%f\n", str, nspo);
  fflush(stdout);
}

int main(int argc, char *argv[])
{
  dynmem_init(&Dm);
  dynmem_add_pool(&Dm, Memory1, sizeof(Memory1));
  tlsf_init(&Tlsf);
  tlsf_add_pool(&Tlsf, Memory2, sizeof(Memory2));

  printf("\n\n");
  HEAD
  TEST(free(malloc(16)))
  TEST(free(malloc(200)))
  TEST(free(malloc(550)))
  TEST(free(malloc(65536)))
  TEST(free(malloc(65586)))
  TEST(dynmem_free(&Dm, dynmem_malloc(&Dm, 16)))
  TEST(dynmem_free(&Dm, dynmem_malloc(&Dm, 200)))
  TEST(dynmem_free(&Dm, dynmem_malloc(&Dm, 550)))
  TEST(dynmem_free(&Dm, dynmem_malloc(&Dm, 65536)))
  TEST(dynmem_free(&Dm, dynmem_malloc(&Dm, 65586)))
  TEST(tlsf_free(&Tlsf, tlsf_malloc(&Tlsf, 16)))
  TEST(tlsf_free(&Tlsf, tlsf_malloc(&Tlsf, 200)))
  TEST(tlsf_free(&Tlsf, tlsf_malloc(&Tlsf, 550)))
  TEST(tlsf_free(&Tlsf, tlsf_malloc(&Tlsf, 65536)))
  TEST(tlsf_free(&Tlsf, tlsf_malloc(&Tlsf, 65586)))
  test_malloc2(32);
  test_malloc2(78);
  test_malloc2(200);
  test_malloc2(550);
  test_malloc2(65536);
  test_malloc2(65586);

  return 0;
}
