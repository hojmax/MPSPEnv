#include "../src/env.h"
#include "../src/array.h"
#include "../src/play.h"
#include "../src/random.h"
#include <stdio.h>
#include <stdlib.h>

float calculate_stats(int R, int C, int N, int repeats)
{
    float max_moves = 0;
    float average_moves = 0;
    float min_moves = 1000;

    for (int i = 0; i < repeats; i++)
    {
        Env env = get_random_env(R, C, N, 1);
        int moves = env.moves_upper_bound;

        if (moves > max_moves)
            max_moves = moves;

        if (moves < min_moves)
            min_moves = moves;

        average_moves += moves;

        free_env(env);
    }

    average_moves /= repeats;
    printf("max_moves: %f\n", max_moves);
    printf("min_moves: %f\n", min_moves);
    printf("average_moves: %f\n", average_moves);
}

int main()
{
    int R = 12;
    int C = 12;
    int N = 16;
    set_seed(0);
    calculate_stats(R, C, N, 1000);
}