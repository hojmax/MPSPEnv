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
    copy.mask = copy_array(bay.mask);
    return copy;
}

void update_column_mask(Bay bay, int column)
{
    // Add mask
    bay.mask.values[column] = !is_full_column(bay, column);
    // Remove mask
    bay.mask.values[column + bay.C] = !is_column_empty(bay, column);
}

void insert_mask(Bay bay)
{
    for (int i = 0; i < bay.C; i++)
    {
        update_column_mask(bay, i);
    }
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
void offload_column(Bay bay, int j, Array offloaded_containers, ReshuffleCallback callback, Env *env)
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

        if (offload_column_rest && container != 1)
            callback(i, j, env);

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

Array bay_offload_containers(Bay bay, ReshuffleCallback callback, Env *env)
{
    Array offloaded_containers = get_zeros(bay.N);
    for (int j = 0; j < bay.C; j++)
    {
        offload_column(bay, j, offloaded_containers, callback, env);
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

// Offloads all containers going to current port (always 1)
// Returns reshuffled containers
Array bay_sail_along(Bay bay, ReshuffleCallback callback, Env *env)
{
    Array reshuffled = bay_offload_containers(bay, callback, env);
    decrement_bay(bay);
    // Ignore 1s, as they are offloaded
    reshuffled.values[1] = 0;
    // And also decrement the reshuffled containers
    shift_array_left(reshuffled, 1);
    insert_mask(bay);
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
    bay.mask = get_zeros(2 * bay.C);
    insert_mask(bay);

    return bay;
}

void free_bay(Bay bay)
{
    free_array(bay.matrix);
    free_array(bay.min_container_per_column);
    free_array(bay.column_counts);
    free_array(bay.mask);
}
void update_min_post_insertion(Bay bay, int column, int container)
{
    if (container < *min_container_ref(bay, column))
    {
        *min_container_ref(bay, column) = container;
    }
}

void insert_container_into_column(Bay bay, int column, int container)
{
    int row = bay.R - containers_in_column(bay, column) - 1;
    bay.matrix.values[row * bay.C + column] = container;
    bay.column_counts.values[column]++;
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

void reorder_mask(Bay bay, Array correct_column_order)
{
    Array add_mask = array_from_ints_shallow_copy(bay.mask.values, bay.C);
    Array remove_mask = array_from_ints_shallow_copy(bay.mask.values + bay.C, bay.C);
    reorder_array(add_mask, correct_column_order);
    reorder_array(remove_mask, correct_column_order);
}

void reorder_bay(Bay bay, int first_affected_row)
{
    Array correct_column_order = get_lexicographic_column_order(bay, first_affected_row);

    reorder_matrix_columns(bay.matrix, bay.C, bay.R, correct_column_order);
    reorder_array(bay.min_container_per_column, correct_column_order);
    reorder_array(bay.column_counts, correct_column_order);
    reorder_mask(bay, correct_column_order);

    free_array(correct_column_order);
}

void bay_add_container(Bay bay, int column, int container)
{
    assert(column >= 0 && column < bay.C);
    assert(containers_in_column(bay, column) < bay.R);
    insert_container_into_column(bay, column, container);
    update_min_post_insertion(bay, column, container);
    update_column_mask(bay, column);
    int first_affected_row = bay.R - containers_in_column(bay, column);
    reorder_bay(bay, first_affected_row);
}

int get_top_container(Bay bay, int column)
{
    assert(column >= 0 && column < bay.C);
    assert(containers_in_column(bay, column) > 0);
    int row = bay.R - containers_in_column(bay, column);
    int index = row * bay.C + column;
    return bay.matrix.values[index];
}

int pop_container_from_column(Bay bay, int column)
{
    int row = bay.R - containers_in_column(bay, column);
    int index = row * bay.C + column;
    int container = bay.matrix.values[index];

    bay.matrix.values[index] = 0;
    bay.column_counts.values[column]--;

    return container;
}

// Assumes the container would be placed at the top of the column
int is_container_blocking(Bay bay, int column, int container)
{
    return container > *min_container_ref(bay, column);
}

void update_min_post_removal(Bay bay, int column, int container)
{
    if (*min_container_ref(bay, column) < container)
        return;

    if (containers_in_column(bay, column) == 0)
    {
        *min_container_ref(bay, column) = bay.N;
        return;
    }

    *min_container_ref(bay, column) = find_min_container_in_column(bay, column);
}

void bay_pop_container(Bay bay, int column)
{
    assert(column >= 0 && column < bay.C);
    assert(containers_in_column(bay, column) > 0);
    int container = pop_container_from_column(bay, column);
    update_min_post_removal(bay, column, container);
    update_column_mask(bay, column);
    int first_affected_row = bay.R - containers_in_column(bay, column) - 1;
    reorder_bay(bay, first_affected_row);
}