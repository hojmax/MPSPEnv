#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/env.h"
#include "../src/random.h"
}

TEST_GROUP(env){};

void print_env(Env env, StepInfo step_info)
{
    printf("\nB:\n");
    print_matrix(env.bay.matrix, env.bay.C, env.bay.R);
    printf("T:\n");
    print_matrix(env.T->matrix, env.bay.N, env.bay.N);
    printf("is_terminal: %d\n", step_info.is_terminal);
    printf("reward: %d\n", step_info.reward);
}

TEST(env, mask)
{
    int R = 4;
    int C = 3;
    int N = 6;
    int T_matrix[] = {
        0, 1, 1, 0, 3, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 2,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};
    Env env = get_specific_env(R, C, N, T_matrix, 0, 1);
    StepInfo step_info;

    CHECK_EQUAL(env.T->containers_left, 8);

    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, 0);
    CHECK_EQUAL(env.T->containers_left, 7);
    CHECK_EQUAL(env.T->containers_placed, 1);

    int expected_mask[] = {1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    test_array_compare(env.mask, expected_mask);

    step_info = step(env, 8);

    int expected_mask2[] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    test_array_compare(env.mask, expected_mask2);

    step_info = step(env, 8);
    step_info = step(env, 4);
    step_info = step(env, 0);

    int expected_mask3[] = {1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};
    test_array_compare(env.mask, expected_mask3);

    step_info = step(env, 0);

    int expected_mask4[] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0};
    test_array_compare(env.mask, expected_mask4);

    free_env(env);
}
