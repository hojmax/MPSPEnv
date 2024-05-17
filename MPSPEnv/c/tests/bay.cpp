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
    test_array_compare(bay.mask, expected_bay.mask);
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

TEST(bay, add_container)
{
    int R = 3;
    int C = 2;
    int N = 4;
    Bay bay = get_bay(R, C, N);
    int container = 1;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    int expected_column_counts[] = {0, 1};
    int expected_matrix[] = {0, 0, 0, 0, 0, 1};
    int expected_min_container_per_column[] = {4, 1};
    int expected_mask[] = {
        1, 1,
        0, 0};
    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};
    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, bay_add_containers2)
{
    int R = 3;
    int C = 2;
    int N = 4;
    Bay bay = get_bay(R, C, N);
    int container = 1;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    int expected_column_counts[] = {1, 1};
    int expected_matrix[] = {0, 0, 0, 0, 1, 1};
    int expected_min_container_per_column[] = {1, 1};
    int expected_mask[] = {
        1, 1,
        0, 0};
    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};
    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

void fake_callback(int a, int b, Env *env)
{
}

TEST(bay, bay_sail_along)
{
    int R = 3;
    int C = 2;
    int N = 4;
    Bay bay = get_bay(R, C, N);
    int container = 1;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    Array reshuffled = bay_sail_along(bay, fake_callback, NULL);
    int expected_reshuffled[] = {0, 0, 0, 0};
    test_array_compare(reshuffled, expected_reshuffled);
    int expected_column_counts[] = {0, 0};
    int expected_matrix[] = {0, 0, 0, 0, 0, 0};
    int expected_min_container_per_column[] = {4, 4};
    int expected_mask[] = {
        1, 1, 0, 0};
    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};
    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
    free_array(reshuffled);
}

TEST(bay, bay_sail_along2)
{
    int R = 3;
    int C = 2;
    int N = 4;
    Bay bay = get_bay(R, C, N);
    int container = 1;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    container = 2;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    column = 1;
    container = 1;
    bay_add_containers(bay, column, container, 1, 1);
    Array reshuffled = bay_sail_along(bay, fake_callback, NULL);
    int expected_reshuffled[] = {0, 2, 0, 0};
    test_array_compare(reshuffled, expected_reshuffled);
    int expected_column_counts[] = {0, 1};
    int expected_matrix[] = {0, 0, 0, 0, 0, 1};
    int expected_min_container_per_column[] = {4, 1};
    int expected_mask[] = {
        1, 1, 0, 1};
    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};
    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
    free_array(reshuffled);
}

TEST(bay, bay_add_containers3)
{
    int R = 3;
    int C = 2;
    int N = 4;
    Bay bay = get_bay(R, C, N);
    int container = 1;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    container = 2;
    column = 1;
    bay_add_containers(bay, column, container, 1, 1);
    container = 3;
    column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    int expected_column_counts[] = {2, 2};
    int expected_matrix[] = {0, 0, 2, 3, 1, 1};
    int expected_min_container_per_column[] = {1, 1};
    int expected_mask[] = {
        1, 1,
        0, 0};

    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};
    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, bay_pop_containers)
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
    int container = 1;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    column = 4;
    bay_pop_containers(bay, column, 1, 1);

    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, bay_pop_containers2)
{
    int R = 3;
    int C = 5;
    int N = 4;
    int expected_matrix[] = {
        0, 0, 0, 0, 1,
        0, 0, 0, 0, 3,
        0, 0, 0, 2, 2};
    int expected_column_counts[] = {0, 0, 0, 1, 3};
    int expected_min_container_per_column[] = {4, 4, 4, 2, 1};
    int expected_mask[] = {
        1, 1, 1, 1, 0,
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
    int container = 2;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    column = 3;
    container = 3;
    bay_add_containers(bay, column, container, 1, 1);
    column = 4;
    container = 1;
    bay_add_containers(bay, column, container, 1, 1);
    bay_pop_containers(bay, column, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);

    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, bay_sail_along_remove)
{
    int R = 3;
    int C = 2;
    int N = 4;
    Bay bay = get_bay(R, C, N);
    int container = 2;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    container = 1;
    column = 0;
    bay_add_containers(bay, column, container, 1, 1);

    int expected_mask3[] = {
        1, 1, 0, 0};
    test_array_compare(bay.mask, expected_mask3);

    Array reshuffled = bay_sail_along(bay, fake_callback, NULL);
    int expected_reshuffled[] = {0, 0, 0, 0};
    test_array_compare(reshuffled, expected_reshuffled);
    int expected_column_counts[] = {0, 1};
    int expected_matrix[] = {0, 0, 0, 0, 0, 1};
    int expected_min_container_per_column[] = {4, 1};
    int expected_mask[] = {
        1, 1, 0, 1};
    Expected_Bay expected_bay = {
        .R = R,
        .C = C,
        .N = N,
        .matrix = expected_matrix,
        .column_counts = expected_column_counts,
        .min_container_per_column = expected_min_container_per_column,
        .mask = expected_mask};
    check_bay_equals_expectation(bay, expected_bay);

    int expected_mask2[] = {
        1, 1, 0, 0};
    container = 1;
    column = 1;
    bay_add_containers(bay, column, container, 1, 1);
    test_array_compare(bay.mask, expected_mask2);

    free_bay(bay);
    free_array(reshuffled);
}

TEST(bay, bay_pop_containers3)
{
    int R = 3;
    int C = 5;
    int N = 4;
    int expected_matrix[] = {
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 2, 2};
    int expected_column_counts[] = {0, 0, 0, 1, 1};
    int expected_min_container_per_column[] = {4, 4, 4, 2, 2};
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
    int container = 2;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    column = 3;
    container = 3;
    bay_add_containers(bay, column, container, 1, 1);
    column = 4;
    container = 1;

    bay_add_containers(bay, column, container, 1, 1);

    bay_pop_containers(bay, column, 2, 1);

    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, bay_pop_containers4)
{
    int R = 3;
    int C = 5;
    int N = 4;
    int expected_matrix[] = {
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 2};
    int expected_column_counts[] = {0, 0, 0, 0, 1};
    int expected_min_container_per_column[] = {4, 4, 4, 4, 2};
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
    int container = 2;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    column = 3;
    container = 3;
    bay_add_containers(bay, column, container, 1, 1);
    column = 4;
    container = 1;

    bay_add_containers(bay, column, container, 1, 1);

    bay_pop_containers(bay, column, 3, 1);

    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}

TEST(bay, bay_pop_containers5)
{
    int R = 3;
    int C = 5;
    int N = 4;
    int expected_matrix[] = {
        0, 0, 0, 1, 0,
        0, 0, 0, 1, 0,
        0, 0, 0, 1, 2};
    int expected_column_counts[] = {0, 0, 0, 3, 1};
    int expected_min_container_per_column[] = {4, 4, 4, 1, 2};
    int expected_mask[] = {
        1, 1, 1, 0, 1,
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
    int container = 2;
    int column = 0;
    bay_add_containers(bay, column, container, 1, 1);
    bay_add_containers(bay, column, container, 1, 1);
    column = 3;
    container = 3;
    bay_add_containers(bay, column, container, 1, 1);
    column = 4;
    container = 1;

    bay_add_containers(bay, column, container, 1, 1);
    bay_pop_containers(bay, column, 3, 1);

    column = 3;
    bay_add_containers(bay, column, container, 3, 1);

    check_bay_equals_expectation(bay, expected_bay);
    free_bay(bay);
}