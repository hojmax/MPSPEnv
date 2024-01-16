#include "CppUTest/TestHarness.h"

extern "C"
{
#include "../src/random.h"
}

TEST_GROUP(random){};

TEST(random, random_uniform_int)
{
    int iterations = 100;
    int min = 0;
    int max = 100;
    for (int i = 0; i < iterations; i++)
    {
        int value = random_uniform_int(min, max);
        CHECK(value >= min);
        CHECK(value < max);
    }
}