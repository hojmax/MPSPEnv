#ifndef BAY_INCLUDED
#define BAY_INCLUDED
#include "array.h"

typedef struct Env Env;

// Define a type for the callback function
typedef void (*ReshuffleCallback)(int row, int column, Env *env);

typedef struct Bay
{
    int R;
    int C;
    int N;
    Array matrix;
    Array min_container_per_column;
    Array column_counts;
    Array mask;
} Bay;

Bay get_bay(int R, int C, int N);

void free_bay(Bay bay);

Bay copy_bay(Bay bay);

int is_container_blocking(Bay bay, int column, int container);

int get_top_container(Bay bay, int column);

void bay_add_container(Bay bay, int column, int container);

Array bay_sail_along(Bay bay, ReshuffleCallback callback, Env *env);

void bay_pop_container(Bay bay, int column);

#endif