#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <cat/mem.h>
#include <cat/stduse.h>

#define NREPS 50000
#define NMEM 100
unsigned nreps = NREPS;

#define TEST(op) \
	{					\
	  struct timeval start, stop;		\
	  double usec, nspo;			\
	  int i;				\
						\
	  gettimeofday(&start, NULL);		\
	  for ( i = 0 ; i < nreps; ++i )	\
	    { op; }				\
	  gettimeofday(&stop, NULL);		\
	  usec = (stop.tv_sec - start.tv_sec) * 1000000 + \
		 (stop.tv_usec - start.tv_usec);\
	  nspo = 1000 * usec / nreps;		\
	  printf("%-40s\t%lf\n", #op, nspo);	\
	  fflush(stdout);			\
	}


#define HEAD printf("%-40s\tnanoseconds\n", "Op"); \
	     printf("%-40s\t-----------\n", "--");


int main(int argc, char *argv[])
{
	void *m;
	void *arr[NMEM];
	int k;
	
	TEST(m=malloc(50); free(m));
	TEST(m=mem_get(&estdmm,50); mem_free(&estdmm,m));

	printf("\n");
	TEST(m=malloc(256); free(m));
	TEST(m=mem_get(&estdmm,256); mem_free(&estdmm,m));

	printf("\n");
	TEST(m=malloc(4096); free(m));
	TEST(m=mem_get(&estdmm,4096); mem_free(&estdmm,m));

	printf("\n");
	TEST(m=malloc(65536); free(m));
	TEST(m=mem_get(&estdmm,65536); mem_free(&estdmm,m));

	printf("\n");
	TEST(m=malloc(65538); free(m));
	TEST(m=mem_get(&estdmm,65538); mem_free(&estdmm,m));

	printf("\n");
	TEST(for(k=0;k<NMEM;++k)arr[k]=malloc(256);while(k-->0)free(arr[k]););
	TEST(for(k=0;k<NMEM;++k)arr[k]=mem_get(&estdmm,256);while(k-->0)mem_free(&estdmm,arr[k]););

	return 0;
}
