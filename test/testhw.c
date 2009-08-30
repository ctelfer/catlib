#include <sys/time.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define NREPS 10000
int nreps = NREPS;

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
	  printf("%-40s\t%f\n", #op, nspo);	\
	  fflush(stdout);			\
	}

#define HEAD printf("%-40s\tnanoseconds\n", "Op"); \
	     printf("%-40s\t-----------\n", "--");

void f() { } 


struct obj { 
	int (*opf)(volatile struct obj *, int);
};


int op(volatile struct obj *op, int i) {
	return i;
}


int main(int argc, char *argv[])
{
  volatile int a, b=2, c=1, *ip;
  volatile float f1, f2, f3 = 1;
  volatile double d1, d2, d3 = 1;
  volatile struct { int a, b; char c; double d; } st, *stp = &st;
  volatile struct obj obj = { op }, *objp = &obj;
  volatile int arr[50];

  st.b = 1;

  if ( argc > 1 )
    nreps = atoi(argv[1]);

  printf("Type:          Size:\n");
  printf("char           %3u\n", (uint)sizeof(char));
  printf("short          %3u\n", (uint)sizeof(short));
  printf("int            %3u\n", (uint)sizeof(int));
  printf("long           %3u\n", (uint)sizeof(long));
  printf("long long      %3u\n", (uint)sizeof(long long));
  printf("char *         %3u\n", (uint)sizeof(char *));
  printf("size_t         %3u\n", (uint)sizeof(size_t));
  printf("ssize_t        %3u\n", (uint)sizeof(ssize_t));
  printf("ptrdiff_t      %3u\n", (uint)sizeof(ptrdiff_t));
  printf("float          %3u\n", (uint)sizeof(float));
  printf("double         %3u\n", (uint)sizeof(double));
  printf("long double    %3u\n", (uint)sizeof(long double));

  printf("\nInteger operations\n");
  HEAD
  TEST({})
  TEST(a = 1)
  TEST(a = b + c)
  TEST(a = b - c)
  TEST(a = b * c)
  TEST(a = b / c)
  TEST(a = b % c)
  TEST(a = b << c)
  TEST(a = b >> c)
  TEST(a = b & c)
  TEST(a = b | c)
  TEST(a = b ^ c)
  TEST(a = ~b)
  TEST(a = !b)

  printf("\nFloating point operations\n");
  HEAD
  TEST(f1 = 1)
  TEST(f1 = f2 + f3)
  TEST(f1 = f2 - f3)
  TEST(f1 = f2 * f3)
  TEST(f1 = f2 / f3)

  printf("\nDouble precision floating point operations\n");
  HEAD
  TEST(d1 = 1)
  TEST(d1 = d2 + d3)
  TEST(d1 = d2 - d3)
  TEST(d1 = d2 * d3)
  TEST(d1 = d2 / d3)

  printf("\nArray operations\n");
  HEAD
  TEST(arr[0] = 1)
  TEST(arr[c] = 1)
  TEST(a = arr[c] + b)
  TEST(a = b + arr[c])
  TEST(a = arr[b] + arr[c])

  printf("\nPointers\n");
  HEAD
  TEST(ip = &a)
  TEST(*ip = 1)
  TEST(*ip = b)
  TEST(stp = &st)
  TEST(stp->a = 5)
  TEST(stp->a = b)
  TEST(obj.opf(&obj, 3))
  TEST(objp->opf(objp, 5))

  printf("\nOther Statements\n");
  HEAD
  TEST(if ( a < 5 ) b = 2)
  TEST(if ( arr[c] < 0 ) b = 2)
  TEST(a = (b < c) ? b : c)
  TEST(st.a = 1)
  TEST(a = b & st.b)
  TEST(if ( st.b ) a = b & st.b; else a = b % st.b)
  TEST(if (stp->b) a = b&stp->b; else a=b%stp->b)
  TEST(if ( !st.b ) a = b & st.b; else a = b % st.b)
  TEST(if (!stp->b) a = b&stp->b; else a=b%stp->b)
  TEST(f())
  TEST(op(&obj, 10))
  TEST(free(malloc(16)))
  TEST(free(malloc(200)))
  TEST(free(malloc(550)))
  TEST(free(malloc(65536)))
  TEST(free(malloc(65586)))

  return 0;
}
