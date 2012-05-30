/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <cat/aux.h>
#include <cat/heap.h>
#include <cat/err.h>
#include <cat/stduse.h>


int my_cmp_int(const void *a, const void *b)
{
  return ptr2int(a) - ptr2int(b);
}


int main(int argc, char *argv[])
{
  int num, i, hold, si, *arr, pos;
  struct heap *hp;

  if ( argc < 2 ) 
    err("need a size of the array\n");

  num = atoi(argv[1]);
  arr = emalloc(sizeof(int) * num);


  /* generate the array */
  for ( i = 0 ; i < num ; ++i ) 
    arr[i] = i+1;

  for ( i = 0 ; i < num - 1 ; ++i ) 
  {
    si = rand();
    if ( si < 0 )
      si = -si;
    si = (si % (num-i-1))+i+1;

    hold = arr[si];
    arr[si] = arr[i];
    arr[i] = hold;
  }

  printf("initial array:  ");
  for ( i = 0 ; i < num ; ++i ) 
    printf("%d ", arr[i]);
  printf("\n");

  /* generate the heap */
  hp = hp_new(&estdmm, 0, &my_cmp_int);

  for ( i = 0 ; i < num ; ++i ) {
    if (hp_add(hp, int2ptr(arr[i]), NULL) < 0) {
      fprintf(stderr, "out of memory\n");
      exit(-1);
    }
  }


  printf("current heap: ");
  for ( i = 0 ; i < num ; ++i ) 
    printf("%d ", ptr2int(hp->elem[i]));
  printf("\n");



  /* do some border test cases for find */
  if ( (pos = hp_find(hp, int2ptr(1))) < 0 )
    err("Couldn't find 1\n");
  else
    printf("1 was at %u\n", pos);

  if ( (pos = hp_find(hp, int2ptr(num / 2))) < 0 )
    err("Couldn't find %d\n", num / 2);
  else
    printf("%d was at %u\n", num / 2, pos);

  if ( (pos = hp_find(hp, int2ptr(num+1))) >= 0 )
    err("Couldn't found %d at %u when I shouldn't have\n", num+1, pos);

  
  /* now print out all the numbers */
  while ( (i = ptr2int(hp_rem(hp, 0))) )
    printf("%d ", i);
  printf("\n");


  /* free the heap */
  hp_free(hp);


  return 0;
}
