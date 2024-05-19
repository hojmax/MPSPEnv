#include "../src/env.h"
#include "../src/array.h"
#include "../src/random.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int get_first_add_action(Array mask)
{
    for (int i = mask.n / 2 - 1; i >= 0; i--)
    {
        if (mask.values[i] == 1)
            return i;
    }
    assert(0);
}

int dummy_strategy(Env env)
{
    StepInfo step_info = {0, 0};

    while (!step_info.is_terminal)
    {
        int action = get_first_add_action(env.mask);
        step_info = step(env, action);
    }

    return env.T->containers_placed + env.T->containers_left;
}

int get_moves_upper_bound(Env env)
{
    Env copy = copy_env(env);
    copy.skip_last_port = 1;
    int moves_upper_bound = dummy_strategy(copy);
    free_env(copy);
    return moves_upper_bound;
}

int calculate_stats(int R, int C, int N, int repeats)
{
    int max_moves = 0;
    float average_moves = 0;
    int min_moves = 1000;

    for (int i = 0; i < repeats; i++)
    {
        Env env = get_random_env(R, C, N, 1, 0);
        int moves = get_moves_upper_bound(env);

        if (moves > max_moves)
            max_moves = moves;

        if (moves < min_moves)
            min_moves = moves;

        average_moves += moves;

        free_env(env);
    }

    average_moves /= repeats;
    return max_moves;
}

int main()
{
    set_seed(0);
    for (int R = 6; R < 12 + 1; R += 2)
    {
        for (int C = 2; C < 12 + 1; C += 2)
        {
            for (int N = 4; N < 16 + 1; N += 2)
            {
                int max_moves = calculate_stats(R, C, N, 10000);
                printf("%d,%d,%d,%d\n", R, C, N, max_moves);
            }
        }
    }
}