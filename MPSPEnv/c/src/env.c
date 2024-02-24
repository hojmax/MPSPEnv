#include "env.h"
#include <assert.h>

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

Env get_random_env(int R, int C, int N, int skip_last_port)
{
    assert(R > 0 && C > 0 && N > 0);
    assert(skip_last_port == 0 || skip_last_port == 1);
    Env env;

    env.bay = get_bay(R, C, N);
    env.T = get_random_transportation_matrix(N, R * C);
    env.skip_last_port = skip_last_port;

    int upper_triangle_size = (N * (N - 1)) / 2;
    env.flat_T_matrix = get_zeros(upper_triangle_size);
    env.one_hot_bay = get_zeros((N - 1) * R * C);
    insert_flat_T_matrix(env);

    return env;
}

Env get_specific_env(int R, int C, int N, int *T_matrix, int skip_last_port)
{
    assert(R > 0 && C > 0 && N > 0);
    assert(skip_last_port == 0 || skip_last_port == 1);
    Env env;

    env.bay = get_bay(R, C, N);
    env.T = get_specific_transportation_matrix(N, T_matrix);
    env.skip_last_port = skip_last_port;

    int upper_triangle_size = (N * (N - 1)) / 2;
    env.flat_T_matrix = get_zeros(upper_triangle_size);
    env.one_hot_bay = get_zeros((N - 1) * R * C);
    insert_flat_T_matrix(env);

    return env;
}

Env copy_env(Env env)
{
    Env copy;
    copy.bay = copy_bay(env.bay);
    copy.T = copy_transportation_info(env.T);
    copy.skip_last_port = env.skip_last_port;
    copy.flat_T_matrix = copy_array(env.flat_T_matrix);
    copy.one_hot_bay = copy_array(env.one_hot_bay);
    return copy;
}

void insert_one_hot_bay(Env env)
{
    int index = 0;

    for (int k = 0; k < env.bay.N - 1; k++)
    {
        for (int i = 0; i < env.bay.R; i++)
        {
            for (int j = 0; j < env.bay.C; j++)
            {
                int container = env.bay.matrix.values[i * env.bay.C + j];
                env.one_hot_bay.values[index] = container == k + 1;
                index++;
            }
        }
    }
}

void free_env(Env env)
{
    free_bay(env.bay);
    free_transportation_matrix(env.T);
    free_array(env.flat_T_matrix);
    free_array(env.one_hot_bay);
}

int get_add_reward(Env env, int column, int next_container)
{
    if (is_container_blocking(env.bay, column, next_container))
        return -1;
    else
        return 0;
}

// Penalize removing non-blocking containers
// But not removing blocking containers
// Since we already penalized blocking containers when we added them
int get_remove_reward(Env env, int column, int top_container)
{
    if (is_container_blocking(env.bay, column, top_container))
        return 0;
    else
        return -1;
}

void handle_sailing(Env env)
{
    while (no_containers_at_port(env.T) && !is_last_port(env.T))
    {
        transportation_sail_along(env.T);
        Array reshuffled = bay_sail_along(env.bay);
        transportation_insert_reshuffled(env.T, reshuffled);
        free_array(reshuffled);
    }
}

int add_container(Env env, int column)
{
    int next_container = transportation_pop_container(env.T);
    int reward = get_add_reward(env, column, next_container);
    bay_add_container(env.bay, column, next_container);

    handle_sailing(env);

    return reward;
}

int remove_container(Env env, int column)
{
    int top_container = get_top_container(env.bay, column);
    int reward = get_remove_reward(env, column, top_container);
    bay_pop_container(env.bay, column);
    transportation_insert_container(env.T, top_container);
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
    int is_adding_container = action < env.bay.C;
    if (is_adding_container)
        step_info->reward = add_container(env, action);
    else
    {
        int column = action - env.bay.C;
        step_info->reward = remove_container(env, column);
    }
}

// Action is in range [0, 2 * C)
// Action < C: Add container to column
// Action >= C: Remove container from column
StepInfo step(Env env, int action)
{
    assert(action >= 0 && action < 2 * env.bay.C);
    assert(env.bay.mask.values[action] == 1);

    StepInfo step_info;
    step_action(&step_info, action, env);
    decide_is_terminated(&step_info, env);

    insert_flat_T_matrix(env);
    insert_one_hot_bay(env);

    return step_info;
}