#include "env.h"
#include "bay.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Array get_left_right_identical(Bay bay)
{
    Array left_right_identical = get_zeros(bay.C * 2); // Whether there exists a column to the left (first C entries) or right (last C entries) that is identical
    for (int c1 = 0; c1 < bay.C; c1++)
    {
        for (int c2 = c1 + 1; c2 < bay.C; c2++)
        {
            int identical = columns_identical(bay, c1, c2);
            if (identical)
            {
                left_right_identical.values[c1 + bay.C] = 1;
                left_right_identical.values[c2] = 1;
            }
        }
    }
    return left_right_identical;
}

int max_to_place_after_action(Env env, int column, int n_containers, int type)
{
    assert(containers_in_column(env.bay, column) + n_containers <= env.bay.R);
    Array bay_copy = copy_array(env.bay.matrix);

    for (int i = 0; i < n_containers; i++)
    {
        int r1 = env.bay.R - 1 - i - containers_in_column(env.bay, column);
        bay_copy.values[r1 * env.bay.C + column] = type;
    }

    int total = 0;
    for (int c1 = column - 1; c1 >= 0; c1--)
    {
        int max_to_place = env.bay.R - containers_in_column(env.bay, c1);
        for (int c2 = c1 + 1; c2 < env.bay.C; c2++)
        {
            int to_place = 0;
            for (int r = env.bay.R - 1; r >= 0; r--)
            {
                int value1 = bay_copy.values[r * env.bay.C + c1];
                int value2 = bay_copy.values[r * env.bay.C + c2];

                if (value1 == 0 && value2 == 0)
                {
                    max_to_place = min(max_to_place, to_place);
                    break;
                }
                else if (value1 == value2)
                {
                    continue;
                }
                else if (value1 == 0 && value2 == type)
                {
                    to_place++;
                    continue;
                }
                else
                {
                    break;
                }
            }
            if (max_to_place == 0)
                break;
        }
        total += max_to_place;
        for (int i = 0; i < max_to_place; i++)
        {
            int r1 = env.bay.R - 1 - i - containers_in_column(env.bay, c1);
            bay_copy.values[r1 * env.bay.C + c1] = type;
        }
    }

    free_array(bay_copy);
    return total;
}

// Cases mask does not currently cover for remove:
// Move sequence 1:
// [0, 1] -r1c0-> [0, 1] -r1c1-> [0, 0]
// [1, 1]         [0, 1]         [0, 1]
// Move sequence 2:
// [0, 1] -r2c1-> [0, 0]
// [1, 1]         [0, 1]
// In both cases, the sequence transposes to the same state, leading to redudancy in the search space.
int compute_mask_entry(Env env, int i, Array left_right_identical, Array column_would_be_superset)
{
    // RULE 1: The current number of containers plus the number of containers to add must be less than or equal to R
    // RULE 2: The current number of containers minus the number of containers to remove must be greater than or equal to 0
    // RULE 3: You may only add one type of container at a time (i.e. you cant add 2x2s and 2x5s in a column at the same time)
    // The rules below assumes that columns are sorted according to the values in the rows from bottom to top:
    // RULE 4: You may only remove if you have not yet added containers (resets after sailing)
    // RULE 5: You may only add to a column that is to the left of the last column you added to (resets after each container type)
    // RULE 6: You may only remove from a column that is to the right of the last column you removed from (resets after sailing)
    // RULE 7: If two columns are identical, you may only add to the rightmost column
    // RULE 8: If two columns are identical, you may only remove from the leftmost column
    // RULE 9: You may not place more containers into a column than you have done into a previous identical column (resets after each container type)
    // RULE 10: You may not take an action that leaves no options for the next action (relevant for rule 5 and 9)
    int is_add = i < env.bay.C * env.bay.R;
    int column = (i / env.bay.R) % env.bay.C;
    int n_containers = i % env.bay.R + 1;

    if (is_add)
    {
        int no_column_overflow = n_containers + containers_in_column(env.bay, column) <= env.bay.R;
        if (!no_column_overflow)
            return 0;

        int type = env.T->last_non_zero_column;
        int n_of_type = env.T->matrix.values[type];
        int non_zero = env.T->last_non_zero_column != -1;
        int not_more_than_type = n_of_type >= n_containers;
        int add_to_the_left = column < *(env.bay.right_most_added_column);
        int not_traped = max_to_place_after_action(env, column, n_containers, type) + n_containers >= n_of_type;
        int not_superset = !column_would_be_superset.values[(env.bay.R - containers_in_column(env.bay, column) - n_containers) * env.bay.C + column];

        return (non_zero &&
                no_column_overflow &&
                not_more_than_type &&
                add_to_the_left &&
                not_traped &&
                not_superset);
    }
    else
    {
        int have_not_added_yet = !*(env.bay.added_since_sailing);
        int not_too_few = (containers_in_column(env.bay, column) - n_containers >= 0);
        int remove_to_the_right = column > *(env.bay.left_most_removed_column);
        int no_left_identical = !left_right_identical.values[column];
        int not_more_than_previous_identical = n_containers <= env.bay.max_to_remove_for_identical.values[env.bay.right_most_identical_index.values[column]];

        return (have_not_added_yet &&
                not_too_few &&
                remove_to_the_right &&
                no_left_identical &&
                not_more_than_previous_identical);
    }
}

// If there is only one legal action, the action index is returned, otherwise -1 is returned
int insert_mask(Env env)
{
    int last_legal_action = -1;
    int n_legal_actions = 0;
    Array left_right_identical = get_left_right_identical(env.bay);
    Array column_would_be_superset = adding_containers_would_be_right_identical(env.bay, env.T->last_non_zero_column);

    for (int is_remove = 0; is_remove <= 1; is_remove++)
    {
        for (int column = 0; column < env.bay.C; column++)
        {
            for (int n_containers = 1; n_containers <= env.bay.R; n_containers++)
            {
                int index = is_remove * env.bay.C * env.bay.R + column * env.bay.R + n_containers - 1;
                env.mask.values[index] = compute_mask_entry(env, index, left_right_identical, column_would_be_superset);

                if (env.mask.values[index])
                {
                    n_legal_actions += 1;
                    last_legal_action = index;
                }
            }
        }
    }

    free_array(left_right_identical);
    free_array(column_would_be_superset);

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

int sail_and_reshuffle(Env env)
{
    int sailed_along = 0;
    while (no_containers_at_port(env.T) && !is_last_port(env.T))
    {
        transportation_sail_along(env.T);
        Array reshuffled = bay_sail_along(env.bay, &env);
        *env.containers_left += get_sum(reshuffled);
        transportation_insert_reshuffled(env.T, reshuffled);
        free_array(reshuffled);
        sailed_along = 1;
    }
    return sailed_along;
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

    int sailed_along = sail_and_reshuffle(env);

    if (sailed_along)
    {
        reset_identical_add_limitation(env.bay);
        reset_identical_remove_limitation(env.bay);
    }
    else if (n_containers == n_left_of_type)
    {
        reset_identical_add_limitation(env.bay);
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
    reset_identical_add_limitation(env.bay);

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
