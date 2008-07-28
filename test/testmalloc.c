#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cat/err.h>

#define NCONS	100000

int main(int argc, char *argv[])
{
  int i, j, na, siz;
  void **allocs;
  struct timeval tv, tv2;
  double usec;


  if ( argc < 4 )
    err("usage: %s <alloc size> <pool size> <number of allocs>\n", argv[0]);

  siz = atoi(argv[1]);
  na = atoi(argv[3]);

  allocs = malloc(sizeof(void *) * na);

  printf("number of rounds: %d\n", NCONS);
  printf("number of allocations/deallocations per round: %d\n", na);
  fflush(stdout);

  gettimeofday(&tv, 0);
  for ( i = 0 ; i < NCONS; ++i )
  {
    for ( j = 0 ; j < na ; ++j ) 
      allocs[j] = malloc(siz);
    for ( j = 0 ; j < na ; ++j ) 
      free(allocs[j]);
  }
  gettimeofday(&tv2, 0);
  usec = (tv2.tv_sec - tv.tv_sec) * 1000000 +
         tv2.tv_usec - tv.tv_usec;
  usec /= (NCONS * na) ;

  printf("Roughly %lf microseconds for the two operations\n", usec);
  fflush(stdout);

  return 0;
}
