#include "env.h"
#include "bay.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void insert_flat_T_matrix(Env env)
{
    int index = 0;

    // Upper Triangular Indeces:
    // i in [0, N)
    // j in [i + 1, N]
    for (int i = 0; i < env.T->N - 1; i++)
    {
        for (int j = i + 1; j < env.T->N; j++)
        {
            env.flat_T_matrix.values[index] = env.T->matrix.values[i * env.T->N + j];
            index++;
        }
    }
}

void update_mask_entry(Env env, int i)
{
    // Action space is column major order:
    // [c0r1, c0r2, c0r3, …, c0rR, c1r1, c1r2, …, cC-1rR]
    int is_add = i < env.bay.C * env.bay.R;
    int column = (i / env.bay.R) % env.bay.C;
    int n_containers = i % env.bay.R + 1;
    int n_to_place = env.T->matrix.values[env.T->last_non_zero_column];

    if (is_add)
        env.mask.values[i] = (n_to_place >= n_containers) && (n_containers + containers_in_column(env.bay, column) <= env.bay.R);
    else
        env.mask.values[i] = !env.bay.added_since_sailing.values[column] && (containers_in_column(env.bay, column) - n_containers >= 0);
}

void insert_mask(Env env)
{
    for (int i = 0; i < env.mask.n; i++)
    {
        update_mask_entry(env, i);
    }
}

void initialize_mask(Env *env)
{
    env->mask = get_zeros(2 * env->bay.R * env->bay.C);
    insert_mask(*env);
}

void initialize_flat_T(Env *env, int N)
{
    int upper_triangle_size = (N * (N - 1)) / 2;
    env->flat_T_matrix = get_zeros(upper_triangle_size);
}

Env build_env(int R, int C, int N, int skip_last_port, int should_reorder, Transportation_Info *T)
{
    assert(R > 0 && C > 0 && N > 0);
    assert(skip_last_port == 0 || skip_last_port == 1);
    Env env;

    env.skip_last_port = skip_last_port;
    env.should_reorder = should_reorder;
    env.bay = get_bay(R, C, N);
    env.T = T;
    initialize_flat_T(&env, N);
    initialize_mask(&env);
    insert_flat_T_matrix(env);
    return env;
}

Env get_random_env(int R, int C, int N, int skip_last_port, int should_reorder)
{
    Transportation_Info *T = get_random_transportation_matrix(N, R * C);
    return build_env(R, C, N, skip_last_port, should_reorder, T);
}

Env get_specific_env(int R, int C, int N, int *T_matrix, int skip_last_port, int should_reorder)
{
    Transportation_Info *T = get_specific_transportation_matrix(N, T_matrix);
    return build_env(R, C, N, skip_last_port, should_reorder, T);
}

Env copy_env(Env env)
{
    Env copy;
    copy.bay = copy_bay(env.bay);
    copy.T = copy_transportation_info(env.T);
    copy.skip_last_port = env.skip_last_port;
    copy.flat_T_matrix = copy_array(env.flat_T_matrix);
    copy.should_reorder = env.should_reorder;
    copy.mask = copy_array(env.mask);

    return copy;
}

void free_env(Env env)
{
    free_bay(env.bay);
    free_transportation_matrix(env.T);
    free_array(env.flat_T_matrix);
    free_array(env.mask);
}

int get_add_reward(Env env, int column, int next_container)
{
    if (is_container_blocking(env.bay, column, next_container))
        return -1;
    else
        return 0;
}

int get_remove_reward(Env env, Array reshuffled)
{
    return -1;
}

void handle_sailing(Env env)
{
    while (no_containers_at_port(env.T) && !is_last_port(env.T))
    {
        transportation_sail_along(env.T);
        Array reshuffled = bay_sail_along(env.bay, &env);
        transportation_insert_reshuffled(env.T, reshuffled);
        free_array(reshuffled);
    }
}

int add_container(Env env, int action)
{
    int column = (action / env.bay.R) % env.bay.C;
    int n_containers = action % env.bay.R + 1;

    int next_container = transportation_pop_n_containers(env.T, n_containers);
    int reward = get_add_reward(env, column, next_container);
    bay_add_containers(env.bay, column, next_container, n_containers, env.should_reorder);

    handle_sailing(env);

    return reward;
}

int remove_container(Env env, int action)
{
    int column = (action / env.bay.R) % env.bay.C;
    int n_containers = action % env.bay.R + 1;
    Array reshuffled = bay_pop_containers(env.bay, column, n_containers, env.should_reorder);
    transportation_insert_reshuffled(env.T, reshuffled);
    int reward = get_remove_reward(env, reshuffled);

    free_array(reshuffled);
    return reward;
}

void decide_is_terminated(StepInfo *step_info, Env env)
{
    if (env.skip_last_port)
        step_info->is_terminal = env.T->current_port >= env.T->N - 2;
    else
        step_info->is_terminal = env.T->current_port >= env.T->N - 1;
}

void step_action(StepInfo *step_info, int action, Env env)
{
    int is_adding_container = action < env.bay.C * env.bay.R;

    if (is_adding_container)
        step_info->reward = add_container(env, action);
    else
        step_info->reward = remove_container(env, action);
}

StepInfo step(Env env, int action)
{
    assert(action >= 0 && action < 2 * env.bay.C * env.bay.R);
    assert(env.mask.values[action] == 1);
    StepInfo step_info;
    step_action(&step_info, action, env);
    decide_is_terminated(&step_info, env);

    insert_flat_T_matrix(env);
    insert_mask(env);

    return step_info;
}