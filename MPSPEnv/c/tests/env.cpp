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

void print_history(Env env)
{
    for (int slice = 0; slice < env.bay.C * env.bay.R * (env.bay.N - 1); slice++)
    {
        for (int i = 0; i < env.bay.R; i++)
        {
            for (int j = 0; j < env.bay.C; j++)
            {
                printf("%d ", env.history[slice * env.bay.C * env.bay.R + i * env.bay.C + j]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void history_checker(int history_index, char *expected_history, Env env)
{
    for (int j = 0; j < env.bay.C; j++)
    {
        for (int i = 0; i < env.bay.R; i++)
        {
            int index = history_index * env.bay.R * env.bay.C + i * env.bay.C + j;
            CHECK(expected_history[i * env.bay.C + j] == env.history[index]);
        }
    }
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
    Env env = get_specific_env(R, C, N, T_matrix, 0, 1, 1);
    StepInfo step_info;

    CHECK_EQUAL(env.T->containers_left, 5);

    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, 0);
    CHECK_EQUAL(env.T->containers_left, 4);
    CHECK_EQUAL(env.T->containers_placed, 1);

    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, 0);
    CHECK_EQUAL(env.T->containers_left, 3);
    CHECK_EQUAL(env.T->containers_placed, 2);

    step_info = step(env, 5);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, -1);
    CHECK_EQUAL(env.T->containers_left, 4);
    CHECK_EQUAL(env.T->containers_placed, 2);

    step_info = step(env, 2);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, -1);
    CHECK_EQUAL(env.T->containers_left, 3);

    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, 0);
    CHECK_EQUAL(env.T->containers_left, 3);

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
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, 0);

    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    CHECK_EQUAL(step_info.reward, 0);

    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 1);
    CHECK_EQUAL(step_info.reward, 0);
    CHECK_EQUAL(env.T->containers_left, 0);
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
    Env env = get_specific_env(R, C, N, T_matrix, 0, 1, 1);
    StepInfo step_info;
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 0);
    step_info = step(env, 0);
    CHECK_EQUAL(step_info.is_terminal, 1);
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
    Env env = get_specific_env(R, C, N, T_matrix, 1, 1, 1);

    StepInfo step_info;
    CHECK_EQUAL(env.T->current_port, 0);
    step_info = step(env, 0);
    CHECK_EQUAL(env.T->current_port, 0);
    step_info = step(env, 0);
    CHECK_EQUAL(env.T->current_port, 0);
    step_info = step(env, 0);
    CHECK_EQUAL(env.T->current_port, 3);
    step_info = step(env, 0);
    CHECK_EQUAL(env.T->current_port, 3);
    step_info = step(env, 0);
    CHECK_EQUAL(env.T->current_port, 4);
    CHECK_EQUAL(step_info.is_terminal, 1);
    CHECK_EQUAL(env.T->containers_left, 5);
    free_env(env);
}

TEST(env, copy_env)
{
    Env env = get_random_env(3, 2, 4, 0, 1, 1);
    step(env, 0);
    Env copy = copy_env(env, 1);

    test_array_equals(env.bay.matrix, copy.bay.matrix);
    test_array_equals(env.bay.min_container_per_column, copy.bay.min_container_per_column);
    test_array_equals(env.bay.column_counts, copy.bay.column_counts);
    test_array_equals(env.bay.mask, copy.bay.mask);
    test_array_equals(env.T->matrix, copy.T->matrix);
    test_array_equals(env.flat_T_matrix, copy.flat_T_matrix);
    test_array_equals(env.one_hot_bay, copy.one_hot_bay);
    test_array_equals(env.T->containers_per_port, copy.T->containers_per_port);
    CHECK(env.T->seed == copy.T->seed);
    CHECK(env.T->last_non_zero_column == copy.T->last_non_zero_column);
    CHECK(env.T->current_port == copy.T->current_port);
    CHECK(env.T->containers_placed == copy.T->containers_placed);
    CHECK(env.T->containers_left == copy.T->containers_left);
    CHECK(env.T->N == copy.T->N);
    CHECK(env.skip_last_port == copy.skip_last_port);
    CHECK(env.bay.C == copy.bay.C);
    CHECK(env.bay.R == copy.bay.R);
    CHECK(env.bay.N == copy.bay.N);
    CHECK(env.should_reorder == copy.should_reorder);
    STRCMP_EQUAL(env.history, copy.history);
    CHECK(env.history_index[0] == copy.history_index[0]);

    free_env(env);
    free_env(copy);
}

TEST(env, history)
{
    set_seed(10);
    Env env = get_random_env(3, 2, 4, 0, 1, 1);
    CHECK_EQUAL(0, env.history_index[0]);
    StepInfo step_info = step(env, 0);
    CHECK_EQUAL(1, env.history_index[0]);

    char expected_history0[] = {
        0,
        0,
        0,
        0,
        0,
        0,
    };
    char expected_history1[] = {
        0,
        0,
        0,
        0,
        0,
        2,
    };
    history_checker(0, expected_history0, env);
    history_checker(1, expected_history1, env);
    step_info = step(env, 3);
    CHECK_EQUAL(2, env.history_index[0]);
    expected_history1[5] = 1;
    char expected_history1_updated[] = {
        0,
        0,
        0,
        0,
        0,
        1,
    };
    history_checker(1, expected_history1_updated, env);
    step_info = step(env, 0);
    step_info = step(env, 1);
    step_info = step(env, 1);
    step_info = step(env, 0);
    step_info = step(env, 3);
    step_info = step(env, 0);
    char expected_history3[] = {
        0,
        0,
        0,
        0,
        0,
        2,
    };
    history_checker(3, expected_history3, env);
    char expected_history4[] = {
        0,
        0,
        0,
        2,
        0,
        2,
    };
    history_checker(4, expected_history4, env);
    char expected_history5[] = {
        0,
        1,
        0,
        2,
        0,
        2,
    };
    history_checker(5, expected_history5, env);
    char expected_history6[] = {
        0,
        1,
        0,
        2,
        2,
        2,
    };
    history_checker(6, expected_history6, env);
    char expected_history7[] = {
        0,
        0,
        0,
        2,
        2,
        2,
    };
    history_checker(7, expected_history7, env);
    char expected_history8[] = {
        0,
        0,
        2,
        2,
        2,
        2,
    };
    history_checker(8, expected_history8, env);
    step_info = step(env, 0);
    step_info = step(env, 1);
    step_info = step(env, 1);
    char expected_history8_new[] = {
        0,
        0,
        1,
        2,
        2,
        2,
    };
    history_checker(8, expected_history8_new, env);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    step_info = step(env, 0);
    char expected[] = {0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 1,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 1,
                       0, 0,
                       0, 0,

                       0, 1,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       1, 0,
                       0, 0,

                       0, 0,
                       1, 0,
                       0, 0,

                       0, 0,
                       1, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0,

                       0, 0,
                       0, 0,
                       0, 0};
    for (int i = 0; i < 18; i++)
    {
        history_checker(i, expected + i * 6, env);
    }
    free_env(env);
}