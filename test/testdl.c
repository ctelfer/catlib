#include <cat/dlist.h>
#include <cat/stduse.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
  struct dlist *node, list, *trav;
  struct list list2;
  struct cat_time ct;

  int arr1[] = { 10, 15, 5, 7, 12, 2, 7 },
      arr2[] = { 9, 1, 6, 10, 6 }, 
      s1 = sizeof(arr1) / sizeof(arr1[0]),
      s2 = sizeof(arr2) / sizeof(arr1[0]), i, j = 0;

  printf("Initial: ");
  for ( i = 0 ; i < s1 ; ++i )
    printf("%u/%u ", ++j, arr1[i]);
  printf("  |  ");
  for ( i = 0 ; i < s2 ; ++i )
    printf("%u/%u ", ++j, arr2[i]);
  printf("\n");

  j=0;
  dl_init(&list, -1, -1);
  for ( i = 0 ; i < s1 ; ++i )
  {
    ++j;
    node = cdl_new(arr1[i], 0, (void *)j);
    dl_ins(&list, node);
  }

  dl_first(&list, &ct);
  printf("The first is at %u\n", (uint)ct.sec);
  node = dl_deq(&list);
  printf("The first was %u at %u\n\n", (uint)cdl_data(node), 
         (uint)node->ttl.sec);
  cdl_free(node);

  printf("Nodes from advance 10:  ");
  l_init(&list2);
  tm_lset(&ct, 10, 0);
  dl_adv(&list, &ct, &list2);

  while ( ! l_isempty(&list2) )
  {
    trav = container(l_head(&list2), struct dlist, entry);
    printf("%u/%u ", (uint)cdl_data(trav), (uint)trav->ttl.sec);
    l_rem(&trav->entry);
    cdl_free(trav);
  }
  printf("\n\n");

  for ( i = 0 ; i < s2 ; ++i ) 
  {
    ++j;
    node = cdl_new(arr2[i], 0, (void *)j);
    dl_ins(&list, node);
  }

  printf("After inserting arr2 array is :\n\t");
  while ( node = dl_deq(&list) )
  {
    printf("%u/", (uint)cdl_data(node));
    printf("%u ", (uint)node->ttl.sec);
    cdl_free(node);
  }
  printf("\n");

  return 0;
}
