#include "env.h"
#include "bay.h"
#include "mask.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Env copy_env(Env env)
{
    Env copy;
    copy.T = copy_transportation_info(env.T);
    copy.bay = copy_bay(env.bay);
    copy.mask = copy_array(env.mask);
    copy.auto_move = env.auto_move;
    copy.total_reward = malloc(sizeof(int));
    *copy.total_reward = *env.total_reward;
    copy.containers_placed = malloc(sizeof(int));
    *copy.containers_placed = *env.containers_placed;
    copy.containers_left = malloc(sizeof(int));
    *copy.containers_left = *env.containers_left;
    copy.terminated = malloc(sizeof(int));
    *copy.terminated = *env.terminated;

    return copy;
}

void free_env(Env env)
{
    free_bay(env.bay);
    free_transportation_matrix(env.T);
    free_array(env.mask);
    free(env.total_reward);
    free(env.containers_placed);
    free(env.containers_left);
    free(env.terminated);
}

int get_add_reward(Env env, int column, int next_container, int n_containers)
{
    if (is_container_blocking(env.bay, column, next_container))
        return -n_containers;
    else
        return 0;
}

int get_remove_reward(Env env, int column, int n_containers)
{
    int reward = 0;
    for (int n = 0; n < n_containers; n++)
    {
        int row = env.bay.R - env.bay.column_counts.values[column] + n;
        int index = row * env.bay.C + column;
        int container = env.bay.matrix.values[index];
        int is_blocking = 0;
        for (int offset = row + 1; offset < env.bay.R; offset++)
        {
            int next_container = env.bay.matrix.values[offset * env.bay.C + column];
            if (next_container < container)
            {
                is_blocking = 1;
                break;
            }
        }
        if (!is_blocking)
            reward -= 1;
    }
    return reward;
}

void sail_and_reshuffle(Env env)
{
    while (no_containers_at_port(env.T) && !is_last_port(env.T))
    {
        transportation_sail_along(env.T);
        Array reshuffled = bay_sail_along(env.bay, &env);
        *env.containers_left += get_sum(reshuffled);
        transportation_insert_reshuffled(env.T, reshuffled);
        free_array(reshuffled);
    }
}

int add_container(Env env, int action)
{
    int column = (action / env.bay.R) % env.bay.C;
    int n_containers = action % env.bay.R + 1;
    int n_left_of_type = env.T->matrix.values[env.T->last_non_zero_column];
    int next_container = transportation_pop_n_containers(env.T, n_containers);
    int reward = get_add_reward(env, column, next_container, n_containers);
    bay_add_containers(env.bay, column, next_container, n_containers);
    *env.containers_placed += n_containers;
    *env.containers_left -= n_containers;

    sail_and_reshuffle(env);

    if (n_containers == n_left_of_type)
    {
        reset_right_most_added_column(env.bay);
    }

    return reward;
}

int remove_container(Env env, int action)
{
    int column = (action / env.bay.R) % env.bay.C;
    int n_containers = action % env.bay.R + 1;
    *env.containers_left += n_containers;
    int reward = get_remove_reward(env, column, n_containers);
    Array reshuffled = bay_pop_containers(env.bay, column, n_containers);
    transportation_insert_reshuffled(env.T, reshuffled);

    free_array(reshuffled);
    return reward;
}

int decide_is_terminated(Env env)
{
    return env.T->current_port >= env.T->N - 1;
}

// Returns reward
int step_action(Env env, int action)
{
    int is_adding_container = action < env.bay.C * env.bay.R;

    if (is_adding_container)
        return add_container(env, action);
    else
        return remove_container(env, action);
}

StepInfo env_step(Env env, int action)
{
    assert(action >= 0 && action < 2 * env.bay.C * env.bay.R);
    assert(env.mask.values[action] == 1);
    StepInfo step_info;
    step_info.reward = step_action(env, action);
    step_info.terminated = decide_is_terminated(env);
    *env.terminated = step_info.terminated;
    *env.total_reward += step_info.reward;

    int only_legal_action = insert_mask(env);
    if (env.auto_move && !step_info.terminated && only_legal_action != -1)
    {
        StepInfo next_step_info = env_step(env, only_legal_action);
        step_info.reward += next_step_info.reward;
        step_info.terminated = next_step_info.terminated;
    }

    return step_info;
}

Env build_env(int R, int C, int N, int auto_move, Transportation_Info *T)
{
    assert(R > 0 && C > 0 && N > 0);
    assert(auto_move == 0 || auto_move == 1);
    Env env;

    env.auto_move = auto_move;
    env.bay = get_bay(R, C, N);
    env.T = T;
    env.mask = get_zeros(2 * env.bay.R * env.bay.C);
    env.total_reward = malloc(sizeof(int));
    *env.total_reward = 0;
    env.containers_placed = malloc(sizeof(int));
    *env.containers_placed = 0;
    env.containers_left = malloc(sizeof(int));
    *env.containers_left = get_sum(T->matrix);
    env.terminated = malloc(sizeof(int));
    *env.terminated = 0;

    int only_legal_action = insert_mask(env);
    if (auto_move && only_legal_action != -1)
    {
        env_step(env, only_legal_action);
    }

    return env;
}

Env get_random_env(int R, int C, int N, int auto_move)
{
    Transportation_Info *T = get_random_transportation_matrix(N, R * C);
    return build_env(R, C, N, auto_move, T);
}

Env get_specific_env(int R, int C, int N, int *T_matrix, int auto_move)
{
    Transportation_Info *T = get_specific_transportation_matrix(N, T_matrix);
    return build_env(R, C, N, auto_move, T);
}
