#ifndef BAY_INCLUDED
#define BAY_INCLUDED
#include "array.h"

typedef struct Env Env;

typedef struct Bay
{
    int R;
    int C;
    int N;
    int *right_most_added_column;
    int *left_most_removed_column;
    int *added_since_sailing;
    Array matrix;
    Array min_container_per_column;
    Array column_counts;
    Array left_most_identical_index;
    Array right_most_identical_index;
    Array max_to_place_for_identical;
    Array max_to_remove_for_identical;
} Bay;

Bay get_bay(int R, int C, int N);

void free_bay(Bay bay);

Bay copy_bay(Bay bay);

void reset_identical_add_limitation(Bay bay);

void reset_identical_remove_limitation(Bay bay);

int columns_identical(Bay bay, int c1, int c2);

int is_container_blocking(Bay bay, int column, int container);

void bay_add_containers(Bay bay, int column, int container, int amount);

Array bay_sail_along(Bay bay, Env *env);

Array bay_pop_containers(Bay bay, int column, int amount);

int containers_in_column(Bay bay, int column);

void reset_right_most_added_column(Bay bay);

#endif