#ifndef __cat_emalloc_h
#define __cat_emalloc_h
#include <cat/cat.h>

typedef void (*emalloc_abort_f)(char *s, void *omem, size_t size, size_t nmem,
                                int syserr);

void * emalloc(size_t size);
void * ecalloc(size_t nmemb, size_t size);
void * erealloc(void *old, size_t size);

/* The abort function must NOT return!  (longjmp is ok) */
void   emalloc_set_abort(emalloc_abort_f newfunc);

#endif /* __cat_emalloc_h */
