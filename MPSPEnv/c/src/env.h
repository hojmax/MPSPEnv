#ifndef ENV_INCLUDED
#define ENV_INCLUDED
#include "transportation_matrix.h"
#include "bay.h"
#include "array.h"

typedef struct Env
{
    Transportation_Info *T;
    Bay bay;
    Array mask;
    int auto_move;
    int *total_reward;
    int *containers_left;
    int *containers_placed;
    int *terminated;
} Env;

typedef struct StepInfo
{
    int terminated;
    int reward;
} StepInfo;

StepInfo env_step(Env env, int action);

Env copy_env(Env env);

Env get_random_env(int R, int C, int N, int auto_move);

Env get_specific_env(int R, int C, int N, int *T_matrix, int auto_move);

void free_env(Env env);

#endif