#ifndef TRANSPORTATION_MATRIX_INCLUDED
#define TRANSPORTATION_MATRIX_INCLUDED
#include "array.h"

#define min(a, b) (a < b ? a : b)

typedef struct Transportation_Info
{
    Array matrix;
    Array containers_per_port;
    int N;
    int seed;
    int last_non_zero_column;
    int current_port;
    int containers_left;
    int containers_placed;
} Transportation_Info;

Transportation_Info *get_random_transportation_matrix(
    int N,
    int bay_capacity);

Transportation_Info *get_specific_transportation_matrix(
    int N,
    int *T_matrix);

Transportation_Info *copy_transportation_info(Transportation_Info *T);

int is_last_port(Transportation_Info *T);

void free_transportation_matrix(Transportation_Info *T);

void transportation_sail_along(Transportation_Info *T);

int transportation_pop_container(Transportation_Info *T);

void transportation_insert_container(Transportation_Info *T, int container);

void transportation_insert_reshuffled(Transportation_Info *T, Array reshuffled);

int no_containers_at_port(Transportation_Info *T);

#endif