#include "env.h"
#include "bay.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int columns_identical(Bay bay, int c1, int c2)
{
    for (int r = bay.R - 1; r >= 0; r--)
    {
        int value1 = bay.matrix.values[r * bay.C + c1];
        int value2 = bay.matrix.values[r * bay.C + c2];

        if (value1 != value2)
            return 0;

        if (value1 == 0)
            break;
    }
    return 1;
}

// First half of output answer whether there exists a column to the left that is identical
// The second half of output answer whether there exists a column to the right that is identical
Array are_columns_identical(Bay bay)
{
    Array output = get_zeros(2 * bay.C);
    for (int c1 = 0; c1 < bay.C; c1++)
    {
        for (int c2 = c1 + 1; c2 < bay.C; c2++)
        {
            int identical = columns_identical(bay, c1, c2);
            if (identical)
            {
                output.values[c1 + bay.C] = 1;
                output.values[c2] = 1;
            }
        }
    }
    return output;
}

int compute_mask_entry(Env env, int i, int empty_spots_to_left, Array identical_columns)
{
    // RULE 1: The current number of containers plus the number of containers to add must be less than or equal to R
    // RULE 2: The current number of containers minus the number of containers to remove must be greater than or equal to 0
    // RULE 3: You may only add one type of container at a time (i.e. you cant add 2x2s and 2x5s in a column at the same time)
    // The rules below assumes that columns are sorted occarding the values in the rows from bottom to top:
    // RULE 4: You may only remove if you have not yet added containers (resets after sailing)
    // RULE 5: You may only add to a column that is to the left of the last column you added to (resets after each container type)
    // RULE 6: You may only remove from a column that is to the right of the last column you removed from (resets after sailing)
    // RULE 7: You may not take an action that leaves no options for the next action
    // RULE 8: If two columns are identical, you may only add to the rightmost column
    // RULE 9: If two columns are identical, you may only remove from the leftmost column

    // Action space is column major order:
    // [c0r1, c0r2, c0r3, …, c0rR, c1r1, c1r2, …, c2*C-1rR]
    int is_add = i < env.bay.C * env.bay.R;
    int column = (i / env.bay.R) % env.bay.C;
    int n_to_place = i % env.bay.R + 1;
    int n_of_type = env.T->matrix.values[env.T->last_non_zero_column];
    if (is_add)
    {
        int non_zero = env.T->last_non_zero_column != -1;
        int no_column_overflow = n_to_place + containers_in_column(env.bay, column) <= env.bay.R;
        int not_more_than_type = n_of_type >= n_to_place;
        int add_to_the_left = column < *(env.bay.right_most_added_column);
        int not_traped = empty_spots_to_left + n_to_place >= n_of_type;
        int no_right_identical = !identical_columns.values[env.bay.C + column];

        return (non_zero &&
                no_column_overflow &&
                not_more_than_type &&
                add_to_the_left &&
                not_traped &&
                no_right_identical);
    }
    else
    {
        int have_not_added_yet = !*(env.bay.added_since_sailing);
        int not_too_few = (containers_in_column(env.bay, column) - n_to_place >= 0);
        int remove_to_the_right = column > *(env.bay.left_most_removed_column);
        int no_left_identical = !identical_columns.values[column];

        return (have_not_added_yet &&
                not_too_few &&
                remove_to_the_right &&
                no_left_identical);
    }
}

// If there is only one legal action, the action index is returned, otherwise -1 is returned
int insert_mask(Env env)
{
    int last_legal_action = -1;
    int n_legal_actions = 0;
    int empty_spots_to_left = 0;
    Array identical_columns = are_columns_identical(env.bay);

    for (int is_remove = 0; is_remove <= 1; is_remove++)
    {
        for (int column = 0; column < env.bay.C; column++)
        {
            for (int n_containers = 1; n_containers <= env.bay.R; n_containers++)
            {
                int index = is_remove * env.bay.C * env.bay.R + column * env.bay.R + n_containers - 1;
                env.mask.values[index] = compute_mask_entry(env, index, empty_spots_to_left, identical_columns);

                if (env.mask.values[index])
                {
                    n_legal_actions += 1;
                    last_legal_action = index;
                }
            }
            empty_spots_to_left += env.bay.R - containers_in_column(env.bay, column);
        }
    }
    if (n_legal_actions == 1)
        return last_legal_action;
    else
        return -1;
}

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

void handle_sailing(Env env)
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

    handle_sailing(env);

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
