/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/time.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  cat_time_t start, last, delta;
    
  start = last = tm_uget();
  printf("sec = %ld, nsec = %ld\n", tm_sec(start), tm_nsec(start));

  sleep(1);
  delta = tm_mark(&last, tm_uget());
  printf("sec delta = %ld, nsec delta = %ld, new time = %ld.%09ld\n",
	 delta.sec, delta.nsec, last.sec, last.nsec);
  printf("Step 1 took %f seconds\n", tm_2dbl(delta));

  sleep(2);
  printf("Step 2 took %f seconds\n", tm_2dbl(tm_mark(&last, tm_uget())));
  sleep(3);
  printf("Last was %f seconds\n", tm_2dbl(tm_mark(&last, tm_uget())));

  printf("Total time was %f seconds\n", tm_2dbl(tm_sub(last, start)));

  last = tm_dset(tm_2dbl(last) * 3.0);
  printf("Multiplied by 3 : %f\n", tm_2dbl(last));

  last = tm_dset(tm_2dbl(last) / 3.0);
  printf("Divided by 3 : %f\n", tm_2dbl(last));

  last = tm_dset(tm_2dbl(last) / 0.3);
  printf("Divided by .3 : %f\n", tm_2dbl(last));

  last = tm_dset(tm_2dbl(last) * 0.3);
  printf("Multiplied by .3 : %f\n", tm_2dbl(last));

  last = tm_dset(tm_2dbl(last) / 7.5);
  printf("Divided by 7.5 : %f\n", tm_2dbl(last));

  last = tm_dset(tm_2dbl(last) * 7.5);
  printf("Multiplied by 7.5 : %f\n", tm_2dbl(last));

  return 0;
}
