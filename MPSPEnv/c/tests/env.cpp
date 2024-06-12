#include "CppUTest/TestHarness.h"
#include "test_helpers.h"

extern "C"
{
#include "../src/env.h"
}

TEST_GROUP(env){};

TEST(env, env)
{
    int R = 5;
    int C = 5;
    int N = 5;
    int auto_move = 0;
    Env env = get_random_env(R, C, N, auto_move);
    CHECK_EQUAL(R, env.bay.R);
    CHECK_EQUAL(C, env.bay.C);
    CHECK_EQUAL(N, env.bay.N);
    for (int i = 0; i < env.mask.n; i++)
    {
        if (env.mask.values[i])
        {
            env_step(env, i);
            break;
        }
    }
    free_env(env);
}