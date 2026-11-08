#ifndef LAB4_SUM_H
#define LAB4_SUM_H
#include <stdint.h>
#include <stddef.h>
struct SumArgs {
    const int *array;
    size_t begin;
    size_t end;
    int64_t partial;
};
int64_t Sum(const struct SumArgs *args);
void *ThreadSum(void *arg);
#endif
