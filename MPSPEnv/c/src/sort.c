#define _GNU_SOURCE
#include "array.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __GLIBC__
int compare_indexes_using_values(const void *a, const void *b, void *values)
{
#else
int compare_indexes_using_values(void *values, const void *a, const void *b)
{
#endif
    int *values_array = (int *)values;
    int index_a = *(int *)a;
    int index_b = *(int *)b;
    int value_a = values_array[index_a];
    int value_b = values_array[index_b];

    if (value_a < value_b)
        return -1;
    else if (value_a > value_b)
        return 1;
    else
        return 0;
}

void sort_indexes_using_values(Array indexes, Array values)
{
    assert(indexes.n == values.n);
#ifdef __GLIBC__
    qsort_r(indexes.values, indexes.n, sizeof(int), compare_indexes_using_values, values.values);
#else
    qsort_r(indexes.values, indexes.n, sizeof(int), values.values, compare_indexes_using_values);
#endif
}