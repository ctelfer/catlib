#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cat/aux.h>
#include <cat/err.h>
#include <cat/pcache.h>
#include <cat/stduse.h>

#define NCONS	10000

struct pcache Pcache;


int main(int argc, char *argv[])
{
  int i, j, na, psiz;
  void **allocs;
  struct timeval tv, tv2;
  double usec;
  uint hiwat = 0;


  if ( argc < 4 )
    err("usage: %s <alloc size> <pool size> <number of allocs> [<hwm>]\n",
	argv[0]);

  psiz = atoi(argv[2]);
  na = atoi(argv[3]);

  if ( argc > 4 )
    hiwat = atoi(argv[4]);

  pc_init(&Pcache, atoi(argv[1]), psiz, hiwat, 0, &estdmm);

  /* add one page */
  pc_addpg(&Pcache, emalloc(psiz), psiz);

  allocs = emalloc(sizeof(void *) * na);

  printf("number of rounds: %d\n", NCONS);
  printf("number of allocations/deallocations per round: %d\n", na);
  fflush(stdout);

  gettimeofday(&tv, 0);
  for ( i = 0 ; i < NCONS; ++i )
  {
    for ( j = 0 ; j < na ; ++j ) 
      allocs[j] = pc_alloc(&Pcache);
    for ( j = 0 ; j < na ; ++j ) 
      pc_free(allocs[j]);
  }
  gettimeofday(&tv2, 0);
  usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
  usec /= NCONS * na;

  printf("Roughly %f nanoseconds for the two operations\n", usec * 1000);
  fflush(stdout);

  pc_freeall(&Pcache);

  return 0;
}
