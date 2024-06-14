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

int column_would_be_subset(Bay bay, int c1, int n_containers)
{
    for (int c2 = c1 - 1; c2 >= 0; c2--)
    {
        int are_identical = 1;
        for (int r = bay.R - 1; r >= bay.R - 1 + n_containers - containers_in_column(bay, c1); r--)
        {
            int value1 = bay.matrix.values[r * bay.C + c1];
            int value2 = bay.matrix.values[r * bay.C + c2];

            if (value1 != value2)
            {
                are_identical = 0;
                break;
            }
        }
        if (are_identical)
            return 1;
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

int compute_mask_entry(Env env, int i)
{
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

        // More expensive to compute, so we only do it if the basic rules are satisfied
        int not_traped = max_to_place_after_action(env, column, n_containers, type) + n_containers >= n_of_type;
        if (!not_traped)
            return 0;

        int not_superset = !column_would_be_superset(env.bay, column, n_containers, type);
        return not_superset;
    }
    else
    {
        int have_not_added_yet = !*(env.bay.added_since_sailing);
        int not_too_few = (containers_in_column(env.bay, column) - n_containers >= 0);
        int remove_to_the_right = column > *(env.bay.left_most_removed_column);

        if (!have_not_added_yet ||
            !not_too_few ||
            !remove_to_the_right)
            return 0;

        // More expensive to compute, so we only do it if the basic rules are satisfied
        int would_not_be_subset = !column_would_be_subset(env.bay, column, n_containers);
        return would_not_be_subset;
    }
}

// If there is only one legal action, the action index is returned, otherwise -1 is returned
int insert_mask(Env env)
{
    int last_legal_action = -1;
    int n_legal_actions = 0;

    for (int is_remove = 0; is_remove <= 1; is_remove++)
    {
        for (int column = 0; column < env.bay.C; column++)
        {
            for (int n_containers = 1; n_containers <= env.bay.R; n_containers++)
            {
                int index = is_remove * env.bay.C * env.bay.R + column * env.bay.R + n_containers - 1;
                env.mask.values[index] = compute_mask_entry(env, index);

                if (env.mask.values[index])
                {
                    n_legal_actions += 1;
                    last_legal_action = index;
                }
            }
        }
    }

    if (n_legal_actions == 1)
        return last_legal_action;
    else
        return -1;
}
