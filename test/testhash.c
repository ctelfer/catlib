/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2015 -- See accompanying license
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


struct htab t; 
struct hnode *buckets[128 * 1024];

void timeit()
{
  struct hnode nodes[NOPS], *node;
  uint hashes[NOPS];
  int i, j;
  char *key;
  struct timeval start, end;
  double usec;
  uint h;

  ht_init(&t, buckets, array_length(buckets), cmp_str, ht_shash, NULL);

  for (i = 0; i < NOPS; i++) {
    key = str_fmt_a("node%d", i);
    ht_ninit(&nodes[i], key);
    hashes[i] = ht_hash(&t, key);
  }

  gettimeofday(&start, NULL);
  for (j = 0; j < NITER / NOPS; j++) {
    for (i = 0; i < NOPS; i++)
      ht_ins(&t, &nodes[i], hashes[i]);
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
      if (ht_lkup(&t, nodes[i].key, &h))
        err("iteration %d: duplicate node for key %d found\n", j, i);
      ht_ins(&t, &nodes[i], h); 
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
    ht_ins(&t, &nodes[0], hashes[0]);
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
    if (ht_lkup(&t, nodes[0].key, &h))
      err("duplicate node for key %d found", i);
    ht_ins(&t, &nodes[0], h); 
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
  struct chtab *table; 
  char *s;

  table = cht_new(128, NULL, NULL);

  for (i = 0; i < NUMSTR; i++) {
    cht_put(table, strs[i][0], strs[i][1]);
    s = cht_get(table, strs[i][0]);
    printf("Put (%s) at key (%s): %p\n", strs[i][1], strs[i][0], s);
  }

  if (cht_get(table, "bye")) 
    printf("found something I shouldn't have!\n"); 

  s = cht_get(table, strs[1][0]);
  printf("Under key %s is the string %s\n", strs[1][0], s);
  printf("address is %p\n\n", s); 

  s = cht_get(table, strs[2][0]);
  printf("Under key %s is the string %s\n", strs[2][0], s);
  printf("address is %p\n\n", s); 

  s = cht_get(table, strs[0][0]);
  printf("Under key %s is the string %s\n", strs[0][0], s); 
  printf("address is %p\n\n", s); 

  s = cht_get(table, strs[1][0]);
  cht_del(table, strs[1][0]);
  printf("Deleted %s\n", s); 
  printf("address is %p\n\n", s); 


  s = cht_del(table, strs[2][0]);
  printf("Deleted %s\n", s); 
  printf("address is %p\n\n", s); 

  if (cht_get(table, strs[1][0]))
    printf("Error!  Thing not deleted! : %s\n",
           (char *)cht_get(table, strs[1][0]));

  cht_free(table); 

  timeit();

  return 0;
} 
