#ifndef TEST_HELPERS_INCLUDED
#define TEST_HELPERS_INCLUDED

extern "C"
{
#include "../src/array.h"
}

void test_array_equals(Array array1, Array array2);

void test_array_full_equals(Array array, int value);

void test_array_compare(Array array, int *expected);

void test_array_condition(Array array, int (*condition)(int));

#endif