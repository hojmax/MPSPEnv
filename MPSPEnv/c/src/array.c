#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "array.h"
#include "random.h"

void free_array(Array array)
{
    free(array.values);
}

void print_array(Array array)
{
    printf("Array[%d]: ", array.n);
    for (int i = 0; i < array.n; i++)
    {
        printf("%d ", array.values[i]);
    }
    printf("\n");
}

Array copy_array(Array array)
{
    Array copy;
    copy.values = (int *)calloc(array.n, sizeof(int));
    copy.n = array.n;
    for (int i = 0; i < array.n; i++)
    {
        copy.values[i] = array.values[i];
    }
    return copy;
}

void print_matrix(Array array, int w, int h)
{
    assert(array.n == w * h);
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            printf("%d ", array.values[i * w + j]);
        }
        printf("\n");
    }
}

Array get_zeros(int n)
{
    assert(n > 0);
    Array array;
    array.values = (int *)calloc(n, sizeof(int));
    array.n = n;
    return array;
}

Array get_full(int n, int value)
{
    assert(n > 0);
    Array array;
    array.values = (int *)calloc(n, sizeof(int));
    array.n = n;
    for (int i = 0; i < n; i++)
    {
        array.values[i] = value;
    }
    return array;
}

// Returns range [start, end)
Array get_range(int start, int end)
{
    assert(start < end);
    Array array;
    array.values = malloc((end - start) * sizeof(int));
    array.n = end - start;
    for (int i = 0; i < array.n; i++)
    {
        array.values[i] = start + i;
    }
    return array;
}

void shift_array_left(Array array, int n_shifts)
{
    assert(n_shifts >= 0);
    for (int i = 0; i < array.n - n_shifts; i++)
    {
        array.values[i] = array.values[i + n_shifts];
    }
    for (int i = array.n - n_shifts; i < array.n; i++)
    {
        array.values[i] = 0;
    }
}

Array array_from_ints(int *values, int n)
{
    Array array;
    array.values = (int *)calloc(n, sizeof(int));
    array.n = n;
    for (int i = 0; i < n; i++)
    {
        array.values[i] = values[i];
    }
    return array;
}

Array array_from_ints_shallow_copy(int *values, int n)
{
    Array array;
    array.values = values;
    array.n = n;
    return array;
}

void reorder_array(Array array, Array order)
{
    assert(array.n == order.n);
    int *temp = (int *)calloc(array.n, sizeof(int));
    for (int i = 0; i < array.n; i++)
    {
        assert(order.values[i] >= 0 && order.values[i] < array.n);
        temp[i] = array.values[order.values[i]];
    }
    for (int i = 0; i < array.n; i++)
    {
        array.values[i] = temp[i];
    }
    free(temp);
}

void reorder_matrix_columns(Array array, int w, int h, Array order)
{
    assert(array.n == w * h);
    assert(order.n == w);
    int *temp = (int *)calloc(array.n, sizeof(int));

    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int new_column = order.values[j];
            temp[i * w + j] = array.values[i * w + new_column];
        }
    }

    for (int i = 0; i < array.n; i++)
    {
        array.values[i] = temp[i];
    }

    free(temp);
}