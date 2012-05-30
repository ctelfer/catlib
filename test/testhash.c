/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <cat/hash.h>
#include <cat/stduse.h>

#define NUMSTR 4
#define NOPS	65536
#define NITER   (128 * NOPS)

void timeit()
{
  struct htab *t; 
  struct hnode nodes[NOPS], *node;
  uint hashes[NOPS];
  int i, j;
  char *key;
  struct timeval start, end;
  double usec;
  uint h;

  t = ht_new(&estdmm, 128 * 1024, CAT_KT_STR, 0, 0);

  for (i = 0; i < NOPS; i++) {
    key = str_fmt_a("node%d", i);
    ht_ninit(&nodes[i], key, NULL);
    hashes[i] = ht_hash(t, key);
  }

  gettimeofday(&start, NULL);
  for (j = 0; j < NITER / NOPS; j++) {
    for (i = 0; i < NOPS; i++)
      if ( (node = ht_ins(t, &nodes[i], hashes[i])) != NULL )
        err("iteration %d: duplicate node for key %d found\n", j, i);
    for (i = 0; i < NOPS; i++)
      ht_rem(&nodes[i]);
  }
  gettimeofday(&end, NULL);
  usec = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
  usec /= NITER;
  printf("Roughly %f nsec for ht_ins(),ht_rem() w/%d elem\n",
	  usec * 1000, NOPS);
  fflush(stdout);


  gettimeofday(&start, NULL);
  for (j = 0; j < NITER / NOPS; j++) { 
    for (i = 0; i < NOPS; i++) { 
      if (ht_lkup(t, nodes[i].key, &h))
        err("iteration %d: duplicate node for key %d found\n", j, i);
      ht_ins(t, &nodes[i], h); 
    }
    for (i = 0; i < NOPS; i++)
      ht_rem(&nodes[i]);
  }
  gettimeofday(&end, NULL);
  usec = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
  usec /= NITER;
  printf("Roughly %f nsec for ht_lkup(),ht_ins(),ht_rem() w/%d elem\n",
	  usec * 1000, NOPS);
  fflush(stdout);


  gettimeofday(&start, NULL);
  for ( i = 0 ; i < NITER ; ++i ) {
    ht_ins(t, &nodes[0], 0); 
    ht_rem(&nodes[0]);
  }
  gettimeofday(&end, NULL);
  usec = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
  usec /= NITER;
  printf("Roughly %f nsec for ht_ins(),ht_rem() with empty table\n",
	  usec * 1000);
  fflush(stdout);


  gettimeofday(&start, NULL);
  for ( i = 0 ; i < NITER ; ++i ) { 
    if (ht_lkup(t, nodes[0].key, &h))
      err("duplicate node for key %d found", i);
    ht_ins(t, &nodes[0], h); 
    ht_rem(&nodes[0]);
  }
  gettimeofday(&end, NULL);
  usec = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
  usec /= NITER;
  printf("Roughly %f nsec for ht_ins(),ht_lkup(),ht_rem() with empty table\n",
	  usec * 1000);
  fflush(stdout);


  for (i = 0; i < NOPS; i++)
    free(nodes[i].key);

  ht_free(t);
}


int main() 
{ 
  int i;
  char *strs[NUMSTR][2] = 
                  { {"key 1", "Hi there!" },
                    {"key2", "String    2!"},
                    {"Key 3!", "By now! this is over."},
                    {"key 1", "Overwrite."}
                  };
  struct htab *table; 
  char *s;
  struct hnode n, *hnp;
  struct raw r;

  table = ht_new(&estdmm, 128, CAT_KT_STR, 0, 0);

  for (i = 0; i < NUMSTR; i++) {
    ht_put(table, strs[i][0], strs[i][1]);
    hnp = ht_lkup(table, strs[i][0], 0);
    printf("Put (%s) at key (%s): %x\n", strs[i][1], strs[i][0],
           ptr2uint(hnp));
  }

  if (ht_get_dptr(table, "bye")) 
    printf("found something I shouldn't have!\n"); 

  s = ht_get_dptr(table, strs[1][0]);
  printf("Under key %s is the string %s\n", strs[1][0], s);
  printf("address is %x\n\n", ptr2uint(s)); 

  s = ht_get_dptr(table, strs[2][0]);
  printf("Under key %s is the string %s\n", strs[2][0], s);
  printf("address is %x\n\n", ptr2uint(s)); 

  s = ht_get_dptr(table, strs[0][0]);
  printf("Under key %s is the string %s\n", strs[0][0], s); 
  printf("address is %x\n\n", ptr2uint(s)); 

  s = ht_get_dptr(table, strs[1][0]);
  ht_clr(table, strs[1][0]);
  printf("Deleted %s\n", s); 
  printf("address is %x\n\n", ptr2uint(s)); 


  hnp = ht_lkup(table, strs[2][0], NULL);
  s = hnp->data;
  ht_rem(hnp);
  free(hnp);
  printf("Deleted %s\n", s); 
  printf("address is %x\n\n", ptr2uint(s)); 

  if (ht_get_dptr(table, strs[1][0]))
    printf("Error!  Thing not deleted! : %s\n",
           (char *)ht_get_dptr(table, strs[1][0]));

  ht_free(table); 

  timeit();

  return 0;
} 
