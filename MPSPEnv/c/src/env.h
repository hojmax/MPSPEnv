#ifndef ENV_INCLUDED
#define ENV_INCLUDED
#include "transportation_matrix.h"
#include "bay.h"
#include "array.h"

typedef struct Env
{
    Transportation_Info *T;
    Bay bay;
    Array one_hot_bay;
    Array flat_T_matrix;
    int skip_last_port;
    int *history_index;
    char *history;
} Env;

typedef struct StepInfo
{
    int is_terminal;
    int reward;
} StepInfo;

StepInfo step(Env env, int action);

Env copy_env(Env env, int track_history);

Env get_random_env(int R, int C, int N, int skip_last_port, int track_history);

Env get_specific_env(int R, int C, int N, int *T_matrix, int skip_last_port, int track_history);

void free_env(Env env);

#endif