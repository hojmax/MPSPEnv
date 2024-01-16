#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/sort.h"
#include "../src/array.h"
}

TEST_GROUP(sort){};

TEST(sort, sort_indexes_using_values)
{
    int n = 5;
    Array indeces = get_range(0, n);
    int ints[] = {1, 4, 5, 2, 1};
    int expected_indeces[] = {0, 4, 3, 1, 2};
    Array values = array_from_ints(ints, n);

    sort_indexes_using_values(indeces, values);
    test_array_compare(indeces, expected_indeces);

    free_array(indeces);
    free_array(values);
}

TEST(sort, sort_indexes_using_values2)
{
    int n = 5;
    Array indeces = get_range(0, n);
    int ints[] = {2, 2, 2, 2, 2};
    int expected_indeces[] = {0, 1, 2, 3, 4};
    Array values = array_from_ints(ints, n);

    sort_indexes_using_values(indeces, values);
    test_array_compare(indeces, expected_indeces);

    free_array(indeces);
    free_array(values);
}