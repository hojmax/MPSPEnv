#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/random.h"
#include "../src/transportation_matrix.h"
#include "../src/array.h"
}

TEST_GROUP(transportation_matrix){};

int geq_zero(int x)
{
    return x >= 0;
}

TEST(transportation_matrix, non_negative_matrix)
{
    int matrices_checked = 100;
    int N = 10;
    int bay_capacity = 40;

    for (int i = 0; i < matrices_checked; i++)
    {
        Transportation_Info *T = get_random_transportation_matrix(N, bay_capacity);
        test_array_condition(T->matrix, geq_zero);
        CHECK(T->containers_per_port.values[0] == bay_capacity);
        free_transportation_matrix(T);
    }
}

void check_is_feasble(Transportation_Info *T, int capacity)
{
    for (int i = 0; i < T->N - 1; i++)
    {
        int sum = 0;
        for (int k = 0; k <= i; k++)
        {
            for (int j = i + 1; j < T->N; j++)
            {
                sum += T->matrix.values[k * T->N + j];
            }
        }
        CHECK(sum <= capacity);
    }
}

TEST(transportation_matrix, feasibility)
{
    int matrices_checked = 100;
    int N = 10;
    int bay_capacity = 40;

    for (int i = 0; i < matrices_checked; i++)
    {
        Transportation_Info *T = get_random_transportation_matrix(N, bay_capacity);
        check_is_feasble(T, bay_capacity);
        free_transportation_matrix(T);
    }
}