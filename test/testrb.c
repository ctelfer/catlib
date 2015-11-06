/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2015 -- See accompanying license
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cat/rbtree.h>
#include <cat/stduse.h>
#include <cat/emalloc.h>
#include <sys/time.h>

#define NUMSTR 4
#define NA	200


int vrfy(struct rbnode *n)
{
  int l, r;

  if ( ! n ) 
    return 0;

  l = vrfy(n->rb_left);
  r = vrfy(n->rb_right);

  if ( l < 0 || r < 0 ) 
    return -1;

  if ( r != l ) 
  {
    printf("Error!  Node %s has black heights of %d (left) and %d (right)\n",
	   (char *)n->key, l, r);
    return -1;
  }

  return (n->col == CRB_BLACK) ? l + 1 : l;
}



void printrb(struct rbnode *n, int d)
{
  int i;
  struct crbnode *crn = container(n, struct crbnode, node);

  if ( n->rb_right ) 
    printrb(n->rb_right, d+1);

  for ( i = 0 ; i < d ; ++i ) 
    printf("  ");

  printf("[%s:%s->%s]\n", (char *)n->key, (char *)crn->data,
	 n->col == CRB_BLACK ? "black" : "red");

  if ( n->rb_left ) 
    printrb(n->rb_left, d+1);

}




void printrbt(struct crbtree *t)
{
  if ( t->tree.rb_root == NULL ) 
    printf("tree is empty\n");
  else
    printrb(t->tree.rb_root, 0);
}




void ourfree(void *data, void *ctx)
{
  free(data);
}


#define NOPS 65536
#define NITER (NOPS * 128)
void timeit()
{
  struct rbnode nodes[NOPS], *finder;
  struct timeval start, end;
  int i, j, dir;
  struct rbtree t;
  double dbl;

  for (i = 0; i < NOPS; i++)
    rb_ninit(&nodes[i], str_fmt_a("node%d", i));
  rb_init(&t, cmp_str);

  gettimeofday(&start, NULL);
  for (j = 0; j < NITER / NOPS; ++j) { 
    for (i = 0; i < NOPS; i++) {
      finder = rb_lkup(&t, nodes[i].key, &dir);
      if (dir == CRB_N)
        err("Found duplicate key for %s", nodes[i].key);
      rb_ins(&t, &nodes[i], finder, dir);
    }
    for (i = 0; i < NOPS; i++)
      rb_rem(&nodes[i]);
  }
  gettimeofday(&end, NULL);

  dbl = end.tv_usec - start.tv_usec;
  dbl *= 1000.0;
  dbl += (end.tv_sec - start.tv_sec) * 1e9;

  printf("Roughly %f nanoseconds per lkup-ins-rem w/ %d entries max\n",
         dbl / (double)NITER, NOPS);

  gettimeofday(&start, NULL);
  for (i = 0; i < NITER; i++) {
    finder = rb_lkup(&t, nodes[0].key, &dir);
    if (dir == CRB_N)
      err("Found duplicate key for %s", nodes[0].key);
    rb_ins(&t, &nodes[0], finder, dir);
    rb_rem(&nodes[0]);
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
struct crbtree *t; 
char *s;
struct rbnode *np;

  t = crb_new(&crb_std_attr_skey);

  for (i = 0; i < NUMSTR; i++)
  {
    crb_put(t, strs[i][0], strs[i][1], NULL);
    s = crb_get(t, strs[i][0]);
    printf("Put (%s) at key (%s): %p\n", s, strs[i][0], s);
    fflush(stdout);
  }

  if (crb_get(t, "bye")) 
    printf("found something I shouldn't have!\n"); 
  fflush(stdout);

  s = crb_get(t, strs[1][0]);
  printf("Under key %s is the string %s\n", strs[1][0], s);
  printf("address is %p\n\n", s); 
  fflush(stdout);

  s = crb_get(t, strs[2][0]);
  printf("Under key %s is the string %s\n", strs[2][0], s);
  printf("address is %p\n\n", s); 
  fflush(stdout);

  s = crb_get(t, strs[0][0]);
  printf("Under key %s is the string %s\n", strs[0][0], s); 
  printf("address is %p\n\n", s); 
  fflush(stdout);

  printrbt(t);
  fflush(stdout);

  s = crb_del(t, strs[1][0]);
  printf("Deleted %s\n", s); 
  printf("address is %p\n\n", s); 
  fflush(stdout);

  s = crb_del(t, strs[2][0]);
  printf("Deleted %s\n", s); 
  printf("address is %p\n\n", s); 
  fflush(stdout);

  if (crb_get(t, strs[1][0]))
    printf("Error!  Thing not deleted! : %s\n", 
	   (char *)crb_get(t, strs[1][0]));
  fflush(stdout);

  printrbt(t);
  printf("\n");
  crb_free(t); 	/* get rid of "Overwrite!" */
  t = crb_new(&crb_std_attr_skey);

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
/* 
    printf("\n");
    printrbt(t);
    printf("\n");
    fflush(stdout);
*/
    sprintf(nstr, "k%03d", arr[i]);
    sprintf(vstr, "v%03d", arr[i]);
    crb_put(t, nstr, estrdup(vstr), NULL);
    if ( vrfy(t->tree.rb_root) < 0 )
    {
      printrbt(t);
      exit(-1);
    }
  }
  printf("\n\n");

  printrbt(t);

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
    printf("\n");
    printrbt(t);
    printf("\n\n");
    fflush(stdout);
*/

    sprintf(nstr, "k%03d", arr[i]);
    s = crb_del(t, nstr);
    free(s);
    if ( vrfy(t->tree.rb_root) < 0 )
    {
      printrbt(t);
      exit(-1);
    }
  }
  printf("\n\n");
  fflush(stdout);

  printrbt(t);
  fflush(stdout);

  crb_apply(t, ourfree, NULL);
  crb_free(t); 

  printf("Freed\n");

  timeit();

  printf("Ok!\n");

  return 0;
}
