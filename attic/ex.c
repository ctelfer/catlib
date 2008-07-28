/*
 * ex.c -- Exception handling via longjmp() 
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#include <cat/cat.h>
#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB

#include <stdlib.h>
#include <cat/ex.h>
#include <cat/err.h>


#if defined(CAT_USE_PTHREADS) && CAT_USE_PTHREADS

#include <pthread.h>
static pthread_once_t cat_exonce = PTHREAD_ONCE_INIT;
static pthread_key cat_exkey;



static void clrexstack(void *stack)
{
	free(stack);
}



static void initex(void)
{
	if ( pthread_key_create(&cat_exkey, clrexstack) < 0 )
		errsys("pthread_key_create in initex:\n\t");
}

#else /* CAT_USE_PTHREADS */

static struct ex_stack ExStack = { -1, NULL };

#endif /* CAT_USE_PTHREADS */ 


int ex_try(void)
{
	struct ex_stack *esp;
	struct ex_ent *eep;

#if defined(CAT_USE_PTHREADS) && CAT_USE_PTHREADS

	/* memory leak:  must free the memory before this thread terminates */
	pthread_once(&cat_exonce, initex());
	if ( !(esp = pthread_getpsecific(cat_exkey)) ) {

		esp = malloc(sizeof(*esp));
		if ( !esp )
			err("ex_try:  out of memory\n");

		memset(esp, 0, sizeof(*esp));
		esp->top = -1;
		esp->uncaught = 0;
		pthread_setspecific(cat_exkey, esp);
	}

#else /* CAT_USE_PTHREADS */

	esp = &ExStack;

#endif /* CAT_USE_PTHREADS */

	if ( esp->top == MAXEXSTK ) 
		err( "too many nested ex_try()s! (max = %d)\n", MAXEXSTK);
  
	eep = &esp->stack[++esp->top];
	eep->data = 0;
  
	return setjmp(eep->env) ? 0 : 1;
}



static struct ex_stack *getexstack()
{
  struct ex_stack *esp;

#if defined(CAT_USE_PTHREADS) && CAT_USE_PTHREADS

	if ( (esp = pthread_getspecific(cat_exkey)) )
		errsys("pthread_getspecific:\n\t");

#else /* CAT_USE_PTHREADS */

	esp = &ExStack;

#endif /* CAT_USE_PTHREADS */

	return esp;
}




void ex_throw(void *data)
{
	struct ex_stack *esp;

	esp = getexstack();

	if ( esp->top < 0 ) {
		if ( !esp->uncaught )
			err("Uncaught exception: %s\nTerminating!\n", data);
		else {
			esp->uncaught(data);
			exit(-1);
		}
	}

	esp->stack[esp->top].data = data;
	longjmp(esp->stack[esp->top].env, 1);
}




void * ex_get(void)
{
	struct ex_stack *esp;

	esp = getexstack();

	if ( esp->top < 0 )
		err("Attempt to obtain data when exception stack is empty!\n");

	return esp->stack[esp->top].data;
}




void ex_end(void)
{
	struct ex_stack *esp;

	esp = getexstack();

	if ( esp->top < 0 )
		err("Exception end with no begin! (terminating)");

	esp->top--;
}




void ex_setu(void (*uncaught)(void *))
{
	struct ex_stack *esp;
	esp = getexstack();
	esp->uncaught = uncaught;
}





/* 
 *
 * void printandfree(void *string)
 * {
 *   printf("Uncaught exception %s\n", string);
 *   free(string);
 * }
 *
 * 
 * ex_setu(printandfree);
 *
 *
 * if ( ex_try() )
 * {
 *
 *   if ( some_error )
 *     ex_throw(str_dup("Hello World"));
 *
 * } else { 
 *   char *exc;
 *   exc = ex_get();
 *   printf("We caught an error:  %s\n", exc);
 *   free(exc);
 * }
 * ex_end();
 *
 * ex_throw(str_dup("Another exception!\n"));
 *
 * */

#endif /* if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB */
