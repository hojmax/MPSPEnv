#include "../src/env.h"
#include "../src/array.h"
#include "../src/random.h"
#include <stdio.h>
#include <stdlib.h>

int get_random_action(Array mask, float remove_prob)
{
    int n;
    // Multiply by 2
    // Because half of actions are remove
    // and half are add
    if (random_float() < remove_prob * 2)
        n = mask.n;
    else
        n = mask.n / 2;
    int action = rand() % n;
    while (mask.values[action] == 0)
    {
        action = rand() % n;
    }
    return action;
}

void print_env(Env env, int action, int episode_reward)
{
    print_matrix(env.bay.matrix, env.bay.C, env.bay.R);
    printf("action: %d\n", action);
    printf("episode_reward: %d\n", episode_reward);
    print_matrix(env.T->matrix, env.bay.N, env.bay.N);
    printf("\n");
}

int run_episode(int R, int C, int N, int print, float remove_prob)
{
    Env env = get_random_env(R, C, N, 1);
    int action = get_random_action(env.bay.mask, remove_prob);
    StepInfo step_info = step(env, action);
    int episode_reward = step_info.reward;

    while (!step_info.is_terminal)
    {
        if (print)
            print_env(env, action, episode_reward);
        action = get_random_action(env.bay.mask, remove_prob);
        step_info = step(env, action);
        episode_reward += step_info.reward;
    }

    if (print)
        print_env(env, action, episode_reward);

    free_env(env);
    return episode_reward;
}

float get_average_reward(int R, int C, int N, int repeats)
{
    int sum_reward = 0;
    set_random_seed();

    for (int i = 0; i < repeats; i++)
        sum_reward += run_episode(R, C, N, 0, 0);

    return (float)sum_reward / repeats;
}

int main()
{
    int R = 5;
    int C = 3;
    int N = 8;
    printf("average reward: %f\n", get_average_reward(R, C, N, 100000));
    // set_random_seed();
    // run_episode(5, 3, 8, 1, 0.2);
}