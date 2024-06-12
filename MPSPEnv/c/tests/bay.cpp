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