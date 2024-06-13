#include "mask.h"
#include "env.h"
#include "bay.h"
#include "array.h"
#include <assert.h>

int column_would_be_superset(Bay bay, int c1, int n_containers, int type)
{
    for (int c2 = c1 + 1; c2 < bay.C; c2++)
    {
        for (int r = bay.R - 1; r >= bay.R - n_containers - containers_in_column(bay, c1); r--)
        {
            int value1 = bay.matrix.values[r * bay.C + c1];
            int value2 = bay.matrix.values[r * bay.C + c2];

            if (value1 == 0 && value2 == 0)
                return 1;
            else if (value1 == value2 || (value1 == 0 && value2 == type))
                continue;
            else
                break;
        }
    }
    return 0;
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
int compute_mask_entry(Env env, int i, Array left_right_identical)
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
        int type = env.T->last_non_zero_column;
        int n_of_type = env.T->matrix.values[type];
        int non_zero = env.T->last_non_zero_column != -1;
        int not_more_than_type = n_of_type >= n_containers;
        int no_column_overflow = n_containers + containers_in_column(env.bay, column) <= env.bay.R;
        int add_to_the_left = column < *(env.bay.right_most_added_column);

        if (!non_zero || !not_more_than_type || !no_column_overflow || !add_to_the_left)
            return 0;

        // These are more expensive to compute, so we only do it if the basic rules are satisfied
        int not_traped = max_to_place_after_action(env, column, n_containers, type) + n_containers >= n_of_type;
        int not_superset = !column_would_be_superset(env.bay, column, n_containers, type);
        return (not_traped && not_superset);
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

// If there is only one legal action, the action index is returned, otherwise -1 is returned
int insert_mask(Env env)
{
    int last_legal_action = -1;
    int n_legal_actions = 0;
    Array left_right_identical = get_left_right_identical(env.bay);

    for (int is_remove = 0; is_remove <= 1; is_remove++)
    {
        for (int column = 0; column < env.bay.C; column++)
        {
            for (int n_containers = 1; n_containers <= env.bay.R; n_containers++)
            {
                int index = is_remove * env.bay.C * env.bay.R + column * env.bay.R + n_containers - 1;
                env.mask.values[index] = compute_mask_entry(env, index, left_right_identical);

                if (env.mask.values[index])
                {
                    n_legal_actions += 1;
                    last_legal_action = index;
                }
            }
        }
    }

    free_array(left_right_identical);

    if (n_legal_actions == 1)
        return last_legal_action;
    else
        return -1;
}