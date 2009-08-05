#include <cat/time.h>
#include <cat/stduse.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  struct cat_time start, last, cur;
    
  last = *tm_uget(&start);
  printf("sec = %ld, nsec = %ld\n", start.sec, start.nsec);
  sleep(1);
  tm_mark(&last, tm_uget(&cur));
  printf("sec delta = %ld, nsec delta = %ld, new time = %ld.%09ld\n",
	 cur.sec, cur.nsec, last.sec, last.nsec);
  printf("Step 1 took %f seconds\n", tm_2dbl(&cur));
  sleep(2);
  printf("Step 2 took %f seconds\n", tm_2dbl(tm_mark(&last, tm_uget(&cur))));
  sleep(3);
  printf("Last was %f seconds\n", tm_2dbl(tm_mark(&last, tm_uget(&cur))));
  printf("Total time was %f seconds\n", tm_2dbl(tm_sub(&last, &start)));

  tm_dset(&last, tm_2dbl(&last) * 3.0);
  printf("Multiplied by 3 : %f\n", tm_2dbl(&last));
  tm_dset(&last, tm_2dbl(&last) / 3.0);
  printf("Divided by 3 : %f\n", tm_2dbl(&last));

  tm_dset(&last, tm_2dbl(&last) / 0.3);
  printf("Divided by .3 : %f\n", tm_2dbl(&last));
  tm_dset(&last, tm_2dbl(&last) * 0.3);
  printf("Multiplied by .3 : %f\n", tm_2dbl(&last));

  tm_dset(&last, tm_2dbl(&last) / 7.5);
  printf("Divided by 7.5 : %f\n", tm_2dbl(&last));
  tm_dset(&last, tm_2dbl(&last) * 7.5);
  printf("Multiplied by 7.5 : %f\n", tm_2dbl(&last));

  return 0;
}
