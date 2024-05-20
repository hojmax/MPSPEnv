#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/bay.h"
#include "../src/array.h"
}

// Define test struct to avoid having to allocate, and write to memory for each test
typedef struct Expected_Bay
{
    int R;
    int C;
    int N;
    int *matrix;
    int *column_counts;
    int *min_container_per_column;
    int *mask;
} Expected_Bay;

TEST_GROUP(bay){};

void check_bay_equals_expectation(
    Bay bay,
    Expected_Bay expected_bay)
{
    CHECK_EQUAL(bay.R, expected_bay.R);
    CHECK_EQUAL(bay.C, expected_bay.C);
    CHECK_EQUAL(bay.N, expected_bay.N);
    test_array_compare(bay.matrix, expected_bay.matrix);
    test_array_compare(bay.column_counts, expected_bay.column_counts);
    test_array_compare(bay.min_container_per_column, expected_bay.min_container_per_column);
}

TEST(bay, get_bay)
{
    int R = 3;
    int C = 5;
    int N = 4;
    int expected_matrix[] = {
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0};
    int expected_column_counts[] = {0, 0, 0, 0, 0};
    int expected_min_container_per_column[] = {4, 4, 4, 4, 4};
    int expected_mask[] = {
        1, 1, 1, 1, 1,
        0, 0, 0, 0, 0};

    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};

    Bay bay = get_bay(R, C, N);
    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, sort_bay)
{
    int R = 8;
    int C = 8;
    int N = 8;

    Bay bay = get_bay(R, C, N);

    bay_add_containers(bay, 7, 7, 2);
    bay_add_containers(bay, 6, 7, 2);
    bay_add_containers(bay, 5, 7, 2);
    bay_add_containers(bay, 4, 7, 2);
    bay_add_containers(bay, 3, 7, 2);
    bay_add_containers(bay, 2, 6, 2);
    bay_add_containers(bay, 1, 5, 2);

    *bay.right_most_added_column = C;
    bay_add_containers(bay, 1, 4, 3);

    int expected_values[] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        4,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        4,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        4,
        0,
        6,
        7,
        7,
        7,
        7,
        7,
        5,
        0,
        6,
        7,
        7,
        7,
        7,
        7,
        5,
    };

    test_array_compare(bay.matrix, expected_values);

    free_bay(bay);
}

TEST(bay, sort_bay2)
{
    int R = 6;
    int C = 6;
    int N = 6;

    Bay bay = get_bay(R, C, N);

    int bay_values[] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        5,
        0,
        0,
        0,
        0,
        0,
        5,
        0,
        3,
        0,
        0,
        5,
        5,
        0,
        3,
        3,
        3,
        5,
        5,
        0,
        5,
        5,
        5,
        5,
        5,
    };

    // set all the values of bay.matrix to bay_values
    for (int i = 0; i < R * C; i++)
    {
        bay.matrix.values[i] = bay_values[i];
    }

    int column_counts_values[] = {0, 3, 2, 2, 3, 4, 5};
    int min_container_per_column_values[] = {
        6,
        3,
        3,
        3,
        5,
        5};
    for (int i = 0; i < C; i++)
    {
        bay.column_counts.values[i] = column_counts_values[i];
        bay.min_container_per_column.values[i] = min_container_per_column_values[i];
    }

    bay_add_containers(bay, 0, 5, 1);

    int expected_matrix[] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        5,
        0,
        0,
        0,
        0,
        0,
        5,
        0,
        0,
        0,
        3,
        5,
        5,
        0,
        3,
        3,
        3,
        5,
        5,
        5,
        5,
        5,
        5,
        5,
        5,
    };

    test_array_compare(bay.matrix, expected_matrix);

    free_bay(bay);
}