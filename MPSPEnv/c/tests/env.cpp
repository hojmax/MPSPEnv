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

TEST(env, blocking)
{
    int R = 4;
    int C = 3;
    int N = 6;
    int T_matrix[] = {
        0, 1, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 1,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};
    Env env = get_specific_env(R, C, N, T_matrix, 0);
    StepInfo step_info;
    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == 0);

    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == 0);

    step_info = step(env, 5);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == -1);

    step_info = step(env, 2);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == -1);

    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == 0);
    int expected_T_matrix[] = {
        0, 0, 1, 0, 0, 0,
        0, 0, 1, 1, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};
    int expected_bay_matrix[] = {
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0};
    test_array_compare(env.T->matrix, expected_T_matrix);
    test_array_compare(env.bay.matrix, expected_bay_matrix);

    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == 0);

    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    CHECK(step_info.reward == 0);

    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 1);
    CHECK(step_info.reward == 0);
    free_env(env);
}

TEST(env, skip_last_port1)
{
    int R = 4;
    int C = 3;
    int N = 6;
    int T_matrix[] = {
        0, 1, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 1,
        0, 0, 0, 0, 0, 5,
        0, 0, 0, 0, 0, 0};
    Env env = get_specific_env(R, C, N, T_matrix, 0);
    StepInfo step_info;
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 0);
    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 1);
    free_env(env);
}

TEST(env, skip_last_port2)
{
    int R = 4;
    int C = 3;
    int N = 6;
    int T_matrix[] = {
        0, 1, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 1,
        0, 0, 0, 0, 0, 5,
        0, 0, 0, 0, 0, 0};
    Env env = get_specific_env(R, C, N, T_matrix, 1);
    StepInfo step_info;
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    CHECK(step_info.is_terminal == 1);
    free_env(env);
}