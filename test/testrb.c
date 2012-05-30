/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cat/rbtree.h>
#include <cat/stduse.h>
#include <sys/time.h>

#define NUMSTR 4
#define NA	200


char *sdup(const char *s)
{
	size_t ls = strlen(s);
	char *ns = malloc(ls + 1);
	if (ns == NULL)
		return NULL;
	memcpy(ns, s, ls);
	ns[ls] = '\0';
	return ns;
}


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

  if ( n->rb_right ) 
    printrb(n->rb_right, d+1);

  for ( i = 0 ; i < d ; ++i ) 
    printf("  ");

  printf("[%s:%s->%s]\n", (char *)n->key, (char *)n->data, n->col == CRB_BLACK ? 
	 "black" : "red");

  if ( n->rb_left ) 
    printrb(n->rb_left, d+1);

}




void printrbt(struct rbtree *t)
{
  if ( t->rb_root == NULL ) 
    printf("tree is empty\n");
  else
    printrb(t->rb_root, 0);
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
    rb_ninit(&nodes[i], str_fmt_a("node%d", i), NULL);
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
struct rbtree *t; 
char *s;
struct rbnode *np;

  t = rb_new(&estdmm, CAT_KT_STR, 0, 0);

  for (i = 0; i < NUMSTR; i++)
  {
    rb_put(t, strs[i][0], strs[i][1]);
    np = rb_lkup(t, strs[i][0], 0);
    printf("Put (%s) at key (%s): %x\n", strs[i][1], strs[i][0],
           ptr2uint(np));
    fflush(stdout);
  }

  if (rb_get_dptr(t, "bye")) 
    printf("found something I shouldn't have!\n"); 
  fflush(stdout);

  s = rb_get_dptr(t, strs[1][0]);
  printf("Under key %s is the string %s\n", strs[1][0], s);
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  s = rb_get_dptr(t, strs[2][0]);
  printf("Under key %s is the string %s\n", strs[2][0], s);
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  s = rb_get_dptr(t, strs[0][0]);
  printf("Under key %s is the string %s\n", strs[0][0], s); 
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  printrbt(t);
  fflush(stdout);

  s = rb_get_dptr(t, strs[1][0]);
  rb_clr(t, strs[1][0]);
  printf("Deleted %s\n", s); 
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  np = rb_lkup(t, strs[2][0], NULL);
  s = np->data;
  rb_rem(np);
  free(np);
  printf("Deleted %s\n", s); 
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  if (rb_get_dptr(t, strs[1][0]))
    printf("Error!  Thing not deleted! : %s\n", (char *)rb_get_dptr(t, strs[1][0]));
  fflush(stdout);

  printrbt(t);
  printf("\n");
  rb_free(t); 	/* get rid of "Overwrite!" */
  t = rb_new(&estdmm, CAT_KT_STR, 0, 0);

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
    rb_put(t, nstr, sdup(vstr));
    if ( vrfy(t->rb_root) < 0 )
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
    s = rb_get_dptr(t, nstr);
    rb_clr(t, nstr);
    free(s);
    if ( vrfy(t->rb_root) < 0 )
    {
      printrbt(t);
      exit(-1);
    }
  }
  printf("\n\n");
  fflush(stdout);

  printrbt(t);
  fflush(stdout);

  rb_apply(t, ourfree, NULL);
  rb_free(t); 

  printf("Freed\n");

  timeit();

  printf("Ok!\n");

  return 0;
}
