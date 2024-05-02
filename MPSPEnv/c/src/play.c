#include "play.h"
#include <assert.h>
#include <stdio.h>

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
    StepInfo step_info;
    int action = get_first_add_action(env.bay.mask);
    step_info = step(env, action);

    while (!step_info.is_terminal)
    {
        action = get_first_add_action(env.bay.mask);
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
