/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <cat/regex.h>

void hurl() 
{ 
  printf("usage:  testreplace search pattern replace\n");
  exit(-1);
} 



int main(int argc, char *argv[])
{
char *new;

  if ( argc != 4 )
    hurl(); 

  new = re_sr(argv[1], argv[2], argv[3]);
  
  if ( ! new ) 
    printf("Nothing returned!\n");
  else
    printf("--\n%s\n--\n", new); 

  free(new);

  return 0 ; 
}
