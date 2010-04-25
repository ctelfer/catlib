/*
 * cat/sort.h -- Sorting arrays
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2009, See accompanying license
 *
 */

#ifndef __cat_sort_h
#define __cat_sort_h

#include <cat/cat.h>

void isort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp);

void ssort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp);

void hsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp);

void qsort_array(void *arr, const size_t nelem, const size_t esize, cmp_f cmp);

void array_to_voidp(void **varr, void *arr, size_t nelem, size_t esize);

void permute_array(void *arr, void *tswap, void **varr, size_t nelem, size_t esize);

#endif /* __cat_sort_h */
