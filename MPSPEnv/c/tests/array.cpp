#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/random.h"
#include "../src/array.h"
}

TEST_GROUP(array){};

TEST(array, get_zeros)
{
    int n = 5;
    Array array = get_zeros(n);
    CHECK_EQUAL(n, array.n);
    test_array_full_equals(array, 0);
    free_array(array);
}

TEST(array, get_full)
{
    int n = 5;
    int value = 1;
    Array array = get_full(n, value);
    CHECK_EQUAL(n, array.n);
    test_array_full_equals(array, value);
    free_array(array);
}

TEST(array, get_range)
{
    int expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    Array array = get_range(0, 10);

    test_array_compare(array, expected);

    free_array(array);
}

TEST(array, shift_array_left)
{
    int expected[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 0};
    int a = 1;
    int b = 11;
    int n = b - a;
    Array array = get_range(a, b);

    shift_array_left(array, 1);

    CHECK_EQUAL(n, array.n);

    test_array_compare(array, expected);
    free_array(array);
}