#include <time.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void set_seed(int seed)
{
    srand(seed);
}

void set_random_seed()
{
    srand(time(NULL));
}

int random_int()
{
    return rand();
}

// Returns a random float in the range [0, 1)
float random_float()
{
    return (float)rand() / (float)RAND_MAX;
}

// Returns a random integer in the range [min, max)
int random_uniform_int(int min, int max)
{
    assert(min < max);
    return min + random_int() % (max - min);
}

int compare_ints(const void *a, const void *b)
{
    int int_a = *(int *)a;
    int int_b = *(int *)b;

    if (int_a < int_b)
        return -1;
    else if (int_a > int_b)
        return 1;
    else
        return 0;
}

void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void shuffle(int *array, int n)
{
    for (int i = 0; i < n; i++)
    {
        int j = random_uniform_int(i, n);
        if (i != j)
        {
            swap(&array[i], &array[j]);
        }
    }
}

// v is randomly partitioned into b integers
int *random_partition(int v, int b)
{
    int *x = (int *)malloc(b * sizeof(int));
    if (b == 1)
    {
        x[0] = v;
        return x;
    }

    int *y = (int *)malloc((v + b - 1) * sizeof(int));
    for (int n = 0; n < v + b - 1; n++)
    {
        y[n] = n + 1;
    }

    for (int i = 0; i < b; i++)
    {
        int j = random_uniform_int(i, v + b - 1);
        if (i != j)
        {
            swap(&y[i], &y[j]);
        }
    }

    qsort(y, b, sizeof(int), compare_ints);

    x[0] = y[0] - 1;
    for (int i = 1; i < b - 1; i++)
    {
        x[i] = y[i] - y[i - 1] - 1;
    }
    x[b - 1] = v + b - 1 - y[b - 2];

    shuffle(x, b);

    free(y);
    return x;
}