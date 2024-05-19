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
    Array added_since_sailing;
} Bay;

Bay get_bay(int R, int C, int N);

void free_bay(Bay bay);

Bay copy_bay(Bay bay);

int is_container_blocking(Bay bay, int column, int container);

void bay_add_containers(Bay bay, int column, int container, int amount, int should_reorder);

Array bay_sail_along(Bay bay, Env *env);

Array bay_pop_containers(Bay bay, int column, int amount, int should_reorder);

int containers_in_column(Bay bay, int column);
#endif