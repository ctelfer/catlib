/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cat/list.h>
#include <cat/stduse.h>
#include <cat/mem.h>


#define NT 100000000
#define LLEN 100
#define NT2 1000000


int main(int argc, char *argv[])
{
  struct list list, elem, *node;
  struct clist *cl;
  struct timeval tv, tv2;
  double usec;
  int i, x;

  l_init(&list);
  l_init(&elem);

  gettimeofday(&tv, 0);
  for ( i = 0 ; i < NT ; ++i ) {
    l_ins(&list, &elem);
    l_rem(&elem);
  }
  gettimeofday(&tv2, 0);
  usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + 
         tv2.tv_usec - tv.tv_usec;
  usec /= NT;

  printf("Roughly %f nanoseconds for l_ins(),l_rem()\n", usec * 1000);


  printf("loading %u elements\n", LLEN);
  for ( i = 0 ; i < LLEN ; ++i ) 
    l_ins(&list, emalloc(sizeof(struct list)));
  printf("The l_length(&list) is %lu\n", l_length(&list));

  gettimeofday(&tv, 0);
  for ( i = 0 ; i < NT2 ; ++i )
    for ( node = list.next ; node != &list ; node = node->next ) 
      ;
  gettimeofday(&tv2, 0);
  usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + 
         tv2.tv_usec - tv.tv_usec;
  usec /= NT2;
  
  printf("Roughly %f nanoseconds for %d element traversal\n", 
	  usec * 1000, LLEN);

  while ( l_isempty(&list) )
    free(l_deq(&list));

  cl = cl_new(NULL);
  gettimeofday(&tv, 0);
  for ( i = 0 ; i < NT ; ++i ) {
    cl_enq(cl, int2ptr(i));
    x = ptr2int(cl_deq(cl));
  }
  gettimeofday(&tv2, 0);
  usec = (tv2.tv_sec - tv.tv_sec) * 1000000 + 
         tv2.tv_usec - tv.tv_usec;
  usec /= NT;
  cl_free(cl);

  printf("Roughly %f nanoseconds for cl_enq(),cl_deq()\n", 
         usec * 1000);

  return 0;
}
