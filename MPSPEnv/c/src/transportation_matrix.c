#include "transportation_matrix.h"
#include "random.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void increase_capacity_by_offloaded_containters(Transportation_Info *T, int row, int *bay_capacity)
{
    for (int i = 0; i < row + 1; i++)
    {
        *bay_capacity += T->matrix.values[i * T->N + row + 1];
    }
}

void insert_row(Transportation_Info *T, int row, int *bay_capacity)
{
    // nonzero_in_row follows from the upper triangular structure
    int nonzero_in_row = T->N - row - 1;
    int *new_row = random_partition(*bay_capacity, nonzero_in_row);
    for (int i = 0; i < nonzero_in_row; i++)
    {
        int x = row + 1 + i;
        int y = row;
        T->matrix.values[y * T->N + x] = new_row[i];
    }
    *bay_capacity = 0;
    free(new_row);
}

void insert_seeded_transportation_matrix(Transportation_Info *T, int bay_capacity)
{
    set_seed(T->seed);
    for (int row = 0; row < T->N - 1; row++)
    {
        if (bay_capacity > 0)
            insert_row(T, row, &bay_capacity);
        increase_capacity_by_offloaded_containters(T, row, &bay_capacity);
    }
}

void insert_containers_per_port(Transportation_Info *T)
{
    // Upper Triangular Indeces:
    // i in [0, N)
    // j in [i + 1, N]
    for (int i = 0; i < T->N - 1; i++)
    {
        T->containers_per_port.values[i] = 0;
        for (int j = i + 1; j < T->N; j++)
        {
            T->containers_per_port.values[i] += T->matrix.values[i * T->N + j];
        }
    }
}

void update_last_non_zero_column(Transportation_Info *T, int inserted_container)
{
    if (inserted_container > T->last_non_zero_column)
    {
        T->last_non_zero_column = inserted_container;
    }
}

void increment_matrix_and_port(Transportation_Info *T, int port, int container, int n_containers)
{
    T->matrix.values[port * T->N + container] += n_containers;
    T->containers_per_port.values[port] += n_containers;
    T->containers_left += n_containers;
}

void decrement_matrix_and_port(Transportation_Info *T, int port, int container)
{
    T->matrix.values[port * T->N + container] -= 1;
    T->containers_per_port.values[port] -= 1;
    T->containers_left -= 1;
}

void insert_containers(Transportation_Info *T, int container, int n_containers)
{
    increment_matrix_and_port(T, 0, container, n_containers);
    update_last_non_zero_column(T, container);
}

void transportation_insert_container(Transportation_Info *T, int container)
{
    insert_containers(T, container, 1);
}

void transportation_insert_reshuffled(Transportation_Info *T, Array reshuffled)
{
    for (int i = 1; i < reshuffled.n; i++)
    {
        int n_reshuffled = reshuffled.values[i];
        if (n_reshuffled > 0)
        {
            insert_containers(T, i, n_reshuffled);
        }
    }
}

int is_last_port(Transportation_Info *T)
{
    return T->current_port == T->N - 1;
}

void shift_up_left(Transportation_Info *T, int shifts)
{
    for (int i = 0; i < T->N - 1; i++)
    {
        for (int j = i + 1; j < T->N; j++)
        {
            int can_shift = i < T->N - 1 - shifts && j < T->N - shifts;
            int index = i * T->N + j;
            if (can_shift)
            {
                int shifted_index = (i + shifts) * T->N + (j + shifts);
                T->matrix.values[index] = T->matrix.values[shifted_index];
            }
            else
            {
                T->matrix.values[index] = 0;
            }
        }
    }
}

void insert_last_non_zero_column(Transportation_Info *T)
{
    assert(T->last_non_zero_column >= 0);

    for (int j = T->last_non_zero_column; j >= 1; j--)
    {
        if (T->matrix.values[j] != 0)
        {
            T->last_non_zero_column = j;
            return;
        }
    }

    T->last_non_zero_column = -1;
}

void reset_last_non_zero_column(Transportation_Info *T)
{
    T->last_non_zero_column = T->N - 1;
    insert_last_non_zero_column(T);
}

void transportation_sail_along(Transportation_Info *T)
{
    shift_up_left(T, 1);
    shift_array_left(T->containers_per_port, 1);

    T->current_port += 1;
    assert(T->current_port < T->N);

    reset_last_non_zero_column(T);
}

int no_containers_at_port(Transportation_Info *T)
{
    return T->containers_per_port.values[0] == 0;
}

int transportation_pop_container(Transportation_Info *T)
{
    assert(!is_last_port(T));
    assert(!no_containers_at_port(T));
    assert(T->last_non_zero_column >= 0);

    int container = T->last_non_zero_column;

    decrement_matrix_and_port(T, 0, T->last_non_zero_column);
    insert_last_non_zero_column(T);

    T->containers_placed += 1;

    return container;
}

Transportation_Info *copy_transportation_info(Transportation_Info *T)
{
    Transportation_Info *copy = malloc(sizeof(Transportation_Info));
    copy->N = T->N;
    copy->seed = T->seed;
    copy->matrix = copy_array(T->matrix);
    copy->containers_per_port = copy_array(T->containers_per_port);
    copy->last_non_zero_column = T->last_non_zero_column;
    copy->current_port = T->current_port;
    copy->containers_left = T->containers_left;
    copy->containers_placed = T->containers_placed;

    return copy;
}

Transportation_Info *get_empty_transportation_matrix(
    int N)
{
    Transportation_Info *T = malloc(sizeof(Transportation_Info));

    T->N = N;
    T->seed = random_int();
    T->matrix = get_zeros(N * N);
    T->containers_per_port = get_zeros(N);
    T->last_non_zero_column = N - 1;
    T->current_port = 0;
    T->containers_left = 0;
    T->containers_placed = 0;

    return T;
}

void insert_t_matrix(Transportation_Info *T, int *T_matrix)
{
    for (int i = 0; i < T->N - 1; i++)
    {
        for (int j = i + 1; j < T->N; j++)
        {
            int index = i * T->N + j;
            T->matrix.values[index] = T_matrix[index];
        }
    }
}

void insert_containers_left(Transportation_Info *T)
{
    for (int i = 0; i < T->N; i++)
    {
        T->containers_left += T->containers_per_port.values[i];
    }
}

Transportation_Info *get_random_transportation_matrix(
    int N,
    int bay_capacity)
{
    assert(N > 0);
    assert(bay_capacity > 0);

    Transportation_Info *T = get_empty_transportation_matrix(N);
    insert_seeded_transportation_matrix(T, bay_capacity);
    insert_containers_per_port(T);
    insert_last_non_zero_column(T);
    insert_containers_left(T);

    return T;
}
Transportation_Info *get_specific_transportation_matrix(
    int N,
    int *T_matrix)
{
    assert(N > 0);

    Transportation_Info *T = get_empty_transportation_matrix(N);
    insert_t_matrix(T, T_matrix);
    insert_containers_per_port(T);
    insert_last_non_zero_column(T);
    insert_containers_left(T);

    return T;
}

void free_transportation_matrix(Transportation_Info *T)
{
    free_array(T->matrix);
    free_array(T->containers_per_port);
    free(T);
}