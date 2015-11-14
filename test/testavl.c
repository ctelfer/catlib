/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2015 -- See accompanying license
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cat/avl.h>
#include <cat/stduse.h>
#include <cat/emalloc.h>
#include <sys/time.h>

#define NUMSTR	4
#define NA	200

/* in case strdup() doesn't exist */
int vrfy(struct anode *an)
{
  int l, r;

  if ( ! an ) 
    return 0;

  l = vrfy(an->avl_left);
  r = vrfy(an->avl_right);

  if ( l < 0 || r < 0 ) 
    return -1;

  if ( r-l != an->b ) 
  {
    printf("Error!  Node %s has balance of %d, but left = %d and right = %d\n",
	   (char *)an->key, an->b, l, r);
    return -1;
  }

  return (l < r) ? r + 1 : l + 1;
}



void printan(struct anode *an, int d)
{
  int i;
  struct canode *can = container(an, struct canode, node);

  if ( an->avl_right ) 
    printan(an->avl_right, d+1);

  for ( i = 0 ; i < d ; ++i ) 
    printf("  ");

  printf("[%s:%s->%d]\n", (char *)an->key, (char *)can->data, an->b);

  if ( an->avl_left ) 
    printan(an->avl_left, d+1);

}




void printavl(struct cavltree *t)
{
  if ( t->tree.avl_root == NULL ) 
    printf("tree is empty\n");
  else
    printan(t->tree.avl_root, 0);
}




void ourfree(void *data, void *ctx)
{
  free(data);
}




#define NOPS 65536
#define NITER (NOPS * 128)
void timeit()
{
  struct anode nodes[NOPS], *finder;
  struct timeval start, end;
  int i, j, dir;
  struct avltree t;
  double dbl;

  for (i = 0; i < NOPS; i++)
    avl_ninit(&nodes[i], str_fmt_a("node%d", i));
  avl_init(&t, cmp_str);

  gettimeofday(&start, NULL);
  for (j = 0; j < NITER / NOPS; j++) { 
    for (i = 0; i < NOPS; i++) {
      finder = avl_lkup(&t, nodes[i].key, &dir);
      if (dir == CA_N)
        err("Found duplicate key for %s", nodes[i].key);
      avl_ins(&t, &nodes[i], finder, dir);
    }
    for (i = 0; i < NOPS; i++)
      avl_rem(&nodes[i]);
  }
  gettimeofday(&end, NULL);

  dbl = end.tv_usec - start.tv_usec;
  dbl *= 1000.0;
  dbl += (end.tv_sec - start.tv_sec) * 1e9;

  printf("Roughly %f nanoseconds per lkup-ins-rem w/ %d entries max\n",
         dbl / (double)NITER, NOPS);

  gettimeofday(&start, NULL);
  for (i = 0; i < NITER; i++) {
    finder = avl_lkup(&t, nodes[0].key, &dir);
    if (dir == CA_N)
      err("Found duplicate key for %s", nodes[0].key);
    avl_ins(&t, &nodes[0], finder, dir);
    avl_rem(&nodes[0]);
  }
  gettimeofday(&end, NULL);

  dbl = end.tv_usec - start.tv_usec;
  dbl *= 1000.0;
  dbl += (end.tv_sec - start.tv_sec) * 1e9;

  printf("Roughly %f nanoseconds per lkup-ins-rem (1 item deep max)\n",
         dbl / (double)NITER);

  for (i = 0; i < NOPS; i++)
    free(nodes[i].key);
}



int main() 
{ 
int i, idx, tmp;
int arr[NA];
char nstr[80], vstr[80];
void free(void *);
char *strs[NUMSTR][2] = 
                  { {"key 1", "Hi there!" },
                    {"key2", "String    2!"},
                    {"Key 3!", "By now! this is over."},
                    {"key 1", "Overwrite."}
                  };
struct cavltree *avl; 
char *s;

  avl = cavl_new(&cavl_std_attr_skey, 1);

  for (i = 0; i < NUMSTR; i++)
  {
    cavl_put(avl, strs[i][0], strs[i][1]);
    s = cavl_get(avl, strs[i][0]);
    printf("Put (%s) at key (%s): %p\n", s, strs[i][0], s);
    fflush(stdout);
  }

  if (cavl_get(avl, "bye")) 
    printf("found something I shouldn't have!\n"); 
  fflush(stdout);

  s = cavl_get(avl, strs[1][0]);
  printf("Under key %s is the string %s\n", strs[1][0], s);
  printf("address is %p\n\n", s); 
  fflush(stdout);

  s = cavl_get(avl, strs[2][0]);
  printf("Under key %s is the string %s\n", strs[2][0], s);
  printf("address is %p\n\n", s); 
  fflush(stdout);

  s = cavl_get(avl, strs[0][0]);
  printf("Under key %s is the string %s\n", strs[0][0], s); 
  printf("address is %p\n\n", s); 
  fflush(stdout);

  printavl(avl);
  fflush(stdout);

  s = cavl_get(avl, strs[1][0]);
  cavl_del(avl, strs[1][0]);
  printf("Deleted %s\n", s); 
  printf("address is %p\n\n", s); 
  fflush(stdout);

  s = cavl_del(avl, strs[2][0]);
  printf("Deleted %s\n", s); 
  printf("address is %p\n\n", s); 
  fflush(stdout);

  if (cavl_get(avl, strs[1][0]))
    printf("Error!  Thing not deleted! : %s\n", 
            (char*)cavl_get(avl, strs[1][0]));
  fflush(stdout);

  printavl(avl);
  printf("\n");
  cavl_free(avl); 	/* get rid of "Overwrite!" */

  avl = cavl_new(&cavl_std_attr_skey, 1);

  for ( i = 0 ; i < NA ; ++i ) 
    arr[i] = i;

  for ( i = 0 ; i < (NA-1) ; ++i ) 
  {
    idx      = abs(rand()) % (NA - i) + i;
    tmp      = arr[idx];
    arr[idx] = arr[i];
    arr[i]   = tmp;
  }

  for ( i = 0 ; i < NA ; ++i ) 
  {
    printf("%d ", arr[i]);
    sprintf(nstr, "k%03d", arr[i]);
    sprintf(vstr, "v%03d", arr[i]);
    cavl_put(avl, nstr, estrdup(vstr));
  }
  printf("\n\n");

  printavl(avl);

  printf("\n\n");
  fflush(stdout);

  for ( i = 0 ; i < (NA-1) ; ++i ) 
  {
    idx      = abs(rand()) % (NA - i) + i;
    tmp      = arr[idx];
    arr[idx] = arr[i];
    arr[i]   = tmp;
  }

  for ( i = 0 ; i < NA ; ++i ) 
  {
    printf("%d ", arr[i]);
/* 
    printf("%d\n", arr[i]);
    printavl(avl);
    printf("\n\n");
*/

    sprintf(nstr, "k%03d", arr[i]);
    s = cavl_del(avl, nstr);
    free(s);
    if ( vrfy(avl->tree.avl_root) < 0 )
    {
      printavl(avl);
      exit(-1);
    }
  }
  printf("\n\n");
  fflush(stdout);

  printavl(avl);
  fflush(stdout);

  cavl_apply(avl, ourfree, NULL);
  cavl_free(avl); 

  printf("Freed\n");

  timeit();

  printf("Ok!\n");

  return 0;
}
