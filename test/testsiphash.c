/*
 * by Christopher Adam Telfer
 *
 * Copyright 2015 -- See accompanying license
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <cat/hash.h>
#include <cat/crypto.h>
#include <cat/stduse.h>

#define NUMSTR 4
#define NOPS	65536
#define NITER   (128 * NOPS)


void time_ht()
{
  struct htab t;
  struct hnode **buckets;
  uint nbuckets = 128 * 1024;
  const char mykey[256] = "Hello world! It's a beautiful day!";
  struct hnode nodes[NOPS], *node;
  struct ht_sh24_ctx hctx;
  int i, j;
  struct timeval start, end;
  double usec;
  uint h;

  buckets = emalloc(sizeof(struct hnode *) * nbuckets);
  ht_sh24_init(&hctx, mykey, strlen(mykey));
  ht_init(&t, buckets, nbuckets, &cmp_str, &ht_sh24_shash, &hctx);

  for (i = 0; i < NOPS; i++)
    ht_ninit(&nodes[i], str_fmt_a("node%d", i));

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
  time_ht();

  return 0;
}
