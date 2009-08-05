#include <stdio.h>
#include <sys/time.h>
#include <cat/list.h>
#include <cat/stduse.h>


#define NT 100000000
#define LLEN 100
#define NT2 1000000


int main(int argc, char *argv[])
{
  struct list list, elem, *node;
  struct timeval tv, tv2;
  double usec;
  int i;

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


  for ( i = 0 ; i < LLEN ; ++i ) 
    l_ins(&list, clist_new(void *));

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

  return 0;
}
