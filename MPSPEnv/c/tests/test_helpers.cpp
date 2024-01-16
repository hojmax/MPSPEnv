#include "CppUTest/TestHarness.h"

extern "C"
{
#include "../src/array.h"
}

void test_array_full_equals(Array array, int value)
{
    for (int i = 0; i < array.n; i++)
    {
        CHECK_EQUAL(value, array.values[i]);
    }
}

void test_array_compare(Array array, int *expected)
{
    for (int i = 0; i < array.n; i++)
    {
        CHECK_EQUAL(expected[i], array.values[i]);
    }
}

void test_array_condition(Array array, int (*condition)(int))
{
    for (int i = 0; i < array.n; i++)
    {
        CHECK_TRUE(condition(array.values[i]));
    }
}