#define _GNU_SOURCE
#include "array.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct
{
    int index;
    int original_index;
} indexed_value;

#ifdef __GLIBC__
int compare_indexes_using_values(const void *a, const void *b, void *values)
{
#else
int compare_indexes_using_values(void *values, const void *a, const void *b)
{
#endif
    int *values_array = (int *)values;
    indexed_value *ia = (indexed_value *)a;
    indexed_value *ib = (indexed_value *)b;
    int value_a = values_array[ia->index];
    int value_b = values_array[ib->index];

    if (value_a < value_b)
        return -1;
    else if (value_a > value_b)
        return 1;
    else
        return ia->original_index - ib->original_index;
}

void sort_indexes_using_values(Array indexes, Array values)
{
    assert(indexes.n == values.n);

    indexed_value *indexed_values = malloc(indexes.n * sizeof(indexed_value));
    for (size_t i = 0; i < indexes.n; i++)
    {
        indexed_values[i].index = ((int *)indexes.values)[i];
        indexed_values[i].original_index = i;
    }

#ifdef __GLIBC__
    qsort_r(indexed_values, indexes.n, sizeof(indexed_value), compare_indexes_using_values, values.values);
#else
    qsort_r(indexed_values, indexes.n, sizeof(indexed_value), values.values, compare_indexes_using_values);
#endif

    for (size_t i = 0; i < indexes.n; i++)
    {
        ((int *)indexes.values)[i] = indexed_values[i].index;
    }

    free(indexed_values);
}