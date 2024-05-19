#include "bay.h"
#include "sort.h"
#include <assert.h>
#include <stdio.h>

int containers_in_column(Bay bay, int column)
{
    return bay.column_counts.values[column];
}

int is_column_empty(Bay bay, int column)
{
    return containers_in_column(bay, column) == 0;
}

int is_full_column(Bay bay, int column)
{
    return containers_in_column(bay, column) == bay.R;
}

Bay copy_bay(Bay bay)
{
    Bay copy;
    copy.R = bay.R;
    copy.C = bay.C;
    copy.N = bay.N;
    copy.matrix = copy_array(bay.matrix);
    copy.min_container_per_column = copy_array(bay.min_container_per_column);
    copy.column_counts = copy_array(bay.column_counts);
    copy.added_since_sailing = copy_array(bay.added_since_sailing);
    return copy;
}

int find_min_container_in_column(Bay bay, int column)
{
    int min = bay.N;
    const int start_row = bay.R - containers_in_column(bay, column);

    for (int i = start_row; i < bay.R; i++)
    {
        int container = bay.matrix.values[i * bay.C + column];
        if (container < min)
        {
            min = container;
        }
    }

    return min;
}

int *min_container_ref(Bay bay, int column)
{
    return &bay.min_container_per_column.values[column];
}

// Checks bottom up for containers going to current port (always 1)
// If container is 1, then it is offloaded, and so are all the containers above it
void offload_column(Bay bay, int j, Array offloaded_containers, Env *env)
{
    int offload_column_rest = 0;
    const int min_row = bay.R - containers_in_column(bay, j);
    const int max_row = bay.R - 1;
    for (int i = max_row; i >= min_row; i--)
    {
        const int index = i * bay.C + j;
        const int container = bay.matrix.values[index];

        if (container == 1)
            offload_column_rest = 1;

        if (offload_column_rest)
        {
            offloaded_containers.values[container]++;
            bay.matrix.values[index] = 0;
            bay.column_counts.values[j]--;
        }
    }
    if (offload_column_rest)
    {
        *min_container_ref(bay, j) = find_min_container_in_column(bay, j);
    }
}

Array bay_offload_containers(Bay bay, Env *env)
{
    Array offloaded_containers = get_zeros(bay.N);
    for (int j = 0; j < bay.C; j++)
    {
        offload_column(bay, j, offloaded_containers, env);
    }

    return offloaded_containers;
}

void decrement_bay(Bay bay)
{
    for (int j = 0; j < bay.C; j++)
    {
        for (int i = bay.R - containers_in_column(bay, j); i < bay.R; i++)
        {
            const int index = i * bay.C + j;
            bay.matrix.values[index]--;
        }
        if (containers_in_column(bay, j) > 0)
            bay.min_container_per_column.values[j]--;
    }
}

void reset_added_since_sailing(Bay bay)
{
    for (int i = 0; i < bay.C; i++)
    {
        bay.added_since_sailing.values[i] = 0;
    }
}

// Offloads all containers going to current port (always 1)
// Returns reshuffled containers
Array bay_sail_along(Bay bay, Env *env)
{
    Array reshuffled = bay_offload_containers(bay, env);
    decrement_bay(bay);
    // Ignore 1s, as they are offloaded
    reshuffled.values[1] = 0;
    // And also decrement the reshuffled containers
    shift_array_left(reshuffled, 1);
    reset_added_since_sailing(bay);
    return reshuffled;
}

Bay get_bay(int R, int C, int N)
{
    Bay bay;
    bay.R = R;
    bay.C = C;
    bay.N = N;
    bay.matrix = get_zeros(R * C);
    // Initialize to max value + 1
    bay.min_container_per_column = get_full(C, N);
    bay.column_counts = get_zeros(C);
    bay.added_since_sailing = get_zeros(C);

    return bay;
}

void free_bay(Bay bay)
{
    free_array(bay.matrix);
    free_array(bay.min_container_per_column);
    free_array(bay.column_counts);
    free_array(bay.added_since_sailing);
}

void update_min_post_insertion(Bay bay, int column, int container)
{
    if (container < *min_container_ref(bay, column))
    {
        *min_container_ref(bay, column) = container;
    }
}

void insert_containers_into_column(Bay bay, int column, int amount, int container)
{
    for (int i = 0; i < amount; i++)
    {
        int row = bay.R - containers_in_column(bay, column) - 1 - i;
        bay.matrix.values[row * bay.C + column] = container;
    }
    bay.column_counts.values[column] += amount;
}

Array get_lexicographic_column_order(Bay bay, int first_affected_row)
{
    Array column_order = get_range(0, bay.C);

    // You only need to check the first affected row and down
    for (int i = first_affected_row; i < bay.R; i++)
    {
        int *row_values = bay.matrix.values + i * bay.C;
        Array row_array = array_from_ints_shallow_copy(row_values, bay.C);
        sort_indexes_using_values(column_order, row_array);
    }

    return column_order;
}

void reorder_bay(Bay bay, int first_affected_row)
{
    Array correct_column_order = get_lexicographic_column_order(bay, first_affected_row);

    reorder_matrix_columns(bay.matrix, bay.C, bay.R, correct_column_order);
    reorder_array(bay.min_container_per_column, correct_column_order);
    reorder_array(bay.column_counts, correct_column_order);
    reorder_array(bay.added_since_sailing, correct_column_order);

    free_array(correct_column_order);
}

void bay_add_containers(Bay bay, int column, int container, int amount, int should_reorder)
{
    assert(column >= 0 && column < bay.C);
    assert(containers_in_column(bay, column) + amount <= bay.R);
    insert_containers_into_column(bay, column, amount, container);
    update_min_post_insertion(bay, column, container);
    bay.added_since_sailing.values[column] = 1;
    if (should_reorder)
    {
        int first_affected_row = bay.R - containers_in_column(bay, column);
        reorder_bay(bay, first_affected_row);
    }
}

Array pop_containers_from_column(Bay bay, int column, int amount)
{
    Array reshuffled = get_zeros(bay.N);
    for (int i = 0; i < amount; i++)
    {
        int row = bay.R - containers_in_column(bay, column) + i;
        int index = row * bay.C + column;
        int container = bay.matrix.values[index];

        reshuffled.values[container]++;

        bay.matrix.values[index] = 0;
    }

    bay.column_counts.values[column] -= amount;
    return reshuffled;
}

// Assumes the container would be placed at the top of the column
int is_container_blocking(Bay bay, int column, int container)
{
    return container > *min_container_ref(bay, column);
}

void update_min_post_removal(Bay bay, int column)
{
    *min_container_ref(bay, column) = find_min_container_in_column(bay, column);
}

Array bay_pop_containers(Bay bay, int column, int amount, int should_reorder)
{
    assert(column >= 0 && column < bay.C);
    assert(containers_in_column(bay, column) - amount >= 0);
    Array reshuffled = pop_containers_from_column(bay, column, amount);
    update_min_post_removal(bay, column);

    if (should_reorder)
    {
        int first_affected_row = bay.R - containers_in_column(bay, column) - 1;
        reorder_bay(bay, first_affected_row);
    }

    return reshuffled;
}