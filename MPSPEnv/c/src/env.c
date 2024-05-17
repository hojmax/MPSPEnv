#include "env.h"
#include "play.h"
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

void initialize_history(Env *env, int track_history)
{
    env->history_index = (int *)malloc(sizeof(int));
    if (track_history)
    {
        env->history = (char *)calloc(env->bay.R * env->bay.C * env->bay.R * env->bay.C * (env->bay.N - 1), sizeof(char));
        env->history_index[0] = 0;
    }
    else
    {
        env->history = NULL;
        env->history_index[0] = -1;
    }
}

void update_extended_mask_entry(Env env, int i)
{
    // Action space is column major order:
    // [c0r1, c0r2, c0r3, …, c0rR, c1r1, c1r2, …, cC-1rR]
    int is_add = i < env.bay.C * env.bay.R;
    int column = (i / env.bay.R) % env.bay.C;
    int n_containers = i % env.bay.R + 1;
    int n_to_place = env.T->matrix.values[env.T->last_non_zero_column];

    if (is_add)
        env.extended_mask.values[i] = (n_to_place >= n_containers) && (n_containers + containers_in_column(env.bay, column) <= env.bay.R);
    else
        env.extended_mask.values[i] = !env.bay.added_since_sailing.values[column] && (containers_in_column(env.bay, column) - n_containers >= 0);
}

void insert_extended_mask(Env env)
{
    for (int i = 0; i < env.extended_mask.n; i++)
    {
        update_extended_mask_entry(env, i);
    }
}

void initialize_extended_mask(Env *env)
{
    if (env->is_extended)
    {
        env->extended_mask = get_zeros(2 * env->bay.R * env->bay.C);
        insert_extended_mask(*env);
    }
    else
    {
        env->extended_mask = null_array();
    }
}

Env get_random_env(int R, int C, int N, int skip_last_port, int track_history, int should_reorder, int is_extended)
{
    assert(R > 0 && C > 0 && N > 0);
    assert(skip_last_port == 0 || skip_last_port == 1);
    Env env;

    env.bay = get_bay(R, C, N);
    env.T = get_random_transportation_matrix(N, R * C);
    env.skip_last_port = skip_last_port;
    env.is_extended = is_extended;

    int upper_triangle_size = (N * (N - 1)) / 2;
    env.flat_T_matrix = get_zeros(upper_triangle_size);
    env.one_hot_bay = get_zeros((N - 1) * R * C);
    env.should_reorder = should_reorder;
    insert_flat_T_matrix(env);
    initialize_history(&env, track_history);
    initialize_extended_mask(&env);
    return env;
}

Env get_specific_env(int R, int C, int N, int *T_matrix, int skip_last_port, int track_history, int should_reorder, int is_extended)
{
    assert(R > 0 && C > 0 && N > 0);
    assert(skip_last_port == 0 || skip_last_port == 1);
    Env env;

    env.bay = get_bay(R, C, N);
    env.T = get_specific_transportation_matrix(N, T_matrix);
    env.skip_last_port = skip_last_port;
    env.is_extended = is_extended;

    int upper_triangle_size = (N * (N - 1)) / 2;
    env.flat_T_matrix = get_zeros(upper_triangle_size);
    env.one_hot_bay = get_zeros((N - 1) * R * C);
    env.should_reorder = should_reorder;
    insert_flat_T_matrix(env);
    initialize_history(&env, track_history);
    initialize_extended_mask(&env);
    return env;
}

