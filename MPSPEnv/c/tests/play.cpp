#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/env.h"
#include "../src/play.h"
}

TEST_GROUP(play){};

TEST(play, dummy_strategy)
{
    int R = 4;
    int C = 3;
    int N = 6;
    int T_matrix[] = {
        0, 0, 0, 0, 3, 0,
        0, 0, 0, 0, 0, 9,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};
    Env env = get_specific_env(R, C, N, T_matrix, 0);
    int moves_to_solve = dummy_strategy(env);
    CHECK_EQUAL(13, moves_to_solve);
    CHECK_EQUAL(13, env.T->containers_placed);
    CHECK_EQUAL(0, env.T->containers_left);
    free_env(env);
}

TEST(play, dummy_strategy2)
{
    int R = 4;
    int C = 3;
    int N = 6;
    int T_matrix[] = {
        0, 0, 0, 0, 3, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 9,
        0, 0, 0, 0, 0, 0};
    // Not skipping last port
    Env env = get_specific_env(R, C, N, T_matrix, 0);
    int moves_to_solve = dummy_strategy(env);
    CHECK_EQUAL(12, moves_to_solve);
    CHECK_EQUAL(12, env.T->containers_placed);
    CHECK_EQUAL(0, env.T->containers_left);
    free_env(env);
    // Skipping last port
    env = get_specific_env(R, C, N, T_matrix, 1);
    moves_to_solve = dummy_strategy(env);
    CHECK_EQUAL(12, moves_to_solve);
    CHECK_EQUAL(3, env.T->containers_placed);
    CHECK_EQUAL(9, env.T->containers_left);
    free_env(env);
}