#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cat/splay.h>
#include <cat/stduse.h>
#include <sys/time.h>

#define NUMSTR	4
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

void printst(struct stnode *n, int d)
{
  int i;

  if ( n->st_right ) 
    printst(n->st_right, d+1);

  for ( i = 0 ; i < d ; ++i ) 
    printf("  ");

  printf("[%s:%s]\n", (char *)n->key, (char *)n->data);

  if ( n->st_left ) 
    printst(n->st_left, d+1);

}




void print_splay(struct splay *t)
{
  struct stnode *top = t->root.p[CST_P];
  if ( top == NULL ) 
    printf("tree is empty\n");
  else
    printst(top, 0);
}




void ourfree(void *data, void *ctx)
{
  free(data);
}




#define NOPS 65536
#define NITER (NOPS * 128)
void timeit()
{
  struct stnode nodes[NOPS], *finder;
  struct timeval start, end;
  int i, j, dir;
  struct splay t;
  double dbl;

  for (i = 0; i < NOPS; i++)
    st_ninit(&nodes[i], str_fmt_a("node%d", i), NULL);
  st_init(&t, cmp_str);

  gettimeofday(&start, NULL);
  for (j = 0; j < NITER / NOPS; j++) { 
    for (i = 0; i < NOPS; i++) {
      finder = st_lkup(&t, nodes[i].key);
      if (finder)
        err("Found duplicate key for %s", nodes[i].key);
      st_ins(&t, &nodes[i]);
    }
    for (i = 0; i < NOPS; i++)
      st_rem(&nodes[i]);
  }
  gettimeofday(&end, NULL);

  dbl = end.tv_usec - start.tv_usec;
  dbl *= 1000.0;
  dbl += (end.tv_sec - start.tv_sec) * 1e9;

  printf("Roughly %f nanoseconds per lkup-ins-rem w/ %d entries max\n",
         dbl / (double)NITER, NOPS);

  gettimeofday(&start, NULL);
  for (i = 0; i < NITER; i++) {
    finder = st_lkup(&t, nodes[0].key);
    if (finder)
      err("Found duplicate key for %s", nodes[0].key);
    st_ins(&t, &nodes[0]);
    st_rem(&nodes[0]);
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
struct splay *t; 
char *s;
struct stnode *np;

  t = st_new(&estdmm, CAT_KT_STR, 0, 0);

  for (i = 0; i < NUMSTR; i++)
  {
    st_put(t, strs[i][0], strs[i][1]);
    np = st_lkup(t, strs[i][0]);
    printf("Put (%s) at key (%s): %x\n", strs[i][1], strs[i][0],
           ptr2uint(np->data));
    fflush(stdout);
  }

  if (st_get_dptr(t, "bye")) 
    printf("found something I shouldn't have!\n"); 
  fflush(stdout);

  s = st_get_dptr(t, strs[1][0]);
  printf("Under key %s is the string %s\n", strs[1][0], s);
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  s = st_get_dptr(t, strs[2][0]);
  printf("Under key %s is the string %s\n", strs[2][0], s);
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  s = st_get_dptr(t, strs[0][0]);
  printf("Under key %s is the string %s\n", strs[0][0], s); 
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  print_splay(t);
  fflush(stdout);

  s = st_get_dptr(t, strs[1][0]);
  st_clr(t, strs[1][0]);
  printf("Deleted %s\n", s); 
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  np = st_lkup(t, strs[2][0]);
  s = np->data;
  st_rem(np);
  free(np);
  printf("Deleted %s\n", s); 
  printf("address is %x\n\n", ptr2uint(s)); 
  fflush(stdout);

  if (st_get_dptr(t, strs[1][0]))
    printf("Error!  Thing not deleted! : %s\n", 
           (char *)st_get_dptr(t, strs[1][0]));
  fflush(stdout);

  print_splay(t);
  printf("\n");
  st_free(t); 	/* get rid of "Overwrite!" */
  t = st_new(&estdmm, CAT_KT_STR, 0, 0);

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
    print_splay(t);
    printf("\n");
    fflush(stdout);
*/
    sprintf(nstr, "k%03d", arr[i]);
    sprintf(vstr, "v%03d", arr[i]);
    st_put(t, nstr, sdup(vstr));
  }
  printf("\n\n");

  print_splay(t);

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
    s = st_get_dptr(t, nstr);
    st_clr(t, nstr);
    free(s);
  }
  printf("\n\n");
  fflush(stdout);

  print_splay(t);
  fflush(stdout);

  st_apply(t, ourfree, NULL);
  st_free(t); 

  printf("Freed\n");

  timeit();

  printf("Ok!\n");

  return 0;
}
