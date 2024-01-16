#ifndef ARRAY_INCLUDED
#define ARRAY_INCLUDED

typedef struct Array
{
    int *values;
    int n;
} Array;

void free_array(Array array);

void print_array(Array array);

Array get_zeros(int n);

Array get_full(int n, int value);

void fill_array(Array array, int value);

Array get_range(int start, int end);

void shuffle_array(Array array);

void shift_array_left(Array array, int shifts);

void shift_array_right(Array array, int shifts);

void print_matrix(Array array, int w, int h);

Array array_from_ints(int *values, int n);

Array array_from_ints_shallow_copy(int *values, int n);

void reorder_array(Array array, Array order);

void reorder_matrix_columns(Array array, int w, int h, Array order);

#endif