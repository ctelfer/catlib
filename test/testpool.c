/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cat/aux.h>
#include <cat/mem.h>
#include <cat/pool.h>
#include <cat/stduse.h>

#define NCONS	100000

struct pool Pool;


int main(int argc, char *argv[])
{
  int i, j, na, psiz;
  void **allocs;
  struct timeval tv, tv2;
  double usec;


  if ( argc < 4 )
    err("usage: %s <alloc size> <pool size> <number of allocs>\n", argv[0]);

  psiz = atoi(argv[2]);
  na = atoi(argv[3]);

  pl_init(&Pool, atoi(argv[1]), -1, emalloc(psiz), psiz);


  if ( na > Pool.max )
    err("Allocating more items than pool size (%d > %d)\n", na, Pool.max);

  allocs = emalloc(sizeof(void *) * na);

  printf("number of rounds: %d\n", NCONS);
  printf("number of allocations/deallocations per round: %d\n", na);
  fflush(stdout);

  gettimeofday(&tv, 0);
  for ( i = 0 ; i < NCONS; ++i )
  {
    for ( j = 0 ; j < na ; ++j ) 
      allocs[j] = pl_alloc(&Pool);
    for ( j = 0 ; j < na ; ++j ) 
      pl_free(&Pool, allocs[j]);
  }
  gettimeofday(&tv2, 0);
  usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + tv2.tv_usec - tv.tv_usec;
  usec /= NCONS * na;

  printf("Roughly %f nanoseconds for the two operations\n", usec * 1000);
  fflush(stdout);

  return 0;
}