Env copy_env(Env env, int track_history)
{
    Env copy;
    copy.bay = copy_bay(env.bay);
    copy.T = copy_transportation_info(env.T);
    copy.skip_last_port = env.skip_last_port;
    copy.flat_T_matrix = copy_array(env.flat_T_matrix);
    copy.one_hot_bay = copy_array(env.one_hot_bay);
    copy.should_reorder = env.should_reorder;
    copy.is_extended = env.is_extended;

    if (env.is_extended)
        copy.extended_mask = copy_array(env.extended_mask);
    else
        copy.extended_mask = null_array();

    if (track_history)
    {
        copy.history = (char *)malloc(env.bay.R * env.bay.C * env.bay.R * env.bay.C * (env.T->N - 1) * sizeof(char));
        memcpy(copy.history, env.history, env.bay.R * env.bay.C * env.bay.R * env.bay.C * (env.T->N - 1) * sizeof(char));
        copy.history_index = (int *)malloc(sizeof(int));
        copy.history_index[0] = env.history_index[0];
    }
    else
    {
        copy.history = NULL;
        copy.history_index = (int *)malloc(sizeof(int));
        copy.history_index[0] = -1;
    }
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
    free(env.history);
    free(env.history_index);
    free_array(env.extended_mask);
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

void backpropagate_reshuffle_in_history(int row, int column, Env *env)
{
    if (env->history_index[0] == -1)
        return;

    int i = env->history_index[0];
    while (1)
    {
        int index = i * env->bay.R * env->bay.C + row * env->bay.C + column;
        if (env->history[index] == 0)
            break;

        env->history[index] = 1;
        i--;
    };
}

void handle_sailing(Env env)
{
    while (no_containers_at_port(env.T) && !is_last_port(env.T))
    {
        transportation_sail_along(env.T);
        Array reshuffled = bay_sail_along(env.bay, backpropagate_reshuffle_in_history, &env);
        transportation_insert_reshuffled(env.T, reshuffled);
        free_array(reshuffled);
    }
}

int add_container(Env env, int action)
{
    int column;
    int n_containers;
    if (env.is_extended)
    {
        column = (action / env.bay.R) % env.bay.C;
        n_containers = action % env.bay.R + 1;
    }
    else
    {
        column = action;
        n_containers = 1;
    }

    int next_container = transportation_pop_n_containers(env.T, n_containers);
    int reward = get_add_reward(env, column, next_container);
    bay_add_containers(env.bay, column, next_container, n_containers, env.should_reorder);

    handle_sailing(env);

    return reward;
}

int remove_container(Env env, int action)
{
    int column = action - env.bay.C;
    int row = env.bay.R - env.bay.column_counts.values[column];
    backpropagate_reshuffle_in_history(row, column, &env);

    int top_container = get_top_container(env.bay, column);
    int reward = get_remove_reward(env, column, top_container);
    bay_pop_containers(env.bay, column, 1, env.should_reorder);
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

void copy_bay_to_history(Env env)
{
    if (env.history_index[0] == -1)
        return;

    env.history_index[0]++;

    if (env.history_index[0] == env.bay.R * env.bay.C * (env.bay.N - 1))
    {
        // Slice would be out of bounds, so we stop tracking history further
        env.history_index[0] = -1;
        return;
    }

    for (int j = 0; j < env.bay.C; j++)
    {
        for (int i = env.bay.R - env.bay.column_counts.values[j]; i < env.bay.R; i++)
        {
            int index = env.history_index[0] * env.bay.R * env.bay.C + i * env.bay.C + j;
            env.history[index] = 2;
        }
    }
}

void cleanup_history(Env env)
{
    if (env.history_index[0] == -1)
        return;
    for (int slice = 0; slice < env.history_index[0] + 1; slice++)
    {
        for (int j = 0; j < env.bay.C; j++)
        {
            int i = env.bay.R - 1;
            while (i >= 0)
            {
                int index = slice * env.bay.R * env.bay.C + i * env.bay.C + j;
                if (env.history[index] == 0)
                    break;
                else if (env.history[index] == 2)
                    env.history[index] = 0;

                i--;
            }
        }
    }
}

void step_action(StepInfo *step_info, int action, Env env)
{
    int is_adding_container;
    if (env.is_extended)
        is_adding_container = action < env.bay.C * env.bay.R;
    else
        is_adding_container = action < env.bay.C;

    if (is_adding_container)
        step_info->reward = add_container(env, action);
    else
        step_info->reward = remove_container(env, action);
}

// Action is in range [0, 2 * C)
// Action < C: Add container to column
// Action >= C: Remove container from column
StepInfo step(Env env, int action)
{
    if (env.is_extended)
    {
        assert(action >= 0 && action < 2 * env.bay.C * env.bay.R);
        assert(env.extended_mask.values[action] == 1);
    }
    else
    {
        assert(action >= 0 && action < 2 * env.bay.C);
        assert(env.bay.mask.values[action] == 1);
    }

    StepInfo step_info;
    step_action(&step_info, action, env);
    decide_is_terminated(&step_info, env);

    insert_flat_T_matrix(env);

    if (env.is_extended)
        insert_extended_mask(env);

    // insert_one_hot_bay(env);
    copy_bay_to_history(env);

    if (step_info.is_terminal)
        cleanup_history(env);

    return step_info;
}