#ifndef RANDOM_INCLUDED
#define RANDOM_INCLUDED

void set_seed(int seed);

int random_uniform_int(int min, int max);

int random_int();

float random_float();

int *random_partition(int v, int b);

#endif